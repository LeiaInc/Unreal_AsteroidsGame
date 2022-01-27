// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ILeiaDevice.h"

UENUM(BlueprintType)
enum class EViewOverrideMode : uint8
{
	LumePad UMETA(DisplayName = "LumePad"),
	Windows_12p5_8V UMETA(DisplayName = "Windows_12p5_8V"),
	AndroidPegasus_12p5_8V UMETA(DisplayName = "AndroidPegasus_12p5_8V"),
	AndroidPegasus_12p3_8V UMETA(DisplayName = "AndroidPegasus_12p3_8V"),
	Windows_15p6_12V UMETA(DisplayName = "Windows_15p6_12V"),
	Windows_15p6_13V UMETA(DisplayName = "Windows_15p6_13V"),
	None UMETA(DisplayName = "NoOverride_UseFirmware")
};

class LEIACAMERA_API AbstractLeiaDevice : public ILeiaDevice
{
public:

	BacklightMode DesiredLightFieldMode = BacklightMode::MODE_3D;

	AbstractLeiaDevice() = default;
	virtual ~AbstractLeiaDevice() {};
	virtual void SetProfileStubName(FString name) override;
	virtual void SetBacklightMode(BacklightMode modeId) override;
	virtual void SetBacklightMode(BacklightMode modeId, int delay) override;
	virtual BacklightMode GetBacklightMode() override;
	virtual FString GetSensors() override;
	virtual bool IsSensorsAvailable() override;
	virtual bool IsConnected() override;
	virtual void CalibrateSensors() override;
	virtual FDisplayConfig GetDisplayConfig() override;
	void SetOverride(EViewOverrideMode overrideMode);

	FDisplayConfig GetDisplayConfigAndroidPegasus12p5_8V() const;
	FDisplayConfig GetDisplayConfigAndroidPegasus12p3_8V() const;
	FDisplayConfig GetDisplayConfigWindows12p5_8V() const;
	FDisplayConfig GetDisplayConfigWindows15p6_12V() const;
	FDisplayConfig GetDisplayConfigWindows15p6_13V() const;
	FDisplayConfig GetDisplayConfigLumePad() const;

	EViewOverrideMode OverrideMode = EViewOverrideMode::Windows_15p6_12V;
};
