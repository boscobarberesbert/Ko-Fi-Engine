------------------- Variables --------------------

characterID = 1
target = nil

Action = {
	IDLE = 1,
	ATTACKING = 2,
	WALKING = 3,
	RUNNING = 4,
	AIMING_KNIFE = 5,
	AIMING_ULTIMATE = 6,
}

currentAction = Action.IDLE
speed = 500  -- consider Start()
maxKnives = 2
knifeCount = maxKnives
ultimateRange = 150.0
ultimateRangeExtension = ultimateRange * 0.66

local speedIVT = INSPECTOR_VARIABLE_TYPE.INSPECTOR_INT			-- IVT == Inspector Variable Type
local speedIV = InspectorVariable.new("speed", speedIVT, speed)
NewVariable(speedIV)

local maxKnivesIVT = INSPECTOR_VARIABLE_TYPE.INSPECTOR_INT
local maxKnivesIV = InspectorVariable.new("maxKnives", maxKnivesIVT, maxKnives)
NewVariable(maxKnivesIV)

local ultimateRangeIVT = INSPECTOR_VARIABLE_TYPE.INSPECTOR_FLOAT
local ultimateRangeIV = InspectorVariable.new("ultimateRange", ultimateRangeIVT, ultimateRange)
NewVariable(ultimateRangeIV)

local characterIDIVT = INSPECTOR_VARIABLE_TYPE.INSPECTOR_INT
local characterIDIV = InspectorVariable.new("characterID", characterIDIVT, characterID)
NewVariable(characterIDIV)

componentAnimator = gameObject:GetComponentAnimator()
if (componentAnimator ~= nil) then
	componentAnimator:SetSelectedClip("Idle")
end
animationDuration = 0.8
animationTimer = 0.0
isAttacking = false -- This should go

componentSwitch = gameObject:GetAudioSwitch()
componentRigidBody = gameObject:GetRigidBody()

mouseParticles = Find("Mouse Particles")
if (mouseParticles ~= nil) then
	mouseParticles:GetComponentParticle():StopParticleSpawn()
end

doubleClickDuration = 0.5
doubleClickTimer = 0.0
isDoubleClicking = false

-------------------- Methods ---------------------

-- Called each loop iteration
function Update(dt)

	-- Timers & Helpers
	if (isAttacking == true and componentAnimator ~= nil) then
		animationTimer = animationTimer + dt
		if (animationTimer >= animationDuration) then
			componentAnimator:SetSelectedClip("Idle")
			isAttacking = false
			animationTimer = 0.0
		end
	end
	if (isDoubleClicking == true) then
		if (doubleClickTimer < doubleClickDuration) then
			doubleClickTimer = doubleClickTimer + dt
		else 
			isDoubleClicking = false
			doubleClickTimer = 0.0
		end
	end
	if (mouseParticles ~= nil) then
		mouseParticles:GetComponentParticle():StopParticleSpawn()
	end
	if (invisibilityDuration ~= nil) then
		invisibilityTimer = invisibilityTimer + dt
		if (invisibilityTimer >= invisibilityDuration) then
			invisibilityDuration = nil
			gameObject:GetComponentMesh():Enable() -- Useless ???
		else
			componentTransform:SetPosition(float3.new(componentTransform:GetPosition().x, 1000.0, componentTransform:GetPosition().z))
		end
	end

	-- Actions
	if (destination ~= nil)	then
		MoveToDestination(dt)
	end

	--Gather Inputs
	if (IsSelected() == true) then 

		if (GetInput(1) == KEY_STATE.KEY_DOWN) then -- Left Click
			if (currentAction == Action.AIMING_KNIFE) then -- Fire Knife (infinite range for now)
				if (knifeCount > 0) then
					target = GetGameObjectHovered()
					if (target.tag == Tag.ENEMY) then
						Fire()
					end
				else
					print("Out of ammo")
				end
			elseif (currentAction == Action.AIMING_ULTIMATE) then -- Cast Ultimate (infinite range for now)
				target = GetGameObjectHovered()
				mousePos = GetLastMouseClick()
				Ultimate(mousePos)
			end
		end

		if (GetInput(3) == KEY_STATE.KEY_DOWN) then -- Right Click
			goHit = GetGameObjectHovered()
			if (goHit ~= hit and goHit ~= gameObject) then
				destination = GetLastMouseClick()
				if (currentAction == Action.WALKING and isDoubleClicking == true) then
					currentAction = Action.RUNNING
				else
					currentAction = Action.WALKING
					isDoubleClicking = true
				end
				if (mouseParticles ~= nil) then
					mouseParticles:GetComponentParticle():ResumeParticleSpawn()
					mouseParticles:GetTransform():SetPosition(destination)
				end
			end
		end

		if (GetInput(5) == KEY_STATE.KEY_DOWN) then -- H
			currentAction = Action.IDLE
		end
		if (GetInput(6) == KEY_STATE.KEY_DOWN) then -- K
			currentAction = Action.AIMING_KNIFE
		end	
		if (GetInput(4) == KEY_STATE.KEY_DOWN) then -- SPACE
			currentAction = Action.AIMING_ULTIMATE
		end
		if (GetInput(8) == KEY_STATE.KEY_DOWN) then -- X -> Toggle crouch
			if (currentAction == Action.CROUCHING) then
				currentAction = Action.WALKING
				if (componentSwitch ~= nil) then
					componentSwitch:StopTrack(2)
					componentSwitch:PlayAudio(1)
				end
			else
				currentAction = Action.CROUCHING
				if (componentSwitch ~= nil) then
					componentSwitch:StopTrack(1)
					componentSwitch:PlayAudio(2)
				end
			end
		end
		if (GetInput(10) == KEY_STATE.KEY_DOWN) then -- R -> For debugging purposes only
			Reload()
		end
	end
