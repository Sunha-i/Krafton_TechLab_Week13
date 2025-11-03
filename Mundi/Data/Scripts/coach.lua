local maxTime = 28.0      -- 전체 색상 변화 시간 (초)
local whiteHoldTime = 8.0 -- 하얀색 유지 시간
local elapsedTime = 0.0
local meshComp = nil

-- 데칼 스폰 관련
local decalSpawnInterval = 2.0  -- 몇 초마다 데칼 생성할지
local decalTimer = 0.0

-- 시간 비율에 따라 색상 보간 (하양 → 노랑 → 주황 → 빨강)
local function LerpColorByTime(ratio)
    local color = Color(1.0, 1.0, 1.0) -- 기본: 하얀색

    if ratio < 0.33 then
        -- 0~0.33: White → Yellow
        local t = ratio / 0.33
        color = Color(1.0, 1.0, 1.0 - t)
    elseif ratio < 0.66 then
        -- 0.33~0.66: Yellow → Orange
        local t = (ratio - 0.33) / 0.33
        color = Color(1.0, 1.0 - 0.5 * t, 0.0)
    else
        -- 0.66~1.0: Orange → Red
        local t = (ratio - 0.66) / 0.34
        color = Color(1.0, 0.5 - 0.5 * t, 0.0)
    end

    return color
end

-- 랜덤 위치 오프셋 생성
local function RandomOffset(rangeXY, rangeZ)
    local x = (math.random() - 0.5) * 2 * rangeXY
    local y = (math.random() - 0.5) * 2 * rangeXY
    local z = (math.random() - 0.5) * 2 * rangeZ
    return Vector(x, y, z)
end

function BeginPlay()
    print("[BeginPlay] " .. Obj.UUID)
    Obj.Tag = "Damageable"

    meshComp = GetComponent(Obj, "UStaticMeshComponent")
    if meshComp then
        meshComp:SetColor(0, "DiffuseColor", Color(1.0, 1.0, 1.0)) -- 초기 하얀색
    end

    local crack = SpawnPrefab("Data/Prefabs/CrackDecal.prefab")
    print(crack.Location.X)
end

function EndPlay()
    print("[EndPlay] " .. Obj.UUID)
end

function Tick(dt)
    if not meshComp then
        return
    end

    -- 시간 누적
    elapsedTime = math.min(elapsedTime + dt, maxTime)

    -- 3초까지는 하얀색 유지
    if elapsedTime <= whiteHoldTime then
        meshComp:SetColor(0, "DiffuseColor", Color(1.0, 1.0, 1.0))
    else
        -- 이후 7초 동안 색상 변화
        local activeTime = elapsedTime - whiteHoldTime
        local colorChangeDuration = maxTime - whiteHoldTime
        local ratio = math.min(activeTime / colorChangeDuration, 1.0)
        local newColor = LerpColorByTime(ratio)
        meshComp:SetColor(0, "DiffuseColor", newColor)
    end

    
end
