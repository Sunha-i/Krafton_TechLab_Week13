-- Fireball 생성, 삭제를 관리해주는 스크립트입니다. 
local MaxFireNumber = 5
local CurrentFireNumber = 0
local FireballsPool = {} 

local function PushFireball(Fireball) 
    Fireball.bIsActive = false
    FireballsPool[#FireballsPool + 1] = Fireball

end 

local function PopFireball()
    local Count = #FireballsPool
    if Count == 0 then
        return nil
    end

    local Fireball = FireballsPool[Count]
    Fireball.bIsActive = true
    FireballsPool[Count] = nil
    
    return Fireball
end


function BeginPlay()
    print("[FireballManager BeginPlay] " .. Obj.UUID)

    for i = 1, MaxFireNumber do 
        local Fireball = SpawnPrefab("Data/Prefabs/Fireball.prefab")
        if Fireball then
            Fireball.bIsActive = false
            Fireball.LocalX = GlobalConfig.SpawnAreaPosX
            Fireball.LocalY = GlobalConfig.SpawnAreaPosY
            Fireball.LocalZ = GlobalConfig.SpawnAreaPosZ    
        end
        PushFireball(Fireball)
    end

    -- Fireball 생성 함수를 전역에 등록
    GlobalConfig.SpawnFireballAt = function(posX, posY, posZ)

        -- 최대 개수 조절
        if CurrentFireNumber >= MaxFireNumber then
            return false
        end
        
        NewFireball = PopFireball()
        if NewFireball ~= nil then
            NewFireball.Location.X = posX
            NewFireball.Location.Y = posY
            NewFireball.Location.Z = posZ
        end
        
        -- TODO: Tick에서 Spaw이 될 때, 그냥 SpawnPrefab으로 수정, 코루틴은 임시로 사용
        -- StartCoroutine(function()
        --     local go = SpawnPrefab("Data/Prefabs/Fireball.prefab")
        --     if go ~= nil then
        --         go.Location.X = posX
        --         go.Location.Y = posY
        --         go.Location.Z = posZ
        --     end
        -- end)
        return true
    end 

    -- Fireball 생성 가능여부 함수를 전역에 등록
    GlobalConfig.IsCanSpawnFireball = function()
        return CurrentFireNumber < MaxFireNumber
    end  

    GlobalConfig.ResetFireballs = function(InactiveFireball)
        InactiveFireball.bIsActive = false 
        InactiveFireball.Location.X = GlobalConfig.SpawnAreaPosX
        InactiveFireball.Location.Y = GlobalConfig.SpawnAreaPosY
        InactiveFireball.Location.Z = GlobalConfig.SpawnAreaPosZ
        
        CurrentFireNumber = CurrentFireNumber - 1 
    end

end

function SpawnFireball()
    for i = 1, 1 do 
        f= SpawnPrefab("Data/Prefabs/Fireball.prefab")
        f.bIsActive = true
        CurrentFireNumber = CurrentFireNumber + 1  
        --f.Location.X = posX
        --f.Location.Y = posY
        --f.Location.Z = posZ
        coroutine.yield("wait_time", 1.0)
    end 
end


function EndPlay()
    print("[FireballManager EndPlay] " .. Obj.UUID)
    -- Cleanup exposed functions
    GlobalConfig.SpawnFireballAt = nil
    GlobalConfig.IsCanSpawnFireball = nil
    GlobalConfig.ResetFireballs = nil
end

function Tick(dt)
    -- Manager could handle lifetime/cleanup here if needed
end

