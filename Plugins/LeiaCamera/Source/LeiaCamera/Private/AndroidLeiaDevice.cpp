// Fill out your copyright notice in the Description page of Project Settings.


#include "AndroidLeiaDevice.h"
#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Launch/Public/Android/AndroidJNI.h"
#include "Android/AndroidPlatformMisc.h"
#endif

#if PLATFORM_ANDROID
#define INIT_JAVA_METHOD(name, signature) \
if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) { \
	name = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, #name, signature, false); \
	check(name != NULL); \
} else { \
	check(0); \
}

#define DECLARE_JAVA_METHOD(name) \
static jmethodID name = NULL;

DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_EnableBacklight);
DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_EnableBacklightWithDelay);
DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_DisableBacklight);
DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_DisableBacklightWithDelay);
DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_GetDisplayConfigToCSVString);
DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_GetDisplayConfigToString);
DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_IsBacklightEnabled);

#endif

AndroidLeiaDevice::AndroidLeiaDevice()
{
#if PLATFORM_ANDROID
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_EnableBacklight, "()V");
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_EnableBacklightWithDelay, "(I)V");
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_DisableBacklight, "()V");
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_DisableBacklightWithDelay, "(I)V");
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_GetDisplayConfigToCSVString, "()Ljava/lang/String;");
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_GetDisplayConfigToString, "()Ljava/lang/String;");
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_IsBacklightEnabled, "()Z");
#endif
}

AndroidLeiaDevice::~AndroidLeiaDevice()
{
}

void AndroidLeiaDevice::SetBacklightMode(BacklightMode modeId)
{
#if PLATFORM_ANDROID
	JNIEnv* Env = FAndroidApplication::GetJavaEnv(true);
	if (Env != nullptr)
	{
		if (modeId == BacklightMode::MODE_3D)
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_EnableBacklight);
		}
		else
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_DisableBacklight);
		}
	}
#endif
}

void AndroidLeiaDevice::SetBacklightMode(BacklightMode modeId, int delay)
{
#if PLATFORM_ANDROID
	JNIEnv* Env = FAndroidApplication::GetJavaEnv(true);
	if (Env != nullptr)
	{
		if (modeId == BacklightMode::MODE_3D)
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_EnableBacklightWithDelay, delay);
		}
		else
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_DisableBacklightWithDelay, delay);
		}
	}
#endif
}

BacklightMode AndroidLeiaDevice::GetBacklightMode()
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true))
	{
		return (FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_IsBacklightEnabled)) ?
			BacklightMode::MODE_3D : BacklightMode::MODE_2D;
	}
#endif
	return BacklightMode::MODE_2D;
}

FDisplayConfig AndroidLeiaDevice::GetDisplayConfig()
{
	FString str = "";
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true))
	{
		jstring jStr = (jstring)FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_GetDisplayConfigToCSVString);
		str = FJavaHelper::FStringFromParam(Env, jStr);
	}
