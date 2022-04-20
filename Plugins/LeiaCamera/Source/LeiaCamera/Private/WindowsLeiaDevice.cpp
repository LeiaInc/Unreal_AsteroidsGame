/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

#include "WindowsLeiaDevice.h"

//UE_LOG(LogLeia, Log, TEXT("SetFinalViewRect %d %f"), someInt, someFloat);

WindowsLeiaDevice::WindowsLeiaDevice()
{
#if LEIA_USE_SERVICE
	hmodDisplaySdkCpp = LoadLibrary(_T("LeiaDisplaySdkCpp.dll"));
	hmodDisplayParams = LoadLibrary(_T("C:\\Program Files\\LeiaInc\\LeiaDisplaySdk\\LeiaDisplayParams.dll"));

	if (hmodDisplaySdkCpp == nullptr) { UE_LOG(LogLeia, Warning, TEXT("LeiaLog : WindowsLeiaDevice::Constructor(line 15) : hmodDisplaySDKCpp is set to null pointer. Expected to find 'C:\\Program Files\\LeiaInc\\LeiaDisplaySdk\\LeiaDisplaySdkCpp.dll'. Confirm Leia Service was setup appropriately.")); }
	if (hmodDisplayParams == nullptr) { UE_LOG(LogLeia, Warning, TEXT("LeiaLog : WindowsLeiaDevice::Constructor(line 16) : hmodDisplayParams set to null pointer. Expected to find 'C:\\Program Files\\LeiaInc\\LeiaDisplaySdk\\LeiaDisplayParams.dll'. Confirm Leia Service was setup appropriately.")); }
#endif
}

WindowsLeiaDevice::~WindowsLeiaDevice()
{
	if (OverrideMode != EViewOverrideMode::None)
	{
		return;
	}
#if LEIA_USE_SERVICE
	FreeLibrary(hmodDisplaySdkCpp);
	FreeLibrary(hmodDisplayParams);
#endif
}

bool WindowsLeiaDevice::IsConnected()
{
	if (OverrideMode != EViewOverrideMode::None)
	{
		UE_LOG(LogLeia, Log, TEXT("LeiaLog : WindowsLeiaDevice::IsConnected(line 34) : defaulting IsConnected to true."));
		return true;
	}
	bool isDisplayConnected = false;

#if LEIA_USE_SERVICE
	if (hmodDisplaySdkCpp != nullptr)
	{
		if (fPtrConnected == nullptr)
		{
			fPtrConnected = (MATOMIC_BOOL)GetProcAddress(hmodDisplaySdkCpp, "isDisplayConnected");
		}

		if (fPtrConnected != nullptr)
		{
			isDisplayConnected = fPtrConnected();
		}
		else { UE_LOG(LogLeia, Warning, TEXT("LeiaLog : WindowsLeiaDevice::IsConnected(line 46) : fPtrConnected set to null pointer. Expected to find isDisplayConnected function in hmodDisplaySDKCpp. ")); }
	}
		else { UE_LOG(LogLeia, Warning, TEXT("LeiaLog : WindowsLeiaDevice::IsConnected(line 42) : hmodDisplaySDKCpp is set to null pointer. Expected to find C:\\Program Files\\LeiaInc\\LeiaDisplaySdk\\LeiaDisplaySdkCpp.dll. Confirm Leia Service was setup appropriately.")); }
#endif

	return isDisplayConnected;
}

void WindowsLeiaDevice::SetBacklightMode(BacklightMode modeId)
{
#if LEIA_USE_SERVICE

	if (IsConnected())
	{
		if (fPtrRequestBacklight == nullptr)
		{
			fPtrRequestBacklight = (MINT_VOID)GetProcAddress(hmodDisplaySdkCpp, "requestBacklightMode"); 
		}

		if (fPtrRequestBacklight != nullptr)
		{
			fPtrRequestBacklight(static_cast<int>(modeId));
			if (modeId == BacklightMode::MODE_3D) { UE_LOG(LogLeia, Log, TEXT("LeiaLog : WindowsLeiaDevice::SetBacklightMode(line 67) : Successfully set backlight to 3D.")); }
			if (modeId == BacklightMode::MODE_2D) { UE_LOG(LogLeia, Log, TEXT("LeiaLog : WindowsLeiaDevice::SetBacklightMode(line 67) : Successfully set backlight to 2D.")); }
		}
		else { UE_LOG(LogLeia, Warning, TEXT("LeiaLog : WindowsLeiaDevice::SetBacklightMode(line 69) : fPtrRequestBacklight set to null pointer. Expected to find requestBacklightMode function in hmodDisplaySDKCpp. ")); }
	}
	else { UE_LOG(LogLeia, Warning, TEXT("LeiaLog : WindowsLeiaDevice::SetBacklightMode(line 65) : IsConnected returned false. ")); }
#endif
}

BacklightMode WindowsLeiaDevice::GetBacklightMode()
{
	BacklightMode mode = BacklightMode::MODE_2D;
#if LEIA_USE_SERVICE
	if (IsConnected())
	{
		if (fPtrGetBacklightMode == nullptr)
		{
			fPtrGetBacklightMode = (MATOMIC_INT)GetProcAddress(hmodDisplaySdkCpp, "getBacklightMode");
		}

		if (fPtrGetBacklightMode != nullptr)
		{
			mode = static_cast<BacklightMode>(fPtrGetBacklightMode());
			if (mode == BacklightMode::MODE_3D) { UE_LOG(LogLeia, Log, TEXT("LeiaLog : WindowsLeiaDevice::GetBacklightMode(line 99) : Successfully read backlight from firmware as 3D.")); }
			if (mode == BacklightMode::MODE_2D) { UE_LOG(LogLeia, Log, TEXT("LeiaLog : WindowsLeiaDevice::GetBacklightMode(line 99) : Successfully read backlight from firmware as 2D.")); }
		}
		else { UE_LOG(LogLeia, Warning, TEXT("LeiaLog : WindowsLeiaDevice::GetBacklightMode(line 92) : fPtrGetBacklightMode set to null pointer. Expected to find getBacklightMode function in hmodDisplaySDKCpp. ")); }
	}
	else { UE_LOG(LogLeia, Warning, TEXT("LeiaLog : WindowsLeiaDevice::GetBacklightMode(line 88) : IsConnected returned false. ")); }
#endif
	return mode;
}

