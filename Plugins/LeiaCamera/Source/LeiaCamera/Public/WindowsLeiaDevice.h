/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

#pragma once

#include "CoreMinimal.h"
#include "AbstractLeiaDevice.h"
#if PLATFORM_WINDOWS && LEIA_USE_SERVICE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

struct XyPairOfFloat {
	float x;
	float y;
};

struct XyPairOfInt {
	int x;
	int y;
};

struct VectorOfFloat {
	float* items;
	int size;
};

struct XyPairOfVectorOfFloat {
	struct VectorOfFloat x;
	struct VectorOfFloat y;
};

struct MatrixOfFloat {
	float** items;
	int xSize;
	int ySize;
};

struct DisplayConfigStruct 
{
	const char* displayClass;
	const char* displayId;
	float beta;
	XyPairOfFloat dotPitchInMm;
	XyPairOfInt panelResolution;
	XyPairOfInt numViews;
	XyPairOfFloat alignmentOffset;
	XyPairOfVectorOfFloat viewSharpeningCoefficients;
	int systemDisparity;
	MatrixOfFloat interlacingMatrix;
	VectorOfFloat interlacingVector;
	XyPairOfInt displaySizeInMm;
	XyPairOfInt viewResolution;
	XyPairOfVectorOfFloat viewSharpeningKernel;
	float convergenceDistance;
	float centerViewNumber;
	XyPairOfFloat viewboxSize;
	float gamma;
	bool isSlanted;
};

#if LEIA_USE_SERVICE
typedef void (WINAPI* PGNSI)(LPSYSTEM_INFO);
typedef void(__cdecl* MINT_VOID) (int); // a pointer to a process that takes 1 int arg and returns a void
typedef bool(__cdecl* MATOMIC_BOOL) (); // a pointer to a process that takes no args and returns a bool
typedef int(__cdecl* MATOMIC_INT) (); // a pointer to a process that takes no args and returns a int
typedef char* (__cdecl* MATOMIC_PCHAR) (); // a pointer to a process that takes no args and returns a char*
typedef int* (__cdecl* MATOMIC_PINT)(); // a pointer to a process that takes no args and returns an int*
typedef float* (__cdecl* MATOMIC_PFLOAT)(); // a pointer to a process that takes no args and returns a float*
typedef DisplayConfigStruct(__cdecl* MATOMIC_DCS)(); // a pointer to a process that takes no args and returns a DisplayConfigStruct
#endif

class LEIACAMERA_API WindowsLeiaDevice : public AbstractLeiaDevice
{
public:
	WindowsLeiaDevice();
	~WindowsLeiaDevice();

	bool IsConnected() override;
	void SetBacklightMode(BacklightMode modeId) override;
	BacklightMode GetBacklightMode() override;
	FDisplayConfig GetDisplayConfig() override;

private:
#if LEIA_USE_SERVICE
	HMODULE hmodDisplaySdkCpp;
	HMODULE hmodDisplayParams;
	MATOMIC_BOOL fPtrConnected = nullptr;
	MINT_VOID fPtrRequestBacklight = nullptr;
	MATOMIC_INT fPtrGetBacklightMode = nullptr;
	MATOMIC_DCS fPtrGetDisplayConfig = nullptr;
#endif
};
#undef WIN32_LEAN_AND_MEAN
