/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

#pragma once

#include "CoreMinimal.h"
#include "AbstractLeiaDevice.h"

/**
 * 
 */
class LEIACAMERA_API AndroidLeiaDevice : public AbstractLeiaDevice
{
public:
	AndroidLeiaDevice();
	~AndroidLeiaDevice();

	void SetBacklightMode(BacklightMode modeId) override;
	void SetBacklightMode(BacklightMode modeId, int delay) override;
	BacklightMode GetBacklightMode() override;
	FDisplayConfig GetDisplayConfig() override;
};