FDisplayConfig WindowsLeiaDevice::GetDisplayConfig()
{
	if (OverrideMode != EViewOverrideMode::None)
	{
		return AbstractLeiaDevice::GetDisplayConfig();
	}

	FDisplayConfig displayConfig;

#if LEIA_USE_SERVICE

	if (IsConnected())
	{
		if (fPtrGetDisplayConfig == nullptr)
		{
			fPtrGetDisplayConfig = (MATOMIC_DCS)GetProcAddress(hmodDisplaySdkCpp, "getDisplayConfig");
		}

		if (fPtrGetDisplayConfig != nullptr) 
		{
			DisplayConfigStruct dcs = fPtrGetDisplayConfig();

			displayConfig.dotPitchInMM[0] = dcs.dotPitchInMm.x;
			displayConfig.dotPitchInMM[1] = dcs.dotPitchInMm.y;

			displayConfig.panelResolution[0] = dcs.panelResolution.x;
			displayConfig.panelResolution[1] = dcs.panelResolution.y;

			displayConfig.numViews[0] = dcs.numViews.x;
			displayConfig.numViews[1] = dcs.numViews.y;

			displayConfig.alignmentOffset[0] = dcs.alignmentOffset.x;
			displayConfig.alignmentOffset[1] = dcs.alignmentOffset.y;

			displayConfig.sharpeningKernelX[0] = static_cast<float>(dcs.viewSharpeningCoefficients.x.size);
			displayConfig.sharpeningKernelX[1] = dcs.viewSharpeningCoefficients.x.items[0];
			displayConfig.sharpeningKernelX[2] = dcs.viewSharpeningCoefficients.x.items[1];
			displayConfig.sharpeningKernelX[3] = dcs.viewSharpeningCoefficients.x.items[2];
			displayConfig.sharpeningKernelX[4] = dcs.viewSharpeningCoefficients.x.items[3];

			displayConfig.sharpeningKernelY[0] = static_cast<float>(dcs.viewSharpeningCoefficients.y.size);
			displayConfig.sharpeningKernelY[1] = dcs.viewSharpeningCoefficients.y.items[0];
			displayConfig.sharpeningKernelY[2] = dcs.viewSharpeningCoefficients.y.items[1];
			displayConfig.sharpeningKernelY[3] = dcs.viewSharpeningCoefficients.y.items[2];
			displayConfig.sharpeningKernelY[4] = dcs.viewSharpeningCoefficients.y.items[3];

			displayConfig.viewResolution[0] = dcs.viewResolution.x;
			displayConfig.viewResolution[1] = dcs.viewResolution.y;

			displayConfig.displaySizeInMm[0] = dcs.displaySizeInMm.x;
			displayConfig.displaySizeInMm[1] = dcs.displaySizeInMm.y;

			displayConfig.interlacingVector = FVector4{ dcs.interlacingVector.items[0], dcs.interlacingVector.items[1], dcs.interlacingVector.items[2], dcs.interlacingVector.items[3] };
			displayConfig.interlacingVectorLandscape = displayConfig.interlacingVector;
			displayConfig.interlacingVectorLandscape180 = displayConfig.interlacingVector;
			displayConfig.interlacingVectorPortrait = displayConfig.interlacingVector;
			displayConfig.interlacingVectorPortrait180 = displayConfig.interlacingVector;

			float row1[4] = { dcs.interlacingMatrix.items[0][0] , dcs.interlacingMatrix.items[0][1], dcs.interlacingMatrix.items[0][2], dcs.interlacingMatrix.items[0][3] };
			float row2[4] = { dcs.interlacingMatrix.items[1][0] , dcs.interlacingMatrix.items[1][1], dcs.interlacingMatrix.items[1][2], dcs.interlacingMatrix.items[1][3] };
			float row3[4] = { dcs.interlacingMatrix.items[2][0] , dcs.interlacingMatrix.items[2][1], dcs.interlacingMatrix.items[2][2], dcs.interlacingMatrix.items[2][3] };
			float row4[4] = { dcs.interlacingMatrix.items[3][0] , dcs.interlacingMatrix.items[3][1], dcs.interlacingMatrix.items[3][2], dcs.interlacingMatrix.items[3][3] };

			displayConfig.interlacingMatrixLandscape = { {row1[0], row1[1], row1[2], row1[3]}, {row2[0], row2[1], row2[2], row2[3]}, {row3[0], row3[1], row3[2], row3[3]}, {row4[0], row4[1], row4[2], row4[3]} };
			displayConfig.interlacingMatrixLandscape180 = displayConfig.interlacingMatrixLandscape;
			displayConfig.interlacingMatrixPortrait = displayConfig.interlacingMatrixLandscape;
			displayConfig.interlacingMatrixPortrait180 = displayConfig.interlacingMatrixLandscape;

			displayConfig.gamma = dcs.gamma;
			displayConfig.beta = dcs.beta;
			displayConfig.systemDisparityPercent = dcs.systemDisparity;

		}
	}
#endif
	return displayConfig;
}
