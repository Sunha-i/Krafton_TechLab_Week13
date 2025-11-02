-- Fireball이 생성되는 위치를 정해주는 스크립트입니다
-- 현재는 X,Y의 바운드를 정할 수 있고, Z는 FireballArea Actor에서 가져옵니다.
local SpawnInterval = 1
local TimeAcc = 0.0

function BeginPlay()
    print("[BeginPlay] " .. Obj.UUID) 
    Obj.Scale.X = 10.0
    Obj.Scale.Y = 10.0 

    GlobalConfig.SpawnAreaPosX = Obj.Location.X
    GlobalConfig.SpawnAreaPosY = Obj.Location.Y
    GlobalConfig.SpawnAreaPosZ = Obj.Location.Z
    
end

function EndPlay()
    print("[EndPlay] " .. Obj.UUID)
end

function OnOverlap(OtherActor)
    -- Overlap hook if needed
end

function RandomInRange(MinRange, MaxRange)
    return MinRange + (MaxRange - MinRange) * math.random()
end

function Tick(dt)
    TimeAcc = TimeAcc + dt

    if not GlobalConfig or not GlobalConfig.SpawnFireballAt then 
        return
    end

    while TimeAcc >= SpawnInterval do
        TimeAcc = TimeAcc - SpawnInterval

        local LocalX, LocalY, LocalZ = Obj.Location.X, Obj.Location.Y, Obj.Location.Z 
        local ScaleX, ScaleY = Obj.Scale.X, Obj.Scale.Y

        local RangeX = RandomInRange(LocalX - ScaleX * 0.5 , LocalX + ScaleX * 0.5)
        local RangeY = RandomInRange(LocalY - ScaleY * 0.5, LocalY + ScaleY * 0.5)
        
        local PosX = RangeX
        local PosY = RangeY
        local PosZ = LocalZ
 
        GlobalConfig.SpawnFireballAt(PosX, PosY, PosZ)
        print("Spawn Fireball !!!!!!!")
    end 

   --[[print("[Tick] ")]]--
end