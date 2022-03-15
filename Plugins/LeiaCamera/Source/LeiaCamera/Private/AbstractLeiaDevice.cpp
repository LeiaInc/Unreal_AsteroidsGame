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

    displayConfig.dotPitchInMM[0] = 0.089640;
    displayConfig.dotPitchInMM[1] = 0.056;
    
    displayConfig.panelResolution[0] = 3840;
	displayConfig.panelResolution[1] = 2160;

	displayConfig.numViews[0] = 8;
	displayConfig.numViews[1] = 1;

    displayConfig.sharpeningKernelXSize = 9;
    displayConfig.sharpeningKernelX[0] = 0.096;
    displayConfig.sharpeningKernelX[1] = 0.009;
    displayConfig.sharpeningKernelX[2] = 0.017;
    displayConfig.sharpeningKernelX[3] = 0.008;
    displayConfig.sharpeningKernelX[4] = 0.008;
    displayConfig.sharpeningKernelX[5] = 0.004;
    displayConfig.sharpeningKernelX[6] = 0.0;
    displayConfig.sharpeningKernelX[7] = 0.0;
    displayConfig.sharpeningKernelX[8] = 0.0;

    displayConfig.sharpeningKernelYSize = 9;
    displayConfig.sharpeningKernelY[0] = 1643.349976;
    displayConfig.sharpeningKernelY[1] = 0.81116;
    displayConfig.sharpeningKernelY[2] = -23.274;
    displayConfig.sharpeningKernelY[3] = 166.24005;
    displayConfig.sharpeningKernelY[4] = 1.99912;
    displayConfig.sharpeningKernelY[5] = 0.0;
    displayConfig.sharpeningKernelY[6] = 0.0;
    displayConfig.sharpeningKernelY[7] = 0.0;
    displayConfig.sharpeningKernelY[8] = 0.0;

    displayConfig.viewResolution[0] = 1280;
    displayConfig.viewResolution[1] = 720;

    displayConfig.displaySizeInMm[0] = 0;
    displayConfig.displaySizeInMm[1] = 0;

    displayConfig.act_gamma = 2.0;
    displayConfig.act_beta = 1.4;
    displayConfig.systemDisparityPercent = 0.0125f;
    displayConfig.systemDisparityPixels = 8.0f;
    displayConfig.cameraCenterX = -25;
    displayConfig.cameraCenterY = 185;
    displayConfig.centerViewNumber = 0.0f;
    displayConfig.n = 1.47f;
    displayConfig.theta = 0.0f;
    displayConfig.s = 11.0f;
    displayConfig.d_over_n = 0.5f;
    displayConfig.p_over_du = 3.0f;
    displayConfig.p_over_dv = 1.0f;
    displayConfig.colorInversion = false;
    displayConfig.colorSlant = 0;
    displayConfig.convergenceDistance = 500.0f;

	return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfigAndroidPegasus12p3_8V() const
{
	FDisplayConfig displayConfig;

    displayConfig.dotPitchInMM[0] = 0.089640;
    displayConfig.dotPitchInMM[1] = 0.056;

	displayConfig.panelResolution[0] = 2304;
	displayConfig.panelResolution[1] = 900;

	displayConfig.numViews[0] = 8;
	displayConfig.numViews[1] = 1;

    displayConfig.sharpeningKernelXSize = 9;
    displayConfig.sharpeningKernelX[0] = 0.096;
    displayConfig.sharpeningKernelX[1] = 0.009;
    displayConfig.sharpeningKernelX[2] = 0.017;
    displayConfig.sharpeningKernelX[3] = 0.008;
    displayConfig.sharpeningKernelX[4] = 0.008;
    displayConfig.sharpeningKernelX[5] = 0.004;
    displayConfig.sharpeningKernelX[6] = 0.0;
    displayConfig.sharpeningKernelX[7] = 0.0;
    displayConfig.sharpeningKernelX[8] = 0.0;

    displayConfig.sharpeningKernelYSize = 9;
    displayConfig.sharpeningKernelY[0] = 1643.349976;
    displayConfig.sharpeningKernelY[1] = 0.81116;
    displayConfig.sharpeningKernelY[2] = -23.274;
    displayConfig.sharpeningKernelY[3] = 166.24005;
    displayConfig.sharpeningKernelY[4] = 1.99912;
    displayConfig.sharpeningKernelY[5] = 0.0;
    displayConfig.sharpeningKernelY[6] = 0.0;
    displayConfig.sharpeningKernelY[7] = 0.0;
    displayConfig.sharpeningKernelY[8] = 0.0;

    displayConfig.viewResolution[0] = 1050;
    displayConfig.viewResolution[1] = 394;

    displayConfig.displaySizeInMm[0] = 0;
    displayConfig.displaySizeInMm[1] = 0;
    
    displayConfig.act_gamma = 2.0;
    displayConfig.act_beta = 1.4;
    displayConfig.systemDisparityPercent = 0.0125f;
    displayConfig.systemDisparityPixels = 8.0f;
    displayConfig.cameraCenterX = -25;
    displayConfig.cameraCenterY = 185;
    displayConfig.centerViewNumber = 0.0f;
    displayConfig.n = 1.47f;
    displayConfig.theta = 0.0f;
    displayConfig.s = 11.0f;
    displayConfig.d_over_n = 0.5f;
    displayConfig.p_over_du = 3.0f;
    displayConfig.p_over_dv = 1.0f;
    displayConfig.colorInversion = false;
    displayConfig.colorSlant = 0;
    displayConfig.convergenceDistance = 500.0f;

	return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfigAndroidPegasus12p5_8V() const
{
	FDisplayConfig displayConfig;

    displayConfig.dotPitchInMM[0] = 0.089640;
    displayConfig.dotPitchInMM[1] = 0.056;
    
    displayConfig.panelResolution[0] = 3744;
	displayConfig.panelResolution[1] = 2160;

	displayConfig.numViews[0] = 8;
	displayConfig.numViews[1] = 1;
	
    displayConfig.sharpeningKernelXSize = 9;
    displayConfig.sharpeningKernelX[0] = 0.096;
    displayConfig.sharpeningKernelX[1] = 0.009;
    displayConfig.sharpeningKernelX[2] = 0.017;
    displayConfig.sharpeningKernelX[3] = 0.008;
    displayConfig.sharpeningKernelX[4] = 0.008;
    displayConfig.sharpeningKernelX[5] = 0.004;
    displayConfig.sharpeningKernelX[6] = 0.0;
    displayConfig.sharpeningKernelX[7] = 0.0;
    displayConfig.sharpeningKernelX[8] = 0.0;

    displayConfig.sharpeningKernelYSize = 9;
    displayConfig.sharpeningKernelY[0] = 1643.349976;
    displayConfig.sharpeningKernelY[1] = 0.81116;
    displayConfig.sharpeningKernelY[2] = -23.274;
    displayConfig.sharpeningKernelY[3] = 166.24005;
    displayConfig.sharpeningKernelY[4] = 1.99912;
    displayConfig.sharpeningKernelY[5] = 0.0;
    displayConfig.sharpeningKernelY[6] = 0.0;
    displayConfig.sharpeningKernelY[7] = 0.0;
    displayConfig.sharpeningKernelY[8] = 0.0;

    displayConfig.viewResolution[0] = 1248;
    displayConfig.viewResolution[1] = 720;

    displayConfig.displaySizeInMm[0] = 0;
    displayConfig.displaySizeInMm[1] = 0;

    displayConfig.act_gamma = 2.0;
    displayConfig.act_beta = 1.4;
    displayConfig.systemDisparityPercent = 0.0125f;
    displayConfig.systemDisparityPixels = 8.0f;
    displayConfig.cameraCenterX = -25;
    displayConfig.cameraCenterY = 185;
    displayConfig.centerViewNumber = 0.0f;
    displayConfig.n = 1.47f;
    displayConfig.theta = 0.0f;
    displayConfig.s = 11.0f;
    displayConfig.d_over_n = 0.5f;
    displayConfig.p_over_du = 3.0f;
    displayConfig.p_over_dv = 1.0f;
    displayConfig.colorInversion = false;
    displayConfig.colorSlant = 0;
    displayConfig.convergenceDistance = 500.0f;

	return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfigWindows15p6_12V() const
{
	FDisplayConfig displayConfig;

    displayConfig.dotPitchInMM[0] = 0.08964;
    displayConfig.dotPitchInMM[1] = 0.056;

	displayConfig.panelResolution[0] = 3840;
	displayConfig.panelResolution[1] = 2160;

	displayConfig.numViews[0] = 12;
	displayConfig.numViews[1] = 1;

    displayConfig.sharpeningKernelXSize = 9;
    displayConfig.sharpeningKernelX[0] = 0.096;
    displayConfig.sharpeningKernelX[1] = 0.009;
    displayConfig.sharpeningKernelX[2] = 0.017;
    displayConfig.sharpeningKernelX[3] = 0.008;
    displayConfig.sharpeningKernelX[4] = 0.008;
    displayConfig.sharpeningKernelX[5] = 0.004;
    displayConfig.sharpeningKernelX[6] = 0.0;
    displayConfig.sharpeningKernelX[7] = 0.0;
    displayConfig.sharpeningKernelX[8] = 0.0;

    displayConfig.sharpeningKernelYSize = 9;
    displayConfig.sharpeningKernelY[0] = 1643.349976;
    displayConfig.sharpeningKernelY[1] = 0.81116;
    displayConfig.sharpeningKernelY[2] = -23.274;
    displayConfig.sharpeningKernelY[3] = 166.24005;
    displayConfig.sharpeningKernelY[4] = 1.99912;
    displayConfig.sharpeningKernelY[5] = 0.0;
    displayConfig.sharpeningKernelY[6] = 0.0;
    displayConfig.sharpeningKernelY[7] = 0.0;
    displayConfig.sharpeningKernelY[8] = 0.0;

    displayConfig.viewResolution[0] = 1280;
    displayConfig.viewResolution[1] = 720;

    displayConfig.displaySizeInMm[0] = 0;
    displayConfig.displaySizeInMm[1] = 0;

    displayConfig.act_gamma = 1.99562001;
    displayConfig.act_beta = 1.99912;
    displayConfig.systemDisparityPercent = 0.0125f;
    displayConfig.systemDisparityPixels = 8.0f;
    displayConfig.cameraCenterX = -23.2740002;
    displayConfig.cameraCenterY = 166.240005;
    displayConfig.centerViewNumber = -3.31475997;
    displayConfig.n = 1.64335001f;
    displayConfig.theta = 0.811160028f;
    displayConfig.s = 9.02087975f;
    displayConfig.d_over_n = 0.408390611f;
    displayConfig.p_over_du = 3.0f;
    displayConfig.p_over_dv = 1.0f;
    displayConfig.colorInversion = false;
    displayConfig.colorSlant = 0;
    displayConfig.convergenceDistance = 500.0f;
    
	return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfigWindows15p6_13V() const
{
	FDisplayConfig displayConfig;

    displayConfig.dotPitchInMM[0] = 0.089640;
    displayConfig.dotPitchInMM[1] = 0.056;
    
    displayConfig.panelResolution[0] = 3840;
	displayConfig.panelResolution[1] = 2160;

	displayConfig.numViews[0] = 13;
	displayConfig.numViews[1] = 1;

    displayConfig.sharpeningKernelXSize = 9;
    displayConfig.sharpeningKernelX[0] = 0.096;
    displayConfig.sharpeningKernelX[1] = 0.009;
    displayConfig.sharpeningKernelX[2] = 0.017;
    displayConfig.sharpeningKernelX[3] = 0.008;
    displayConfig.sharpeningKernelX[4] = 0.008;
    displayConfig.sharpeningKernelX[5] = 0.004;
    displayConfig.sharpeningKernelX[6] = 0.0;
    displayConfig.sharpeningKernelX[7] = 0.0;
    displayConfig.sharpeningKernelX[8] = 0.0;

    displayConfig.sharpeningKernelYSize = 9;
    displayConfig.sharpeningKernelY[0] = 1643.349976;
    displayConfig.sharpeningKernelY[1] = 0.81116;
    displayConfig.sharpeningKernelY[2] = -23.274;
    displayConfig.sharpeningKernelY[3] = 166.24005;
    displayConfig.sharpeningKernelY[4] = 1.99912;
    displayConfig.sharpeningKernelY[5] = 0.0;
    displayConfig.sharpeningKernelY[6] = 0.0;
    displayConfig.sharpeningKernelY[7] = 0.0;
    displayConfig.sharpeningKernelY[8] = 0.0;

    displayConfig.viewResolution[0] = 1280;
    displayConfig.viewResolution[1] = 720;

    displayConfig.displaySizeInMm[0] = 0;
    displayConfig.displaySizeInMm[1] = 0;

    displayConfig.act_gamma = 2.0;
    displayConfig.act_beta = 1.4;
    displayConfig.systemDisparityPercent = 0.0125f;
    displayConfig.systemDisparityPixels = 8.0f;
    displayConfig.cameraCenterX = -25;
    displayConfig.cameraCenterY = 185;
    displayConfig.centerViewNumber = 0.0f;
    displayConfig.n = 1.47f;
    displayConfig.theta = 0.0f;
    displayConfig.s = 11.0f;
    displayConfig.d_over_n = 0.5f;
    displayConfig.p_over_du = 3.0f;
    displayConfig.p_over_dv = 1.0f;
    displayConfig.colorInversion = false;
    displayConfig.colorSlant = 0;
    displayConfig.convergenceDistance = 500.0f;

	return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfigLumePad() const
{
    FDisplayConfig displayConfig;

    displayConfig.dotPitchInMM[0] = 0.089640;
    displayConfig.dotPitchInMM[1] = 0.056;

    displayConfig.panelResolution[0] = 2560;
    displayConfig.panelResolution[1] = 1600;

    displayConfig.numViews[0] = 4;
    displayConfig.numViews[1] = 1;

    displayConfig.sharpeningKernelXSize = 9;
    displayConfig.sharpeningKernelX[0] = 0.096;
    displayConfig.sharpeningKernelX[1] = 0.009;
    displayConfig.sharpeningKernelX[2] = 0.017;
    displayConfig.sharpeningKernelX[3] = 0.008;
    displayConfig.sharpeningKernelX[4] = 0.008;
    displayConfig.sharpeningKernelX[5] = 0.004;
    displayConfig.sharpeningKernelX[6] = 0.0;
    displayConfig.sharpeningKernelX[7] = 0.0;
    displayConfig.sharpeningKernelX[8] = 0.0;

    displayConfig.sharpeningKernelYSize = 9;
    displayConfig.sharpeningKernelY[0] = 1643.349976;
    displayConfig.sharpeningKernelY[1] = 0.81116;
    displayConfig.sharpeningKernelY[2] = -23.274;
    displayConfig.sharpeningKernelY[3] = 166.24005;
    displayConfig.sharpeningKernelY[4] = 1.99912;
    displayConfig.sharpeningKernelY[5] = 0.0;
    displayConfig.sharpeningKernelY[6] = 0.0;
    displayConfig.sharpeningKernelY[7] = 0.0;
    displayConfig.sharpeningKernelY[8] = 0.0;

    displayConfig.viewResolution[0] = 640;
    displayConfig.viewResolution[1] = 400;

    displayConfig.displaySizeInMm[0] = 0;
    displayConfig.displaySizeInMm[1] = 0;

    displayConfig.act_gamma = 2.0;
    displayConfig.act_beta = 1.4;
    displayConfig.systemDisparityPercent = 0.0125f;
    displayConfig.systemDisparityPixels = 8.0f;
    displayConfig.cameraCenterX = -25;
    displayConfig.cameraCenterY = 185;
    displayConfig.centerViewNumber = 0.0f;
    displayConfig.n = 1.47f;
    displayConfig.theta = 0.0f;
    displayConfig.s = 11.0f;
    displayConfig.d_over_n = 0.5f;
    displayConfig.p_over_du = 3.0f;
    displayConfig.p_over_dv = 1.0f;
    displayConfig.colorInversion = false;
    displayConfig.colorSlant = 0;
    displayConfig.convergenceDistance = 500.0f;

    return displayConfig;
}

FDisplayConfig AbstractLeiaDevice::GetDisplayConfig2D() const
{
    // Use a config based on LumePad with a single view. Specializations for Windows/Android can modify the resolution and/or any relevant properties.
    FDisplayConfig displayConfig = GetDisplayConfigLumePad();
    displayConfig.numViews[0] = 1;
    displayConfig.numViews[1] = 1;
    return displayConfig;
}