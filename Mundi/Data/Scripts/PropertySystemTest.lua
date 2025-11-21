-- PropertySystemTest.lua
-- Comprehensive test script for Lua property binding system
-- Tests: basic types, vectors, arrays, maps, enums, structs, and recursive access

local TestComp = nil
local TestsPassed = 0
local TestsFailed = 0

-- Helper function to log test results
local function LogTest(testName, passed, message)
    if passed then
        print("[PASS] " .. testName .. (message and (" - " .. message) or ""))
        TestsPassed = TestsPassed + 1
    else
        print("[FAIL] " .. testName .. (message and (" - " .. message) or ""))
        TestsFailed = TestsFailed + 1
    end
end

-- Helper function to compare floats with tolerance
local function FloatEquals(a, b, tolerance)
    tolerance = tolerance or 0.001
    return math.abs(a - b) < tolerance
end

function BeginPlay()
    print("========================================")
    print("Property System Binding Test Started")
    print("========================================")

    -- Get the PropertyTestComponent from this actor
    TestComp = GetComponent(Obj, "UPropertyTestComponent")

    if not TestComp then
        print("[ERROR] Failed to get UPropertyTestComponent!")
        print("Make sure this actor has UPropertyTestComponent attached.")
        return
    end

    print("[INFO] Successfully retrieved UPropertyTestComponent")
    print("")

    -- Run all tests
    TestBasicTypes()
    TestVectorTypes()
    TestArrayTypes()
    TestMapTypes()
    TestEnumTypes()
    TestStructTypes()
    TestRecursiveAccess()
    TestComponentChaining()

    -- Print summary
    print("")
    print("========================================")
    print("Test Summary")
    print("========================================")
    print("Passed: " .. TestsPassed)
    print("Failed: " .. TestsFailed)
    print("Total:  " .. (TestsPassed + TestsFailed))
    print("========================================")
end

function EndPlay()
    print("Property System Test Ended")
end

function Tick(dt)
    -- No per-frame testing needed
end

-- ===== Basic Types Test =====
function TestBasicTypes()
    print("")
    print("--- Testing Basic Types ---")

    -- Test bool
    local originalBool = TestComp.bTestBool
    TestComp.bTestBool = true
    LogTest("Bool Write/Read (true)", TestComp.bTestBool == true)
    TestComp.bTestBool = false
    LogTest("Bool Write/Read (false)", TestComp.bTestBool == false)
    TestComp.bTestBool = originalBool

    -- Test int32
    local originalInt = TestComp.TestInt
    TestComp.TestInt = 42
    LogTest("Int32 Write/Read", TestComp.TestInt == 42)
    TestComp.TestInt = -100
    LogTest("Int32 Negative", TestComp.TestInt == -100)
    TestComp.TestInt = originalInt

    -- Test float
    local originalFloat = TestComp.TestFloat
    TestComp.TestFloat = 3.14159
    LogTest("Float Write/Read", FloatEquals(TestComp.TestFloat, 3.14159))
    TestComp.TestFloat = -0.5
    LogTest("Float Negative", FloatEquals(TestComp.TestFloat, -0.5))
    TestComp.TestFloat = originalFloat

    -- Test FString
    local originalString = TestComp.TestString
    TestComp.TestString = "Hello Lua!"
    LogTest("FString Write/Read", TestComp.TestString == "Hello Lua!")
    TestComp.TestString = ""
    LogTest("FString Empty", TestComp.TestString == "")
    TestComp.TestString = originalString

    -- Test FName (use Name() constructor to create FName from string)
    local originalName = TestComp.TestName
    TestComp.TestName = Name("TestNameValue")
    -- FName comparison: convert to string for comparison
    local readName = TestComp.TestName
    local nameStr = readName:ToString()
    LogTest("FName Write/Read", nameStr == "TestNameValue", "Got: " .. nameStr)
    TestComp.TestName = originalName
end

