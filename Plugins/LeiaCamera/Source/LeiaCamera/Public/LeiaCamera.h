/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FLeiaCameraModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
