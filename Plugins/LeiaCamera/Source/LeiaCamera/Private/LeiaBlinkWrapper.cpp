// Fill out your copyright notice in the Description page of Project Settings.

#include "LeiaBlinkWrapper.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"


#define LEIA_ENABLE_BLINK (PLATFORM_WINDOWS)


#if LEIA_ENABLE_BLINK
#include <iostream>
#include <iomanip>

#undef  TEXT

#include "Windows/AllowWindowsPlatformTypes.h"
#include "blinkTracking.h"
#include <WinUser.h>
#include <stdio.h>
#include <usb.h>
#include <tchar.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include "Windows/HideWindowsPlatformTypes.h"

#define TEXT(x) TEXT_PASTE(x)

#endif


DEFINE_LOG_CATEGORY_STATIC(LogLeiaBlink, Log, All);



LeiaBlinkWrapper::LeiaBlinkWrapper()
{

}

LeiaBlinkWrapper::~LeiaBlinkWrapper()
{

}

FVector2D GetGameResolutionWrapper()
{
	FVector2D Result = FVector2D(1, 1);

	Result.X = GSystemResolution.ResX;
	Result.Y = GSystemResolution.ResY;

	return Result;
}



bool camConnected = false;
bool isRealSenseConnected();

void LeiaBlinkWrapper::Start()
{
	auto viewPortSize = GetGameResolutionWrapper();

#if LEIA_ENABLE_BLINK

	camConnected = isRealSenseConnected();

	if (!camConnected)
	{
		GEngine->AddOnScreenDebugMessage(5000, 5, FColor::Red, FString::Printf(TEXT("Realsense cam not found")), true, FVector2D(2, 2));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(5000, 5, FColor::Green, FString::Printf(TEXT("Realsense Cam connected")), true, FVector2D(2, 2));
	}

	if (!GIsPlayInEditorWorld && camConnected)
	{
		blink_initialize(false);
		blink_cam_setCameraPos(0, 0, 0);
		blink_startTracking();

	}

#endif

}


FVector2D GetGameViewportSizeWrapper()
{
	FVector2D Result = FVector2D(1, 1);

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize( /*out*/Result);
	}

	return Result;
}