-- ===== Vector Types Test =====
function TestVectorTypes()
    print("")
    print("--- Testing Vector Types ---")

    -- Test FVector
    local originalVector = TestComp.TestVector
    TestComp.TestVector = Vector(1.0, 2.0, 3.0)
    local v = TestComp.TestVector
    LogTest("FVector Write/Read",
        FloatEquals(v.X, 1.0) and FloatEquals(v.Y, 2.0) and FloatEquals(v.Z, 3.0),
        string.format("Got: (%.2f, %.2f, %.2f)", v.X, v.Y, v.Z))

    -- Test FVector component access (Note: direct component write requires full vector assignment)
    -- TestComp.TestVector.X = 10.0 returns a COPY, so we must assign the full vector
    local v2 = TestComp.TestVector
    v2.X = 10.0
    v2.Y = 20.0
    v2.Z = 30.0
    TestComp.TestVector = v2  -- Assign modified copy back
    local v3 = TestComp.TestVector
    LogTest("FVector Component Modify via Copy",
        FloatEquals(v3.X, 10.0) and FloatEquals(v3.Y, 20.0) and FloatEquals(v3.Z, 30.0),
        string.format("Got: (%.2f, %.2f, %.2f)", v3.X, v3.Y, v3.Z))

    TestComp.TestVector = originalVector

    -- Test FLinearColor (Note: constructor is Color(), not LinearColor())
    local originalColor = TestComp.TestColor
    TestComp.TestColor = Color(1.0, 0.5, 0.25, 1.0)
    local c = TestComp.TestColor
    LogTest("FLinearColor Write/Read",
        FloatEquals(c.R, 1.0) and FloatEquals(c.G, 0.5) and FloatEquals(c.B, 0.25) and FloatEquals(c.A, 1.0),
        string.format("Got: (%.2f, %.2f, %.2f, %.2f)", c.R, c.G, c.B, c.A))

    TestComp.TestColor = originalColor
end

