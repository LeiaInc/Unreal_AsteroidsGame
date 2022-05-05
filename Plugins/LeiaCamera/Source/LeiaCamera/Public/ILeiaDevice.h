/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

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
	int32 panelResolution[2] = { 2560, 1600 };
	int32 numViews[2] = { 4, 1 };
	int32 alignmentOffset[2] = { 0, 0 };
	float actCoefficients[4] = { 0.06f, 0.025f, 0.04f, 0.02f };
	float sharpeningKernelX[5] = {};
	float sharpeningKernelY[5] = {};
	int32 viewResolution[2] = { 640, 400 };
	int32 displaySizeInMm[2] = { 0, 0 };
	FMatrix interlacingMatrixLandscape = { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {640, 0, 0, 0} };
	FMatrix interlacingMatrixLandscape180 = { {1, 0, 0, 0}, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 640, 0, 0, 0 } };
	FMatrix interlacingMatrixPortrait = { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {400, 0, 0, 0} };
	FMatrix interlacingMatrixPortrait180 = { {1, 0, 0, 0}, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 400, 0, 0, 0 } };

	FVector4 interlacingVector = FVector4(0.0f);
	FVector4 interlacingVectorLandscape = FVector4(0.0f);
	FVector4 interlacingVectorLandscape180 = FVector4(0.0f);
	FVector4 interlacingVectorPortrait = FVector4(0.0f);
	FVector4 interlacingVectorPortrait180 = FVector4(0.0f);

	float gamma = 2.88;
	float beta = 1.4;
	float systemDisparityPercent = 0.0125f;
	float systemDisparityPixels = 8.0f;

	struct OrientationConfig
	{
		float dotPitchInMM[2] = { 0.0f, 0.0f };
		int32 panelResolution[2] = { 2560, 1600 };
		int32 numViews[2] = { 4, 4 };
		int32 alignmentOffset[2] = { 0, 0 };
		float actCoefficients[4] = { 0.06f, 0.025f, 0.04f, 0.02f };
		int32 viewResolution[2] = { 640, 400 };
		float displaySizeInMm[2] = { 0, 0 };
		FMatrix interlacingMatrix = { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {640, 0, 0, 0} };
		FVector4 interlacingVector = FVector4(0.0f);

		float systemDisparityPercent = 0.0125f;
		float systemDisparityPixels = 8.0f;
	};

	OrientationConfig ToScreenOrientationConfig(EScreenOrientation::Type orientation) const
	{
		OrientationConfig config;

		config.numViews[0] = this->numViews[0];
		config.numViews[1] = this->numViews[1];
		config.systemDisparityPercent = this->systemDisparityPercent;
		config.systemDisparityPixels = this->systemDisparityPixels;

		if (orientation == EScreenOrientation::LandscapeLeft || orientation == EScreenOrientation::LandscapeRight || orientation == EScreenOrientation::Unknown)
		{
			config.dotPitchInMM[0] = this->dotPitchInMM[0];
			config.dotPitchInMM[1] = this->dotPitchInMM[1];

			config.panelResolution[0] = this->panelResolution[0];
			config.panelResolution[1] = this->panelResolution[1];

			config.alignmentOffset[0] = this->alignmentOffset[0];
			config.alignmentOffset[1] = this->alignmentOffset[1];

			config.viewResolution[0] = this->viewResolution[0];
			config.viewResolution[1] = this->viewResolution[1];

			config.displaySizeInMm[0] = this->displaySizeInMm[0];
			config.displaySizeInMm[1] = this->displaySizeInMm[1];

		}
		else
		{
			config.dotPitchInMM[0] = this->dotPitchInMM[1];
			config.dotPitchInMM[1] = this->dotPitchInMM[0];

			config.panelResolution[0] = this->panelResolution[1];
			config.panelResolution[1] = this->panelResolution[0];

			config.alignmentOffset[0] = this->alignmentOffset[1];
			config.alignmentOffset[1] = this->alignmentOffset[0];

			config.viewResolution[0] = this->viewResolution[1];
			config.viewResolution[1] = this->viewResolution[0];

			config.displaySizeInMm[0] = this->displaySizeInMm[1];
			config.displaySizeInMm[1] = this->displaySizeInMm[0];
		}

		switch (orientation)
		{
		case EScreenOrientation::Unknown:
		case EScreenOrientation::LandscapeLeft: config.interlacingMatrix = this->interlacingMatrixLandscape; config.interlacingVector = this->interlacingVectorLandscape; break;
		case EScreenOrientation::LandscapeRight: config.interlacingMatrix = this->interlacingMatrixLandscape180; config.interlacingVector = this->interlacingVectorLandscape180; break;
		case EScreenOrientation::Portrait: config.interlacingMatrix = this->interlacingMatrixPortrait; config.interlacingVector = this->interlacingVectorPortrait; break;
		case EScreenOrientation::PortraitUpsideDown: config.interlacingMatrix = this->interlacingMatrixPortrait180;	config.interlacingVector = this->interlacingVectorPortrait180; break;
		default: break;
		}

		return config;
	}
};

UENUM(BlueprintType)
enum class BacklightMode : uint8
{
	MODE_3D = 0 UMETA(DisplayName = "3D"),
	MODE_2D = 1 UMETA(DisplayName = "2D"),
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
