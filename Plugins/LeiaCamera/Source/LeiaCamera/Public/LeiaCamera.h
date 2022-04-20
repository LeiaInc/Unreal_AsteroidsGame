/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

#pragma once

#include "CoreMinimal.h"
#include "Runtime/HeadMountedDisplay/Public/IHeadMountedDisplayModule.h"

#define LEIA_IS_UE5 (ENGINE_MAJOR_VERSION == 5)

class FLeiaCameraModule : public IHeadMountedDisplayModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual FString GetModuleKeyName() const override
	{
		return FString("LeiaCamera");
	}

	virtual TSharedPtr< class IXRTrackingSystem, ESPMode::ThreadSafe > CreateTrackingSystem() override;

	virtual bool IsHMDConnected() override { return true; }
};