#endif

	TArray<FString> outStrArray;
	str.ParseIntoArrayLines(outStrArray);

	FDisplayConfig displayConfig;

	if (outStrArray.Num() != 0)
	{
		displayConfig.dotPitchInMM[0] = FCString::Atof(*outStrArray[1]);
		displayConfig.dotPitchInMM[1] = FCString::Atof(*outStrArray[0]);

		displayConfig.panelResolution[0] = FCString::Atoi(*outStrArray[3]);
		displayConfig.panelResolution[1] = FCString::Atoi(*outStrArray[2]);

		displayConfig.numViews[0] = FCString::Atoi(*outStrArray[5]);
		displayConfig.numViews[1] = FCString::Atoi(*outStrArray[4]);

		displayConfig.alignmentOffset[0] = FCString::Atof(*outStrArray[7]);
		displayConfig.alignmentOffset[1] = FCString::Atof(*outStrArray[6]);

		displayConfig.displaySizeInMm[0] = FCString::Atoi(*outStrArray[9]);
		displayConfig.displaySizeInMm[1] = FCString::Atoi(*outStrArray[8]);

		displayConfig.viewResolution[0] = FCString::Atoi(*outStrArray[11]);
		displayConfig.viewResolution[1] = FCString::Atoi(*outStrArray[10]);

		displayConfig.actCoefficients[0] = FCString::Atof(*outStrArray[13]);
		displayConfig.actCoefficients[1] = FCString::Atof(*outStrArray[12]);
		displayConfig.actCoefficients[2] = FCString::Atof(*outStrArray[15]);
		displayConfig.actCoefficients[3] = FCString::Atof(*outStrArray[14]);

		displayConfig.sharpeningKernelX[0] = FCString::Atof(*outStrArray[16]);
		displayConfig.sharpeningKernelX[1] = FCString::Atof(*outStrArray[17]);
		displayConfig.sharpeningKernelX[2] = FCString::Atof(*outStrArray[18]);
		displayConfig.sharpeningKernelX[3] = FCString::Atof(*outStrArray[19]);
		displayConfig.sharpeningKernelX[4] = FCString::Atof(*outStrArray[20]);

		displayConfig.sharpeningKernelY[0] = FCString::Atof(*outStrArray[21]);
		displayConfig.sharpeningKernelY[1] = FCString::Atof(*outStrArray[22]);
		displayConfig.sharpeningKernelY[2] = FCString::Atof(*outStrArray[23]);
		displayConfig.sharpeningKernelY[3] = FCString::Atof(*outStrArray[24]);
		displayConfig.sharpeningKernelY[4] = FCString::Atof(*outStrArray[25]);

		displayConfig.gamma = FCString::Atof(*outStrArray[26]);
		displayConfig.beta = FCString::Atof(*outStrArray[27]);

		displayConfig.interlacingVectorLandscape.X = FCString::Atof(*outStrArray[28]);
		displayConfig.interlacingVectorLandscape.Y = FCString::Atof(*outStrArray[29]);
		displayConfig.interlacingVectorLandscape.Z = FCString::Atof(*outStrArray[30]);
		displayConfig.interlacingVectorLandscape.W = FCString::Atof(*outStrArray[31]);

		displayConfig.interlacingVectorLandscape180.X = FCString::Atof(*outStrArray[32]);
		displayConfig.interlacingVectorLandscape180.Y = FCString::Atof(*outStrArray[33]);
		displayConfig.interlacingVectorLandscape180.Z = FCString::Atof(*outStrArray[34]);
		displayConfig.interlacingVectorLandscape180.W = FCString::Atof(*outStrArray[35]);

		displayConfig.interlacingVectorPortrait.X = FCString::Atof(*outStrArray[36]);
		displayConfig.interlacingVectorPortrait.Y = FCString::Atof(*outStrArray[37]);
		displayConfig.interlacingVectorPortrait.Z = FCString::Atof(*outStrArray[38]);
		displayConfig.interlacingVectorPortrait.W = FCString::Atof(*outStrArray[39]);

		displayConfig.interlacingVectorPortrait180.X = FCString::Atof(*outStrArray[40]);
		displayConfig.interlacingVectorPortrait180.Y = FCString::Atof(*outStrArray[41]);
		displayConfig.interlacingVectorPortrait180.Z = FCString::Atof(*outStrArray[42]);
		displayConfig.interlacingVectorPortrait180.W = FCString::Atof(*outStrArray[43]);

		for (int32 index = 0; index < 4; index++)
		{
			displayConfig.interlacingMatrixLandscape.M[index][0] = FCString::Atof(*outStrArray[44 + index * 4]);
			displayConfig.interlacingMatrixLandscape.M[index][1] = FCString::Atof(*outStrArray[44 + index * 4 + 1]);
			displayConfig.interlacingMatrixLandscape.M[index][2] = FCString::Atof(*outStrArray[44 + index * 4 + 2]);
			displayConfig.interlacingMatrixLandscape.M[index][3] = FCString::Atof(*outStrArray[44 + index * 4 + 3]);

			displayConfig.interlacingMatrixLandscape180.M[index][0] = FCString::Atof(*outStrArray[44 + index * 4 + 16]);
			displayConfig.interlacingMatrixLandscape180.M[index][1] = FCString::Atof(*outStrArray[44 + index * 4 + 1 + 16]);
			displayConfig.interlacingMatrixLandscape180.M[index][2] = FCString::Atof(*outStrArray[44 + index * 4 + 2 + 16]);
			displayConfig.interlacingMatrixLandscape180.M[index][3] = FCString::Atof(*outStrArray[44 + index * 4 + 3 + 16]);

			displayConfig.interlacingMatrixPortrait.M[index][0] = FCString::Atof(*outStrArray[44 + index * 4 + 32]);
			displayConfig.interlacingMatrixPortrait.M[index][1] = FCString::Atof(*outStrArray[44 + index * 4 + 1 + 32]);
			displayConfig.interlacingMatrixPortrait.M[index][2] = FCString::Atof(*outStrArray[44 + index * 4 + 2 + 32]);
			displayConfig.interlacingMatrixPortrait.M[index][3] = FCString::Atof(*outStrArray[44 + index * 4 + 3 + 32]);

			displayConfig.interlacingMatrixPortrait180.M[index][0] = FCString::Atof(*outStrArray[44 + index * 4 + 48]);
			displayConfig.interlacingMatrixPortrait180.M[index][1] = FCString::Atof(*outStrArray[44 + index * 4 + 1 + 48]);
			displayConfig.interlacingMatrixPortrait180.M[index][2] = FCString::Atof(*outStrArray[44 + index * 4 + 2 + 48]);
			displayConfig.interlacingMatrixPortrait180.M[index][3] = FCString::Atof(*outStrArray[44 + index * 4 + 3 + 48]);
		}
	}

	return displayConfig;
}
