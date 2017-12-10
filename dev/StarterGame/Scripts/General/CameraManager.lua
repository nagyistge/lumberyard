
local cameramanager =
{
	Properties =
	{
		InitialCamera = { default = EntityId(), },
		InitialCameraTag = { default = "PlayerCamera" },
		PlayerCameraTag = { default = "PlayerCamera" },
		FlyCamTag = { default = "FlyCam" },
		
		ActiveCamTag = { default = "ActiveCamera", description = "The tag that's used and applied at runtime to identify the active camera." },
	},
}

function cameramanager:OnActivate()

	self.activateEventId = GameplayNotificationId(self.entityId, "ActivateCameraEvent", "float");
	self.activateHandler = GameplayNotificationBus.Connect(self, self.activateEventId);
	
	self.pushCameraSettingsEventId = GameplayNotificationId(self.entityId, "PushCameraSettings", "float");
	self.pushCameraSettingsHandler = GameplayNotificationBus.Connect(self, self.pushCameraSettingsEventId);
	self.popCameraSettingsEventId = GameplayNotificationId(self.entityId, "PopCameraSettings", "float");
	self.popCameraSettingsHandler = GameplayNotificationBus.Connect(self, self.popCameraSettingsEventId);

	self.toggleFlyCamEventId = GameplayNotificationId(self.entityId, "ToggleFlyCam", "float");
	self.toggleFlyCamHandler = GameplayNotificationBus.Connect(self, self.toggleFlyCamEventId);
	
	self.activeCamTagCrc = Crc32(self.Properties.ActiveCamTag);
	
	-- We can't set the initial camera here because there's no gaurantee that a camera won't
	-- be initialised AFTER the camera manager and override what we've set here. As a result
	-- we need to join the tick bus so we can set the initial camera on the first tick and
	-- then disconnect from the tick bus (since there shouldn't be anything that we need to
	-- do every frame).
	self.tickHandler = TickBus.Connect(self);
	
end

function cameramanager:OnDeactivate()
	
	if (self.cameraSettingsHandler ~= nil) then
		self.cameraSettingsHandler:Disconnect();
		self.cameraSettingsHandler = nil;
	end
	if (self.activateHandler ~= nil) then
		self.activateHandler:Disconnect();
		self.activateHandler = nil;
	end

end
	
function cameramanager:OnTick(deltaTime, timePoint)
	
	local startingCam = self.Properties.InitialCamera;
	if (not startingCam:IsValid()) then
		-- TODO: Change this to a 'Warning()'.
		Debug.Log("Camera Manager: No initial camera has been set - finding and using the camera tagged with " .. self.Properties.InitialCameraTag .. " instead.");
		startingCam = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.InitialCameraTag));
		
		-- If we couldn't find a camera with the fallback tag then we can't reliably assume
		-- which camera became the active one.
		if (not startingCam:IsValid()) then
			-- TODO: Change this to an 'Assert()'.
			Debug.Log("Camera Manager: No initial camera could be found.");
		end
	end

	self.playerCameraId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.PlayerCameraTag));
	self.playerCameraSettingsEventId = GameplayNotificationId(self.playerCameraId, "CameraSettings", "float");

	self.flyCamId = TagGlobalRequestBus.Event.RequestTaggedEntities(Crc32(self.Properties.FlyCamTag));
	self.flyCamEnableEventId = GameplayNotificationId(self.flyCamId, "EnableFlyCam", "float");
	self.flyCamActive = false;
	
	self.currentSettingsName = "";
	self:UpdatePlayerCameraSettings();

	self:ActivateCam(startingCam);
	
	-- This function is a work-around because we can't disconnect the tick handler directly
	-- inside the 'OnTick()' function, but we can inside another function; apparently, even
	-- if that function is called FROM the 'OnTick()' function.
	self:StopTicking();

end

function cameramanager:StopTicking()

	self.tickHandler:Disconnect();
	self.tickHandler = nil;
	
end

function cameramanager:ToggleFlyCam()
	if (self.flyCamActive) then
		-- disable fly cam
		GameplayNotificationBus.Event.OnEventBegin(self.flyCamEnableEventId, false);
		self.flyCamActive = false;

		-- activate previous cam
		self:ActivateCam(self.previousActiveCamera);
	else
		-- remember current active cam
		self.previousActiveCamera = self.activeCamera;
		
		-- move fly cam to where active cam is
		local cameraTm = TransformBus.Event.GetWorldTM(self.previousActiveCamera);
		TransformBus.Event.SetWorldTM(self.flyCamId, cameraTm);

		-- enable fly cam
		GameplayNotificationBus.Event.OnEventBegin(self.flyCamEnableEventId, true);
		
		-- activate fly cam
		self:ActivateCam(self.flyCamId);
		
		self.flyCamActive = true;
	end

end

function cameramanager:ActivateCam(camToActivate)
	if (self.flyCamActive) then
		-- activating a camera while in fly cam means we need to change the camera to which we return once we exit fly cam
		self.previousActiveCamera = camToActivate;
	else
		local c1Tags = TagComponentRequestBus.Event.AddTag(camToActivate, self.activeCamTagCrc);

		local camToDeactivate = TagGlobalRequestBus.Event.RequestTaggedEntities(self.activeCamTagCrc);
		if (camToDeactivate ~= nil) then
			local c2Tags = TagComponentRequestBus.Event.RemoveTag(camToDeactivate, self.activeCamTagCrc);
		end
		self.activeCamera = camToActivate;
		--Debug.Log("Activating camera: " .. tostring(camToActivate));
		CameraRequestBus.Event.MakeActiveView(camToActivate);
	end	
end

function cameramanager:UpdatePlayerCameraSettings()
	local activeSettings = CameraSettingsComponentRequestsBus.Event.GetActiveSettings(self.entityId);
	if (self.currentSettingsName == "" or self.currentSettingsName ~= activeSettings.Name) then
		-- camera mode has changed, tell the player camera to go to new mode
		self.currentSettingsName = activeSettings.Name;
		GameplayNotificationBus.Event.OnEventBegin(self.playerCameraSettingsEventId, activeSettings);
	end
end

function cameramanager:OnEventBegin(value)
	local settingsChanged = false;

	if (GameplayNotificationBus.GetCurrentBusId() == self.activateEventId) then
		self:ActivateCam(value);
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.pushCameraSettingsEventId) then
		CameraSettingsComponentRequestsBus.Event.PushSettings(self.entityId, value.name, value.entityId);
		settingsChanged = true;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.popCameraSettingsEventId) then
		CameraSettingsComponentRequestsBus.Event.PopSettings(self.entityId, value.name, value.entityId);
		settingsChanged = true;
	elseif (GameplayNotificationBus.GetCurrentBusId() == self.toggleFlyCamEventId) then
		self:ToggleFlyCam();
	end

	if (settingsChanged) then
		self:UpdatePlayerCameraSettings();
	end
		
end

return cameramanager;