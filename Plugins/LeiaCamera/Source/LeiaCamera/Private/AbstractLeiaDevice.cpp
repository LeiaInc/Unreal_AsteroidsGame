// Fill out your copyright notice in the Description page of Project Settings.


#include "AbstractLeiaDevice.h"

void AbstractLeiaDevice::SetProfileStubName(FString name)
{
	
}

void AbstractLeiaDevice::SetBacklightMode(BacklightMode modeId)
{
	
}

void AbstractLeiaDevice::SetBacklightMode(BacklightMode modeId, int delay)
{
	
}

BacklightMode AbstractLeiaDevice::GetBacklightMode()
{
	return BacklightMode::MODE_2D;
}

FString AbstractLeiaDevice::GetSensors()
{
	return "";
}

bool AbstractLeiaDevice::IsSensorsAvailable()
{
	return false;
}

bool AbstractLeiaDevice::IsConnected()
{
	return false;
}

void AbstractLeiaDevice::CalibrateSensors()
{
	
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfig()
{
#if WITH_EDITOR
	if (OverrideMode == EViewOverrideMode::EightView)
	{
		return Get8VDisplayConfig();
	}
	else if (OverrideMode == EViewOverrideMode::FourView)
	{
		return Get4VDisplayConfig();
	}
#endif
	return FDisplayConfig();
}

#if WITH_EDITOR

void AbstractLeiaDevice::SetOverride(EViewOverrideMode overrideMode)
{
	this->OverrideMode = overrideMode;
}

FDisplayConfig AbstractLeiaDevice::Get8VDisplayConfig() const
{
	FDisplayConfig displayConfig;

	displayConfig.panelResolution[0] = 3840;
	displayConfig.panelResolution[1] = 2160;

	displayConfig.numViews[0] = 8;
	displayConfig.numViews[1] = 1;

	displayConfig.viewResolution[0] = 1280;
	displayConfig.viewResolution[1] = 720;

	displayConfig.sharpeningKernelY[0] = 4;
	displayConfig.sharpeningKernelY[1] = 0.1f;
	displayConfig.sharpeningKernelY[2] = 0.02999999f;
	displayConfig.sharpeningKernelY[3] = 0.02999999f;
	displayConfig.sharpeningKernelY[4] = 0.02999999f;

	displayConfig.interlacingMatrixLandscape = { {1, 0, 0, 0}, {0, 1, 0.00139, 0}, {0, 0, 1, 0}, {1440.0, 270.0, 0.375, 0} };
	displayConfig.interlacingMatrixLandscape180 = { {1, 0, 0, 0}, {0, 1, 0.00139, 0}, {0, 0, 1, 0}, {1440.0, 270.0, 0.375, 0} };
	displayConfig.interlacingMatrixPortrait = { {1, 0, 0, 0}, {0, 1, 0.00139, 0}, {0, 0, 1, 0}, {1440.0, 270.0, 0.375, 0} };
	displayConfig.interlacingMatrixPortrait180 = { {1, 0, 0, 0}, {0, 1, 0.00139, 0}, {0, 0, 1, 0}, {1440.0, 270.0, 0.375, 0} };

	return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::Get4VDisplayConfig() const
{
	return FDisplayConfig();
}
#endif // WITH_EDITOR