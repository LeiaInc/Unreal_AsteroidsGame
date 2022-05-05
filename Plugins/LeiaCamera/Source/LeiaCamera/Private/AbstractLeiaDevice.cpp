/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

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
	if (OverrideMode != EViewOverrideMode::None) {
		return BacklightMode::MODE_3D;
	}
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
	switch (OverrideMode)
	{
	case EViewOverrideMode::AndroidPegasus_12p5_8V:
		return GetDisplayConfigAndroidPegasus12p5_8V();
		break;
	case EViewOverrideMode::AndroidPegasus_12p3_8V:
		return GetDisplayConfigAndroidPegasus12p3_8V();
		break;
	case EViewOverrideMode::Windows_12p5_8V:
		return GetDisplayConfigWindows12p5_8V();
		break;
	case EViewOverrideMode::Windows_15p6_12V:
		return GetDisplayConfigWindows15p6_12V();
		break;
	case EViewOverrideMode::Windows_15p6_13V:
		return GetDisplayConfigWindows15p6_13V();
		break;
	case EViewOverrideMode::LumePad:
		return GetDisplayConfigLumePad();
		break;
	case EViewOverrideMode::None:
		return FDisplayConfig();
		break;
	default:
		return FDisplayConfig();
		break;
	}
}

void AbstractLeiaDevice::SetOverride(EViewOverrideMode overrideMode)
{
	this->OverrideMode = overrideMode;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfigWindows12p5_8V() const
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

	displayConfig.gamma = 2.2f;

	FMatrix interlacingMatrix = { 
		{1, 0, 0, 0}, 
		{0, 1, 0.00139, 0}, 
		{0, 0, 1, 0}, 
		{1440.0, 270.0, 0.375, 0} };

	displayConfig.interlacingMatrixLandscape = interlacingMatrix;
	displayConfig.interlacingMatrixLandscape180 = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait180 = interlacingMatrix;

	return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfigAndroidPegasus12p3_8V() const
{
	FDisplayConfig displayConfig;

	displayConfig.panelResolution[0] = 2304;
	displayConfig.panelResolution[1] = 900;

	displayConfig.numViews[0] = 8;
	displayConfig.numViews[1] = 1;

	displayConfig.viewResolution[0] = 1050;
	displayConfig.viewResolution[1] = 394;

	displayConfig.sharpeningKernelY[0] = 4;
	displayConfig.sharpeningKernelY[1] = 0.1f;
	displayConfig.sharpeningKernelY[2] = 0.02999999f;
	displayConfig.sharpeningKernelY[3] = 0.02999999f;
	displayConfig.sharpeningKernelY[4] = 0.02999999f;

	FMatrix interlacingMatrix = { 
		{1, 0, 0, 0}, 
		{0, 1, 0.0033, 0}, 
		{0, 0, 1, 0}, 
		{864.0, 112.5, 0.375, 0} };

	displayConfig.interlacingMatrixLandscape = interlacingMatrix;
	displayConfig.interlacingMatrixLandscape180 = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait180 = interlacingMatrix;

	return displayConfig;
}
FDisplayConfig AbstractLeiaDevice::GetDisplayConfigAndroidPegasus12p5_8V() const
{
	FDisplayConfig displayConfig;

	displayConfig.panelResolution[0] = 3744;
	displayConfig.panelResolution[1] = 2160;

	displayConfig.numViews[0] = 8;
	displayConfig.numViews[1] = 1;

	displayConfig.viewResolution[0] = 1248;
	displayConfig.viewResolution[1] = 720;

	displayConfig.sharpeningKernelX[0] = 4;
	displayConfig.sharpeningKernelX[1] = 0.1f;
	displayConfig.sharpeningKernelX[2] = 0.02999999f;
	displayConfig.sharpeningKernelX[3] = 0.02999999f;
	displayConfig.sharpeningKernelX[4] = 0.02999999f;

	FMatrix interlacingMatrix = { 
		{1, 0, 0, 0}, 
		{0, 1, 0.00139, 0}, 
		{0, 0, 1, 0}, 
		{1404.0, 270.0, 0.375, 0} };

	displayConfig.interlacingMatrixLandscape = interlacingMatrix;
	displayConfig.interlacingMatrixLandscape180 = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait180 = interlacingMatrix;

	return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfigWindows15p6_12V() const
{
	FDisplayConfig displayConfig;

	displayConfig.panelResolution[0] = 3840;
	displayConfig.panelResolution[1] = 2160;

	displayConfig.numViews[0] = 12;
	displayConfig.numViews[1] = 1;

	displayConfig.alignmentOffset[0] = 7;
	displayConfig.alignmentOffset[1] = 0;

	displayConfig.viewResolution[0] = 1280;
	displayConfig.viewResolution[1] = 720;

	displayConfig.sharpeningKernelX[0] = 4;
	displayConfig.sharpeningKernelX[1] = 0.1f;
	displayConfig.sharpeningKernelX[2] = 0.02999999f;
	displayConfig.sharpeningKernelX[3] = 0.02999999f;
	displayConfig.sharpeningKernelX[4] = 0.02999999f;

	FMatrix interlacingMatrix = {
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{960.0, 180.0, .25, 1.0} };

	displayConfig.interlacingMatrixLandscape = interlacingMatrix;
	displayConfig.interlacingMatrixLandscape180 = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait180 = interlacingMatrix;

	return displayConfig;
}
FDisplayConfig AbstractLeiaDevice::GetDisplayConfigWindows15p6_13V() const
{
	FDisplayConfig displayConfig;

	displayConfig.panelResolution[0] = 3840;
	displayConfig.panelResolution[1] = 2160;

	displayConfig.numViews[0] = 13;
	displayConfig.numViews[1] = 1;

	displayConfig.viewResolution[0] = 1280;
	displayConfig.viewResolution[1] = 720;

	displayConfig.sharpeningKernelX[0] = 4;
	displayConfig.sharpeningKernelX[1] = 0.1f;
	displayConfig.sharpeningKernelX[2] = 0.02999999f;
	displayConfig.sharpeningKernelX[3] = 0.02999999f;
	displayConfig.sharpeningKernelX[4] = 0.02999999f;

	FMatrix interlacingMatrix = {
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{886.1538696289063, 166.15383911132813, 0.23076923191547395, 1.0} };

	displayConfig.interlacingMatrixLandscape = interlacingMatrix;
	displayConfig.interlacingMatrixLandscape180 = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait = interlacingMatrix;
	displayConfig.interlacingMatrixPortrait180 = interlacingMatrix;

	return displayConfig;
}
FDisplayConfig AbstractLeiaDevice::GetDisplayConfigLumePad() const
{
	return FDisplayConfig();
}
