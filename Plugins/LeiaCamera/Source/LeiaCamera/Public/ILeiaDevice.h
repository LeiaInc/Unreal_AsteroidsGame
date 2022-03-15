// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintPlatformLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogLeia, Log, All);

// Moved this define here because it's used in both WindowsLeiaDevice and LeiaCameraBase, and their only common header is this file.
#define LEIA_USE_SERVICE  (PLATFORM_WINDOWS && !WITH_EDITOR)

struct FDisplayConfig
{
	float dotPitchInMM[2] = { 0.0f, 0.0f };
	int32 panelResolution[2] = { 0, 0 };
	int32 numViews[2] = { 0, 0 };
	int32 sharpeningKernelXSize = 0;
	float sharpeningKernelX[18] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	int32 sharpeningKernelYSize = 0;
	float sharpeningKernelY[18] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	int32 viewResolution[2] = { 0, 0 };
	int32 displaySizeInMm[2] = { 0, 0 };
	float act_gamma = 0.0f;
	float act_beta = 0.0f;
	float systemDisparityPercent = 0.0f;
	float systemDisparityPixels = 0.0f;
	float cameraCenterX = -5.0f;
	float cameraCenterY = 145.0f;
	float centerViewNumber = 0.0f;
    float n = 0.0f;
    float theta = 0.0f;
    float s = 0.0f;
    float d_over_n = 0.0f;
    float p_over_du = 0.0f;
    float p_over_dv = 0.0f;
    bool colorInversion = false;
    int colorSlant = 0;
	float cameraTheta = 0.0f;
	float convergenceDistance = 0.0f;

	struct OrientationConfig
	{
		float dotPitchInMM[2] = { 0.0f, 0.0f };
		int32 panelResolution[2] = { 0, 0 };
		int32 numViews[2] = { 0, 0 };
		int32 viewResolution[2] = { 0, 0 };
		float displaySizeInMm[2] = { 0, 0 };
	};

	OrientationConfig ToScreenOrientationConfig(EScreenOrientation::Type orientation) const
	{
		OrientationConfig config;

		config.numViews[0] = numViews[0];
		config.numViews[1] = numViews[1];

		if (orientation == EScreenOrientation::LandscapeLeft || orientation == EScreenOrientation::LandscapeRight || orientation == EScreenOrientation::Unknown)
		{
			config.dotPitchInMM[0] = dotPitchInMM[0];
			config.dotPitchInMM[1] = dotPitchInMM[1];

			config.panelResolution[0] = panelResolution[0];
			config.panelResolution[1] = panelResolution[1];

			config.viewResolution[0] = viewResolution[0];
			config.viewResolution[1] = viewResolution[1];

			config.displaySizeInMm[0] = displaySizeInMm[0];
			config.displaySizeInMm[1] = displaySizeInMm[1];

		}
		else
		{
			config.dotPitchInMM[0] = dotPitchInMM[1];
			config.dotPitchInMM[1] = dotPitchInMM[0];

			config.panelResolution[0] = panelResolution[1];
			config.panelResolution[1] = panelResolution[0];

			config.viewResolution[0] = viewResolution[1];
			config.viewResolution[1] = viewResolution[0];

			config.displaySizeInMm[0] = displaySizeInMm[1];
			config.displaySizeInMm[1] = displaySizeInMm[0];
		}

		return config;
	}
};

UENUM(BlueprintType)
enum class BacklightMode : uint8
{
	//MODE_INVALID = 0 UMETA(DisplayName = "INVALID_DONT_USE"),
	MODE_2D = 0 UMETA(DisplayName = "2D"),
	MODE_3D = 3 UMETA(DisplayName = "3D"),
};

/**
 *
 */
class LEIACAMERA_API ILeiaDevice
{
public:
	ILeiaDevice() = default;
	virtual ~ILeiaDevice() {};
	virtual void SetProfileStubName(FString name) = 0;
	virtual void SetBacklightMode(BacklightMode modeId) = 0;
	virtual void SetBacklightMode(BacklightMode modeId, int delay) = 0;
	virtual BacklightMode GetBacklightMode() = 0;
	virtual FString GetSensors() = 0;
	virtual bool IsSensorsAvailable() = 0;
	virtual bool IsConnected() = 0;
	virtual void CalibrateSensors() = 0;
	virtual FDisplayConfig GetDisplayConfig() = 0;
	int CalibrationOffset[2];

};