end

-- Move to destination
function MoveToDestination(dt)

	local targetPos2D = { destination.x, destination.z }
	local pos2D = { componentTransform:GetPosition().x, componentTransform:GetPosition().z }
	local d = Distance(pos2D, targetPos2D)
	local vec2 = { targetPos2D[1] - pos2D[1], targetPos2D[2] - pos2D[2] }

	if (d > 5.0) then

		if (currentAction ~= Action.IDLE) then
			if (currentAction == Action.WALKING) then
				if (componentSwitch ~= nil) then
					componentSwitch:PlayTrack(1)
				end
				if (componentAnimator ~= nil) then
					componentAnimator:SetSelectedClip("Walk")
				end
			elseif (currentAction == Action.CROUCHING) then
				if (componentSwitch ~= nil) then
					componentSwitch:PlayTrack(2)
				end
				if (componentAnimator ~= nil) then
					componentAnimator:SetSelectedClip("Crouch")
				end
			elseif (currentAction == Action.RUNNING) then
				if (componentSwitch ~= nil) then
					componentSwitch:PlayTrack(1)
				end
				if (componentAnimator ~= nil) then
					componentAnimator:SetSelectedClip("Run")
				end
			end
		end
		
		-- Adapt speed
		local s = speed
		if (currentAction == Action.CROUCHING) then
			s = speed * 0.66
		elseif (currentAction == Action.RUNNING) then
			s = speed * 1.5
		end

		-- Movement
		vec2 = Normalize(vec2, d)
		if (componentRigidBody ~= nil) then
			componentRigidBody:Set2DVelocity(float2.new(vec2[1] * s * dt, vec2[2] * s * dt))
		end

		-- Rotation
		local rad = math.acos(vec2[2])
		if(vec2[1] < 0)	then
			rad = rad * (-1)
		end
		componentTransform:SetRotation(float3.new(componentTransform:GetRotation().x, componentTransform:GetRotation().y, rad))
	else
		
		destination = nil
		if (componentRigidBody ~= nil) then
			componentRigidBody:Set2DVelocity(float2.new(0,0))
		end
		if (componentSwitch ~= nil) then
			componentSwitch:StopTrack(1)
		end
		if (componentAnimator ~= nil) then
			componentAnimator:PlayAnimation("Idle")
		end
	end
	-- Add ChangeAnimation() to check the speed of the rigid body
end

function Reload() -- For debugging purposes only
	knifeCount = maxKnives
end

function IsSelected()
	
	id = GetVariable("GameState.lua", "characterSelected", INSPECTOR_VARIABLE_TYPE.INSPECTOR_INT)

	if (id == characterID) then	
		return true
	else 
		return false
	end
end

function Fire()

	CreateGameObject("Knife")
	knifeCount = knifeCount - 1
	if (componentSwitch ~= nil) then
		componentSwitch:PlayTrack(0)
	end
end

function Ultimate(mousePos)

	-- Get all enemies in range of the Mouse
	enemiesInRange = {}
	enemies = GetObjectsByTag(Tag.ENEMY)
	for i = 1, #enemies do
		if (Distance3D(enemies[i]:GetTransform():GetPosition(), mousePos) <= ultimateRange) then
			enemiesInRange[#enemiesInRange + 1] = enemies[i]
		end
	end

	-- If there are none, the ability isn't casted
	if (#enemiesInRange <= 0) then
		return
	end

	-- Check them all for adjacent enemies, different than the ones on the list and add them if there are anyway
	for i = 1, #enemiesInRange do

		for j = 1, #enemies do
			if (enemiesInRange[i] ~= enemies[j]) then
				if (Distance3D(enemiesInRange[i]:GetTransform():GetPosition(), enemies[j]:GetTransform():GetPosition()) <= ultimateRangeExtension) then
					
					isAlreadyInArray = false
					for k = 1, #enemiesInRange do
						if (enemiesInRange[k] == enemies[j]) then
							isAlreadyInArray = true
						end
					end

					if (not isAlreadyInArray) then 
						enemiesInRange[#enemiesInRange + 1] = enemies[j]
					end
				end
			end
		end		
	end

	deathMarkTimer = 0.2
	-- Set IN ORDER the death mark
	for i = 1, #enemiesInRange do
		deathMarkTimer = deathMarkTimer + 0.1
		SetLuaVariableFromGameObject(enemiesInRange[i].name, "deathMark", deathMarkTimer)
	end
	
	Disapear(deathMarkTimer)

	-- Set timer equal to the longest dath mark timer to reappear
end

function Disapear(duration)
	gameObject:GetComponentMesh():Disable() -- Useless ???
	componentTransform:SetPosition(float3.new(componentTransform:GetPosition().x, 1000.0, componentTransform:GetPosition().z))

	invisibilityTimer = 0
	invisibilityDuration = duration
end

----------------- Math Functions -----------------

function Normalize(vec, distance)
    
	vec[1] = vec[1] / distance
	vec[2] = vec[2] / distance

	return vec
end

function Distance(a, b)

    local dx, dy = a[1] - b[1], a[2] - b[2]
    return math.sqrt(dx * dx + dy * dy)

end

function Distance3D(a, b)

    diff = { x = b.x - a.x, y = b.y - a.y, z = b.z - a.z }
    return math.sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z)
end

--------------------------------------------------

print("Zhib.lua compiled succesfully")

-------- Scraps --------
--local components = gameObject:GetComponents()
--print(components[3].type) -- return it as an int, can't associate back to enum (I don't know how to anyway)

--GameState = require "Assets.Scripts.GameState"
--GameState:Update(1)
--print(GameState:GetGameState())