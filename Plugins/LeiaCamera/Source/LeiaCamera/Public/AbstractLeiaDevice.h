// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ILeiaDevice.h"

UENUM(BlueprintType)
enum class EViewOverrideMode : uint8
{
	NoOverride UMETA(DisplayName = "NoOverride"),
	FourView UMETA(DisplayName = "FourView"),
	EightView UMETA(DisplayName = "EightView")
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
	#if WITH_EDITOR
	void SetOverride(EViewOverrideMode overrideMode);
	#endif

private:
#if WITH_EDITOR
	EViewOverrideMode OverrideMode = EViewOverrideMode::NoOverride;

	FDisplayConfig Get8VDisplayConfig() const;
	FDisplayConfig Get4VDisplayConfig() const;
#endif
};