int LeiaBlinkWrapper::Tick(FVector & primaryFace, float deltaTime, float cameraCenterX, float cameraCenterY)
{
	int face_size = 0;

	if (initDelay > 0.0f)
	{
		initDelay -= deltaTime;
		return face_size;
	}

	primaryFace = lastPrimaryFace;

#if LEIA_ENABLE_BLINK
	if (GIsPlayInEditorWorld || !camConnected)
	{		
		return face_size;
	}

	std::vector<LeiaFaceFeature> faces;
	faces.resize(20);

	LeiaDisplayParameter param;
	param.centerViewNumX = 3.5f;
	param.centerViewNumY = 1.0f;
	param.convergeDimX = 30.0f;
	param.convergeDimY = 1000000.0f;
	param.convergeDist = 650.0f;
	param.displayOption = 0;
	param.viewSlant = -0.3217f;
	param.viewCountX = 12;
	param.viewCountY = 1;


	bool use_rgb = false;
	bool enable_ae = true;

	//todo, make this screen res?

	
	face_size = blink_getTrackingResultVector(faces, param);

	const float eyeDistanceThreshold = 100.f;
	if (face_size > 0) 
	{
		
		int closestFaceIdx = 0;
		float closestFaceZDist = FLT_MAX;

		for (int i = 0; i < face_size; ++i)
		{
			auto const& f = faces[i];
			float faceDistance = 4 * f.faceWorldX * f.faceWorldX + 4 * f.faceWorldY * f.faceWorldY + f.faceWorldZ * f.faceWorldZ;
			if (faceDistance < closestFaceZDist)
			{
				closestFaceZDist = faceDistance;
				closestFaceIdx = i;
			}
		}
		auto closestFace = faces[closestFaceIdx];

		if (FMath::Abs(closestFace.leftEye.eyeX - closestFace.rightEye.eyeX) < eyeDistanceThreshold)
		{
			primaryFace.X = closestFace.faceWorldX;
			primaryFace.Y = closestFace.faceWorldY;
			primaryFace.Z = closestFace.faceWorldZ;

#if 0 //Validate on 12v at office!
			// Correct theta
			float angle = cameraTheta * PI / 180.0;
			float newFaceZ = primaryFace.Z * cos(angle) - primaryFace.Y * sin(angle);
			float newFaceY = primaryFace.Z * sin(angle) + primaryFace.Y * cos(angle);
			primaryFace.Y = newFaceY;
			primaryFace.Z = newFaceZ;
#endif
			//float cameraCenterX = -25 + 20; //Move to displayConfig 
			//float cameraCenterY = 125 + 20; //Move to displayConfig 

			primaryFace.X = -primaryFace.X - cameraCenterX;
			primaryFace.Y = primaryFace.Y + cameraCenterY;

			lastPrimaryFace = primaryFace;

		}

		//Drive mouse to affect metahuman demo
#if 1 //MetaHuman Demo looks less impressive when the avatar follows you with view peeling 
		
		auto viewPortSize = GetGameResolutionWrapper();

		int vswidth = viewPortSize.X;
		int vsheight = viewPortSize.Y;

		auto halfWidth = vswidth / 2;
		auto halfHeight = vsheight / 2;
		float zScale = closestFace.faceWorldZ * 0.001;

		auto xPos = halfWidth - closestFace.faceWorldX * 1.50f;
		auto yPos = vsheight - (halfHeight + closestFace.faceWorldY);



		//xPos = (xPos - halfWidth);// *zScale;

		auto controller = UGameplayStatics::GetPlayerController(GWorld, 0);
		auto Player = controller ? controller->GetLocalPlayer() : NULL;
		ULocalPlayer* LocPlayer = Cast<ULocalPlayer>(Player);
		if (LocPlayer && LocPlayer->ViewportClient->Viewport && LocPlayer->ViewportClient->Viewport->HasFocus())
		{
			controller->SetMouseLocation(xPos, yPos);
			//controller->SetMouseLocation(vswidth/2, vsheight/2);
		}
#endif

		//controller >SetShowMouseCursor(true);
		//GEngine->AddOnScreenDebugMessage(3, 1.0f, FColor::Cyan, FString::Printf(TEXT("Found Face %d %d %d"), (int) faces[0].faceWorldX, (int)faces[0].faceWorldY, (int)faces[0].faceWorldZ), true, FVector2D(2,2));
		//GEngine->AddOnScreenDebugMessage(1, 0.2f, FColor::Cyan, FString::Printf(TEXT("Face Found at\n%d, %d\nDistance %2.f"), (int)xPos, (int)yPos, primaryFace.Z), true, FVector2D(1.5, 1.5));

		//GEngine->AddOnScreenDebugMessage(4, 1.0f, FColor::Red, FString::Printf(TEXT("Test %f %f"), viewPortSize.X, viewPortSize.Y), true, FVector2D(3, 3));

	}
	else
#endif
	{
		GEngine->AddOnScreenDebugMessage(0, 0.2f, FColor::Red, FString::Printf(TEXT("No Face Found")), true, FVector2D(1.5, 1.5));
	}

	lastFaceCount = face_size;
	return face_size;
}


void LeiaBlinkWrapper::Shutdown()
{
#if LEIA_ENABLE_BLINK
	if (!GIsPlayInEditorWorld && camConnected)
	{
		blink_stopTracking();
		blink_terminate();
	}
#endif
}

#if LEIA_ENABLE_BLINK
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
	0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);

bool isRealSenseConnected()
{

	HDEVINFO                         hDevInfo;
	SP_DEVICE_INTERFACE_DATA         DevIntfData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
	SP_DEVINFO_DATA                  DevData;

	DWORD dwSize, dwMemberIdx = 0;

	// We will try to get device information set for all USB devices that have a
	// device interface and are currently present on the system (plugged in).
	hDevInfo = SetupDiGetClassDevs(
		&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
		DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		dwMemberIdx = 0;

		SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE,
			dwMemberIdx, &DevIntfData);
		FString realSenseVendorAndProductId = TEXT("vid_8086&pid_0b07");

		while (GetLastError() != ERROR_NO_MORE_ITEMS && !camConnected)
		{
			DevData.cbSize = sizeof(DevData);

			SetupDiGetDeviceInterfaceDetail(
				hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

			// Allocate memory for the DeviceInterfaceDetail struct. Don't forget to
			// deallocate it later!
			DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
			DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData,
				DevIntfDetailData, dwSize, &dwSize, &DevData))
			{

				FString deviceInfo = (TCHAR*)DevIntfDetailData->DevicePath;

				if (deviceInfo.Contains(realSenseVendorAndProductId))
				{
					//GEngine->AddOnScreenDebugMessage(100 + (int)dwMemberIdx, 10.0f, FColor::Green, FString::Printf(TEXT("Found realsense camera")), true, FVector2D(1, 1));
					camConnected = true;
				}
			}

			HeapFree(GetProcessHeap(), 0, DevIntfDetailData);

			// Continue looping
			SetupDiEnumDeviceInterfaces(
				hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, ++dwMemberIdx, &DevIntfData);
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);
	}

	return camConnected;
}

#endif