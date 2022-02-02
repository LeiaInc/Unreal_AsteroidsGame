// Fill out your copyright notice in the Description page of Project Settings.

#include "WindowsLeiaDevice.h"

WindowsLeiaDevice::WindowsLeiaDevice()
{
#if PLATFORM_WINDOWS && !WITH_EDITOR
	hmodDisplaySdkCpp = LoadLibrary(_T("C:\\Program Files\\LeiaInc\\LeiaDisplaySdk\\LeiaDisplaySdkCpp.dll"));
	hmodDisplayParams = LoadLibrary(_T("C:\\Program Files\\LeiaInc\\LeiaDisplaySdk\\LeiaDisplayParams.dll"));
#endif
}

WindowsLeiaDevice::~WindowsLeiaDevice()
{
#if PLATFORM_WINDOWS && !WITH_EDITOR
	FreeLibrary(hmodDisplaySdkCpp);
	FreeLibrary(hmodDisplayParams);
#endif
}

bool WindowsLeiaDevice::IsConnected()
{
	bool isDisplayConnected = false;

#if PLATFORM_WINDOWS && !WITH_EDITOR
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
	}
#endif
	return isDisplayConnected;
}

void WindowsLeiaDevice::SetBacklightMode(BacklightMode modeId)
{
#if PLATFORM_WINDOWS && !WITH_EDITOR

	if (IsConnected())
	{
		if (fPtrRequestBacklight == nullptr)
		{
			fPtrRequestBacklight = (MINT_VOID)GetProcAddress(hmodDisplaySdkCpp, "requestBacklightMode");
		}

		if (fPtrRequestBacklight != nullptr)
		{
			fPtrRequestBacklight((modeId == BacklightMode::MODE_3D) ? 1 : 0);
		}
	}
#endif
}

BacklightMode WindowsLeiaDevice::GetBacklightMode()
{
	BacklightMode mode = BacklightMode::MODE_2D;
#if PLATFORM_WINDOWS && !WITH_EDITOR
	if (IsConnected())
	{
		if (fPtrGetBacklightMode == nullptr)
		{
			fPtrGetBacklightMode = (MATOMIC_INT)GetProcAddress(hmodDisplaySdkCpp, "getBacklightMode");
		}

		if (fPtrGetBacklightMode != nullptr)
		{
			mode = static_cast<BacklightMode>(fPtrGetBacklightMode());
		}
	}
#endif
	return mode;
}

FDisplayConfig WindowsLeiaDevice::GetDisplayConfig()
{
	FDisplayConfig displayConfig;
#if PLATFORM_WINDOWS && !WITH_EDITOR

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