-- ===== Array Types Test =====
function TestArrayTypes()
    print("")
    print("--- Testing Array Types ---")

    -- Note: LuaBindHelpers returns sol::table, so use Lua table syntax
    -- Arrays are returned as copies; modifications require reassignment

    -- Test TArray<int32>
    local intArray = TestComp.TestIntArray
    if intArray then
        local originalSize = #intArray

        -- Add elements to copy
        table.insert(intArray, 100)
        table.insert(intArray, 200)
        table.insert(intArray, 300)

        -- Assign back to C++
        TestComp.TestIntArray = intArray

        -- Read back and verify
        local readBack = TestComp.TestIntArray
        LogTest("IntArray Add", #readBack == originalSize + 3, "Size: " .. #readBack)

        -- Access elements (1-based index in Lua)
        local lastVal = readBack[#readBack]
        LogTest("IntArray Access", lastVal == 300, "Last element: " .. tostring(lastVal))

        -- Modify element
        readBack[#readBack] = 999
        TestComp.TestIntArray = readBack
        local modified = TestComp.TestIntArray
        LogTest("IntArray Set", modified[#modified] == 999)

        -- Restore original (remove added elements)
        for i = 1, 3 do
            table.remove(modified)
        end
        TestComp.TestIntArray = modified
        LogTest("IntArray Remove", #TestComp.TestIntArray == originalSize)
    else
        LogTest("IntArray Access", false, "Array is nil")
    end

    -- Test TArray<float>
    local floatArray = TestComp.TestFloatArray
    if floatArray then
        local originalSize = #floatArray
        table.insert(floatArray, 1.5)
        table.insert(floatArray, 2.5)
        TestComp.TestFloatArray = floatArray

        local readBack = TestComp.TestFloatArray
        LogTest("FloatArray Add", #readBack == originalSize + 2)
        LogTest("FloatArray Get", FloatEquals(readBack[#readBack], 2.5))

        -- Cleanup
        table.remove(readBack)
        table.remove(readBack)
        TestComp.TestFloatArray = readBack
    else
        LogTest("FloatArray Access", false, "Array is nil")
    end

    -- Test TArray<FString>
    local stringArray = TestComp.TestStringArray
    if stringArray then
        local originalSize = #stringArray
        table.insert(stringArray, "First")
        table.insert(stringArray, "Second")
        TestComp.TestStringArray = stringArray

        local readBack = TestComp.TestStringArray
        LogTest("StringArray Add", #readBack == originalSize + 2)
        LogTest("StringArray Get", readBack[#readBack] == "Second")

        -- Cleanup
        table.remove(readBack)
        table.remove(readBack)
        TestComp.TestStringArray = readBack
    else
        LogTest("StringArray Access", false, "Array is nil")
    end
end

-- ===== Map Types Test =====
function TestMapTypes()
    print("")
    print("--- Testing Map Types ---")

    -- Note: LuaBindHelpers returns sol::table for maps
    -- Maps are returned as copies; modifications require reassignment

    -- Test TMap<FString, int32>
    local stringIntMap = TestComp.TestStringIntMap
    if stringIntMap then
        -- Add entries to copy
        stringIntMap["Key1"] = 100
        stringIntMap["Key2"] = 200

        -- Assign back to C++
        TestComp.TestStringIntMap = stringIntMap

        -- Read back and verify
        local readBack = TestComp.TestStringIntMap
        local val1 = readBack["Key1"]
        local val2 = readBack["Key2"]
        LogTest("StringIntMap Add/Get", val1 == 100 and val2 == 200,
            string.format("Key1=%s, Key2=%s", tostring(val1), tostring(val2)))

        -- Contains check (nil means not present)
        local hasKey1 = readBack["Key1"] ~= nil
        local hasKey3 = readBack["Key3"] ~= nil
        LogTest("StringIntMap Contains", hasKey1 == true and hasKey3 == false)

        -- Remove entries
        readBack["Key1"] = nil
        readBack["Key2"] = nil
        TestComp.TestStringIntMap = readBack
        local afterRemove = TestComp.TestStringIntMap
        LogTest("StringIntMap Remove", afterRemove["Key1"] == nil)
    else
        LogTest("StringIntMap Access", false, "Map is nil")
    end

    -- Test TMap<FString, FString>
    local stringStringMap = TestComp.TestStringStringMap
    if stringStringMap then
        stringStringMap["Name"] = "TestValue"
        TestComp.TestStringStringMap = stringStringMap

        local readBack = TestComp.TestStringStringMap
        LogTest("StringStringMap Add/Get", readBack["Name"] == "TestValue")

        -- Cleanup
        readBack["Name"] = nil
        TestComp.TestStringStringMap = readBack
    else
        LogTest("StringStringMap Access", false, "Map is nil")
    end
end

-- ===== Enum Types Test =====
function TestEnumTypes()
    print("")
    print("--- Testing Enum Types ---")

    -- Test ETestMode enum
    local originalEnum = TestComp.TestEnum

    -- Set by integer value
    TestComp.TestEnum = 0  -- None
    LogTest("Enum Set (None=0)", TestComp.TestEnum == 0)

    TestComp.TestEnum = 1  -- ModeA
    LogTest("Enum Set (ModeA=1)", TestComp.TestEnum == 1)

    TestComp.TestEnum = 2  -- ModeB
    LogTest("Enum Set (ModeB=2)", TestComp.TestEnum == 2)

    TestComp.TestEnum = 3  -- ModeC
    LogTest("Enum Set (ModeC=3)", TestComp.TestEnum == 3)

    TestComp.TestEnum = originalEnum
end

-- ===== Struct Types Test =====
function TestStructTypes()
    print("")
    print("--- Testing Struct Types ---")

    -- Test FTestStruct
    local testStruct = TestComp.TestStruct
    if testStruct then
        -- Test struct field access
        local originalValue = testStruct.Value
        local originalName = testStruct.Name

        -- Write and read Value (float)
        testStruct.Value = 123.456
        LogTest("Struct Field Write/Read (Value)", FloatEquals(testStruct.Value, 123.456))

        -- Write and read Name (FString)
        testStruct.Name = "StructTest"
        LogTest("Struct Field Write/Read (Name)", testStruct.Name == "StructTest")

        -- Test nested struct (FVector inside FTestStruct)
        local pos = testStruct.Position
        if pos then
            -- Write to nested vector
            testStruct.Position = Vector(100.0, 200.0, 300.0)
            local newPos = testStruct.Position
            LogTest("Struct Nested Vector Write/Read",
                FloatEquals(newPos.X, 100.0) and FloatEquals(newPos.Y, 200.0) and FloatEquals(newPos.Z, 300.0),
                string.format("Got: (%.2f, %.2f, %.2f)", newPos.X, newPos.Y, newPos.Z))

            -- Test deep recursive access: TestStruct.Position.X
            -- Note: Direct component write returns a copy, so modify and assign back
            local posForModify = testStruct.Position
            posForModify.X = 999.0
            testStruct.Position = posForModify
            LogTest("Struct Deep Recursive Access (Position.X)",
                FloatEquals(testStruct.Position.X, 999.0),
                "Position.X = " .. tostring(testStruct.Position.X))
        else
            LogTest("Struct Nested Vector Access", false, "Position is nil")
        end

        -- Restore original values
        testStruct.Value = originalValue
        testStruct.Name = originalName
    else
        LogTest("Struct Access", false, "TestStruct is nil")
    end
end

-- ===== Recursive Object Access Test =====
function TestRecursiveAccess()
    print("")
    print("--- Testing Recursive Object Access ---")

    -- Test accessing actor properties through Obj
    local actorLoc = Obj.Location
    LogTest("Actor Location Access", actorLoc ~= nil,
        actorLoc and string.format("(%.2f, %.2f, %.2f)", actorLoc.X, actorLoc.Y, actorLoc.Z) or "nil")

    -- Test modifying actor location
    local originalLoc = Obj.Location
    Obj.Location = Vector(10.0, 20.0, 30.0)
    local newLoc = Obj.Location
    LogTest("Actor Location Write/Read",
        FloatEquals(newLoc.X, 10.0) and FloatEquals(newLoc.Y, 20.0) and FloatEquals(newLoc.Z, 30.0))
    Obj.Location = originalLoc

    -- Test chained property access: Obj -> Component -> Property -> Sub-property
    if TestComp then
        -- Access struct through component, then access nested property
        local structPos = TestComp.TestStruct.Position
        LogTest("Chained Access (Comp.Struct.Position)", structPos ~= nil)

        if structPos then
            -- Deep chain: Comp -> Struct -> Vector -> Component
            local xVal = TestComp.TestStruct.Position.X
            LogTest("Deep Chained Access (Comp.Struct.Position.X)", xVal ~= nil, "X = " .. tostring(xVal))
        end
    end
end

-- ===== Component Chaining Test =====
function TestComponentChaining()
    print("")
    print("--- Testing Component Chaining ---")

    -- Try to get other components and access their properties
    local sceneComp = GetComponent(Obj, "USceneComponent")
    if sceneComp then
        -- Access SceneComponent transform properties
        local relLoc = sceneComp.RelativeLocation
        if relLoc then
            LogTest("SceneComponent RelativeLocation Access", true,
                string.format("(%.2f, %.2f, %.2f)", relLoc.X, relLoc.Y, relLoc.Z))
        else
            LogTest("SceneComponent RelativeLocation Access", false, "nil")
        end

        -- Test modifying SceneComponent properties
        local origScale = sceneComp.RelativeScale
        sceneComp.RelativeScale = Vector(2.0, 2.0, 2.0)
        local newScale = sceneComp.RelativeScale
        LogTest("SceneComponent Scale Write/Read",
            newScale and FloatEquals(newScale.X, 2.0) and FloatEquals(newScale.Y, 2.0) and FloatEquals(newScale.Z, 2.0))
        sceneComp.RelativeScale = origScale
    else
        LogTest("SceneComponent Access", false, "Component not found")
    end

    -- Try to add and access a new component dynamically
    local newComp = AddComponent(Obj, "UPointLightComponent")
    if newComp then
        LogTest("Dynamic Component Add", true, "UPointLightComponent added")

        -- Access the new component's properties
        local origIntensity = newComp.Intensity
        newComp.Intensity = 5000.0
        LogTest("Dynamic Component Property Access", FloatEquals(newComp.Intensity, 5000.0),
            "Intensity = " .. tostring(newComp.Intensity))
        newComp.Intensity = origIntensity
    else
        LogTest("Dynamic Component Add", false, "Failed to add component")
    end
end

function OnBeginOverlap(OtherActor)
    -- Test recursive access through overlapping actors
    if OtherActor then
        print("[Overlap] Testing access to other actor...")
        local otherLoc = OtherActor.Location
        if otherLoc then
            print(string.format("  Other actor location: (%.2f, %.2f, %.2f)",
                otherLoc.X, otherLoc.Y, otherLoc.Z))
        end

        -- Try to get component from other actor
        local otherComp = GetComponent(OtherActor, "UPropertyTestComponent")
        if otherComp then
            print("  Other actor has UPropertyTestComponent: " .. tostring(otherComp.TestInt))
        end
    end
end

function OnEndOverlap(OtherActor)
end
