rotSpeed = 150.0
angle = 120.0
remainingAngle = 0.0
offset = float3.new(0, 0, -235)
scrollspeed = 15.0
targetAngle = 0.0
newZoomedPos = float3.new(0, 0, 0)
zoomSpeed = 50
mosquitoAlive = false
arrived = false
actualId = 0

local rotationEnded = true

local isStarting = true

local neralaAvailable
local omozraAvailable

function Start()
    leftArrow = Find("LeftArrow")
    rightArrow = Find("RightArrow")

    rotationEnded = true
end

function Update(dt)

    if (isStarting == true) then
        omozraAvailable = GetVariable("UI_GameState.lua", "omozra_available", INSPECTOR_VARIABLE_TYPE.INSPECTOR_BOOL)
        neralaAvailable = GetVariable("UI_GameState.lua", "nerala_available", INSPECTOR_VARIABLE_TYPE.INSPECTOR_BOOL)

        isStarting = false
    end

    target = Find("CameraReference"):GetTransform():GetPosition()

    if ((leftArrow:GetButton():IsPressed() or GetInput(14) == KEY_STATE.KEY_DOWN) and leftArrow.active == true and
        rotationEnded == true) then -- Q

        if (actualId == nil) then
            actualId = 0
        elseif (actualId == 0) then
            actualId = 1
        elseif (actualId == 1) then
            actualId = 2
        elseif (actualId == 2) then
            actualId = 0
        end

        str = "ActualPos" .. actualId .. "\n"
        Log(str)

        arrived = false
        remainingAngle = remainingAngle - angle
    end

    if ((rightArrow:GetButton():IsPressed() or GetInput(15) == KEY_STATE.KEY_DOWN) and rightArrow.active == true and
        rotationEnded == true) then -- E

        if (actualId == nil) then
            actualId = 0
        elseif (actualId == 0) then
            actualId = 2
        elseif (actualId == 1) then
            actualId = 0
        elseif (actualId == 2) then
            actualId = 1
        end

        str = "ActualPos" .. actualId .. "\n"
        Log(str)

        arrived = false
        remainingAngle = remainingAngle + angle
    end

    if (actualId == 0) then
        rightArrow:Active(omozraAvailable)
        leftArrow:Active(neralaAvailable)
    elseif (actualId == 1) then
        rightArrow:Active(true)
        leftArrow:Active(omozraAvailable)
    elseif (actualId == 2) then
        rightArrow:Active(neralaAvailable)
        leftArrow:Active(true)
    end

    local newRemainingAngle = MoveTowards(remainingAngle, 0, rotSpeed * dt)
    local delta = remainingAngle - newRemainingAngle
    remainingAngle = newRemainingAngle
    local newQuat = Quat.new(float3.new(0, 1, 0), delta * 0.0174533)
    offset = MulQuat(newQuat, offset)
    local newPos = float3.new(0, 0, 0)
    newPos.x = target.x + offset.x
    newPos.y = target.y + offset.y
    newPos.z = target.z + offset.z
    if delta == 0 then
        local finalPos = float3.new(0, 0, 0)
        finalPos.x = newPos.x + newZoomedPos.x
        finalPos.y = newPos.y + newZoomedPos.y
        finalPos.z = newPos.z + newZoomedPos.z
        componentTransform:SetPosition(finalPos)
        if (arrived == false) then
            DispatchGlobalEvent("TransitionedFromLastCharacter", {actualId})
            arrived = true
            rotationEnded = true
        end
    else
        componentTransform:SetPosition(newPos)
        rotationEnded = false
    end

    gameObject:GetCamera():LookAt(target)
end

function MoveTowards(current, target, maxDistanceDelta)
    local delta = target - current
    local sqrDelta = delta * delta

    local sqrDistance = maxDistanceDelta * maxDistanceDelta

    if sqrDelta > sqrDistance then
        local magnitude = math.sqrt(sqrDelta)
        if magnitude > 0.000006 then
            delta = delta * (maxDistanceDelta / magnitude)
            delta = delta + current
            return delta
        else
            return current
        end
    end
    return target
end

function EventHandler(key, fields)
    if (key == "Mosquito_Spawn") then
        mosquito = fields[1]
    elseif (key == "Mosquito_Death") then
        mosquito = nil
    end
end

print("TransitionCamera.lua compiled successfully!")
