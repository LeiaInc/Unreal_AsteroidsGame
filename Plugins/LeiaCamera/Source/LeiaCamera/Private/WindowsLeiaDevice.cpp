// Fill out your copyright notice in the Description page of Project Settings.

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
	static FDisplayConfig displayConfig;

	static bool dllHasBeenRead = false;


#if LEIA_USE_SERVICE

	if (IsConnected())
	{
		if (fPtrGetDisplayConfig == nullptr)
		{
			fPtrGetDisplayConfig = (MATOMIC_DCS)GetProcAddress(hmodDisplaySdkCpp, "getDisplayConfig");
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read"));
		}

		if (fPtrGetDisplayConfig != nullptr && dllHasBeenRead == false)
		{
			dllHasBeenRead = true;
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read Succeeded"));

			DisplayConfigStruct dcs = fPtrGetDisplayConfig();

			displayConfig.n = 1.47f;
			displayConfig.theta = 0.0f;
			displayConfig.cameraCenterX = -37.88f;
			displayConfig.cameraCenterY = 165.57f;
			displayConfig.colorSlant = 0;
			displayConfig.colorInversion = false;
			displayConfig.p_over_du = 3.0f;
			displayConfig.p_over_dv = dcs.isSlanted ? 1.0f : -1.0f;
			displayConfig.s = 11.0f;
			displayConfig.d_over_n = 0.5f;
			displayConfig.cameraTheta = 0.0f;

			// Get CenterViewNumm, Beta and Gamma from the old location. These will be overridden by values in new location.
			displayConfig.centerViewNumber = dcs.centerViewNumber;
			displayConfig.act_gamma = dcs.gamma;
			displayConfig.act_beta = dcs.beta;

			displayConfig.dotPitchInMM[0] = dcs.dotPitchInMm.x;
			displayConfig.dotPitchInMM[1] = dcs.dotPitchInMm.y;

			displayConfig.panelResolution[0] = dcs.panelResolution.x;
			displayConfig.panelResolution[1] = dcs.panelResolution.y;

			displayConfig.numViews[0] = dcs.numViews.x;
			displayConfig.numViews[1] = dcs.numViews.y;
			displayConfig.convergenceDistance = dcs.convergenceDistance;

			displayConfig.sharpeningKernelXSize = dcs.viewSharpeningCoefficients.x.size;
			for (int i = 0; i < displayConfig.sharpeningKernelXSize; i++)
				displayConfig.sharpeningKernelX[i] = dcs.viewSharpeningCoefficients.x.items[i];

			displayConfig.sharpeningKernelYSize = dcs.viewSharpeningCoefficients.y.size;
			for (int i = 0; i < displayConfig.sharpeningKernelYSize; i++)
				displayConfig.sharpeningKernelY[i] = dcs.viewSharpeningCoefficients.y.items[i];

			displayConfig.viewResolution[0] = dcs.viewResolution.x;
			displayConfig.viewResolution[1] = dcs.viewResolution.y;

			displayConfig.displaySizeInMm[0] = dcs.displaySizeInMm.x;
			displayConfig.displaySizeInMm[1] = dcs.displaySizeInMm.y;

			displayConfig.systemDisparityPercent = dcs.systemDisparity;
			displayConfig.systemDisparityPixels = 8.0f;

			const float row1[4] = { dcs.interlacingMatrix.items[0][0] , dcs.interlacingMatrix.items[0][1], dcs.interlacingMatrix.items[0][2], dcs.interlacingMatrix.items[0][3] };
			const float row2[4] = { dcs.interlacingMatrix.items[1][0] , dcs.interlacingMatrix.items[1][1], dcs.interlacingMatrix.items[1][2], dcs.interlacingMatrix.items[1][3] };
			const float row3[4] = { dcs.interlacingMatrix.items[2][0] , dcs.interlacingMatrix.items[2][1], dcs.interlacingMatrix.items[2][2], dcs.interlacingMatrix.items[2][3] };
			const float row4[4] = { dcs.interlacingMatrix.items[3][0] , dcs.interlacingMatrix.items[3][1], dcs.interlacingMatrix.items[3][2], dcs.interlacingMatrix.items[3][3] };

			// Version number is first entry of the interlace matrix.
			const float versionNum = row1[0];

			// Use the new method using interlace matrix. This will be valid IF version number is 2 or higher.
			if (versionNum >= 2.0f)
			{
				UE_LOG(LogLeia, Log, TEXT("LeiaLog : Using DisplayConfig v2.0 or greater"));
				// We are loading from the interlace matrix
				displayConfig.centerViewNumber = row1[1];
				displayConfig.n = row1[2];
				displayConfig.theta = row1[3];
				displayConfig.s = row2[0];
				displayConfig.d_over_n = row2[1];
				displayConfig.p_over_du = row2[2];
				displayConfig.p_over_dv = row2[3];
				displayConfig.colorSlant = ((int)row3[0]);
				displayConfig.colorInversion = fabs(row3[1]) > 0.001;
				displayConfig.cameraCenterX = row3[2];
				displayConfig.cameraCenterY = row3[3];
				displayConfig.act_beta = row4[0];
				displayConfig.act_gamma = row4[1];
				displayConfig.cameraTheta = row4[2];
			}
			else if (displayConfig.sharpeningKernelY[0] >= 1000.0)
			{
				UE_LOG(LogLeia, Log, TEXT("LeiaLog : Using DisplayConfig not 2.0!"));
				// Fallback, old method of storing in ACT Y coeifficents
				// Only valid IF the first element of ACT Y coefficients is greather than or equal to 10000

				displayConfig.n = displayConfig.sharpeningKernelY[0] / 1000.0f;
				displayConfig.theta = displayConfig.sharpeningKernelY[1];
				displayConfig.cameraCenterX = displayConfig.sharpeningKernelY[2];
				displayConfig.cameraCenterY = displayConfig.sharpeningKernelY[3];
				displayConfig.centerViewNumber = displayConfig.centerViewNumber;



				// Check if Beta is valid and if so override
				if (displayConfig.sharpeningKernelY[4] > 1.0f)
				{
					displayConfig.act_beta = displayConfig.sharpeningKernelY[4];
					UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read Overriding Beta!"));
				}



				//UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read Succeeded %f"), ));

				// Check if IO (ViewBoxSize) is valid and override (FB late change)
				float IO = dcs.viewboxSize.x;
				if (displayConfig.sharpeningKernelY[5] > 0.0)
				{
					IO = displayConfig.sharpeningKernelY[5];
					UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read IO (ViewBoxSize) is valid,  overriding!!! (late change)"));
				}

				UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read IO  %f"), IO);


				// Step 6: Compute from D and IO the d_over_n and s
				const float D = dcs.convergenceDistance;

				displayConfig.convergenceDistance = D;

				const float du = displayConfig.dotPitchInMM[0] / displayConfig.p_over_du;
				displayConfig.d_over_n = du * D / IO;
				displayConfig.s = (du / IO) * float(displayConfig.panelResolution[0]) * displayConfig.p_over_du;

				UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read displayConfig.convergenceDistance  %f"), dcs.convergenceDistance);
			}
			else
			{
				UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read Cant Resolve Version, falling through"));
			}

			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Overriding Centerview - shifting by 4.0 from %f"), displayConfig.centerViewNumber);

			static const float UE4_TEMP_CENTER_VIEW_SHIFT = 4.0f;
			displayConfig.centerViewNumber += UE4_TEMP_CENTER_VIEW_SHIFT;


			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read displayConfig.centerViewNumber %f"), displayConfig.centerViewNumber);
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read displayConfig.n %f"), displayConfig.n);
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read displayConfig.theta   %f"), displayConfig.theta);
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read displayConfig.cameraCenterX %f"), displayConfig.cameraCenterX);
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read displayConfig.cameraCenterY %f"), displayConfig.cameraCenterY);
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: displayConfig.act_beta %f"), displayConfig.act_beta);
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read displayConfig.d_over_n  %f"), displayConfig.d_over_n);
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read displayConfig.s  %f"), displayConfig.s);



#if 0 //For developers we want them to fall through to their override in this case
			if (!displayConfigInitialized)
			{
				UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read FAILED. Using 2D configuration"));
				displayConfig = GetDisplayConfig2D();
				displayConfigInitialized = true;
			}
			else
#endif 
		}

		if (dllHasBeenRead == false)
		{
			UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Read Cant read display struct"));
		}

	}
#endif
	if (dllHasBeenRead == false && OverrideMode != EViewOverrideMode::None)
	{
		UE_LOG(LogLeia, Log, TEXT("LeiaLog : Display Config: Couldn't read config, using override set in pawn!"));

		return AbstractLeiaDevice::GetDisplayConfig();
	}
	return displayConfig;
}

FDisplayConfig WindowsLeiaDevice::GetDisplayConfig2D() const
{
	// Start with base class config.
	FDisplayConfig config = AbstractLeiaDevice::GetDisplayConfig2D();

#if 0 //getting build errors on windows

	// Get display resolution.
	const int PrimaryDisplayHorizontalResolution = GetSystemMetrics(SM_CXSCREEN);
	const int PrimaryDisplayVerticalResolution   = GetSystemMetrics(SM_CYSCREEN);

	// Modify config to match display resolution.
	config.viewResolution[0]  = PrimaryDisplayHorizontalResolution;
	config.viewResolution[1]  = PrimaryDisplayVerticalResolution;
	config.panelResolution[0] = PrimaryDisplayHorizontalResolution;
	config.panelResolution[1] = PrimaryDisplayVerticalResolution;

#endif
	return config;
}