/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/


#include "LeiaCameraPawn.h"
#include "LeiaCamera.h"
#include "Interfaces/IPluginManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraComponent.h"
#include "Engine/SceneCapture2D.h"
#include "Components/LineBatchComponent.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "ScreenRendering.h"
#include "ClearQuad.h"

#include "XRRenderTargetManager.h"
#include "RendererInterface.h"
#include "CommonRenderResources.h"
#include "Serialization/MemoryLayout.h"
#include "Shader.h"
#include "Engine/UserInterfaceSettings.h"
#include "DefaultStereoLayers.h"
#include "IHeadMountedDisplay.h"
#include "HeadMountedDisplayBase.h"
#include "DefaultSpectatorScreenController.h"
#include "Templates/SharedPointer.h"



void AViewData::Serialize(FArchive & Ar)
{
	Super::Serialize(Ar);
}
// -----------------------------------------------

AViewData::AViewData(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
	sceneComp = CreateDefaultSubobject<UViewDataComp>(TEXT("LeiaViewDataComp"));
	sceneComp->SetupAttachment(RootComponent);
}

FVector2D GetGameViewportSize();
FVector2D GetGameResolution();

class FInterlacePS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FInterlacePS, Global, LEIACAMERA_API);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }

	FInterlacePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		texture_0.Bind(Initializer.ParameterMap, TEXT("texture_0"));
		texture_0Sampler.Bind(Initializer.ParameterMap, TEXT("texture_0Sampler"));

		EShaderParameterFlags flags = SPF_Mandatory;
		viewsX.Bind(Initializer.ParameterMap, TEXT("viewsX"), flags);
		viewsY.Bind(Initializer.ParameterMap, TEXT("viewsY"), SPF_Optional); //it's often assumed to always be 1
		viewResX.Bind(Initializer.ParameterMap, TEXT("viewResX"), flags);
		viewResY.Bind(Initializer.ParameterMap, TEXT("viewResY"), flags);
		viewOffset.Bind(Initializer.ParameterMap, TEXT("viewOffset"), SPF_Optional);

		interlace_vector.Bind(Initializer.ParameterMap, TEXT("interlace_vector"), flags);

		interlace_matrixA.Bind(Initializer.ParameterMap, TEXT("interlace_matrixA"), flags);
		interlace_matrixB.Bind(Initializer.ParameterMap, TEXT("interlace_matrixB"), flags);
		interlace_matrixC.Bind(Initializer.ParameterMap, TEXT("interlace_matrixC"), flags);
		interlace_matrixD.Bind(Initializer.ParameterMap, TEXT("interlace_matrixD"), flags);
	}
	FInterlacePS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FTexture* Texture)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), texture_0, texture_0Sampler, Texture);
	}

	void SetParameters(FRHICommandList& RHICmdList, FRHISamplerState* SamplerStateRHI, FRHITexture* TextureRHI)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), texture_0, texture_0Sampler, SamplerStateRHI, TextureRHI);
	}

	void SetParameters(FRHICommandList& RHICmdList, InterlaceParams const& interlaceParams)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), viewsX, interlaceParams.viewsX);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), viewsY, interlaceParams.viewsY);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), viewResX, interlaceParams.viewRes.R);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), viewResY, interlaceParams.viewRes.G);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), viewOffset, interlaceParams.viewOffset);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), interlace_vector, interlaceParams.intVecs);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), interlace_matrixA, interlaceParams.matA);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), interlace_matrixB, interlaceParams.matB);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), interlace_matrixC, interlaceParams.matC);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), interlace_matrixD, interlaceParams.matD);
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, texture_0);
	LAYOUT_FIELD(FShaderResourceParameter, texture_0Sampler);

	LAYOUT_FIELD(FShaderParameter, viewsX);
	LAYOUT_FIELD(FShaderParameter, viewsY);
	LAYOUT_FIELD(FShaderParameter, viewResX);
	LAYOUT_FIELD(FShaderParameter, viewResY);
	LAYOUT_FIELD(FShaderParameter, viewOffset);

	LAYOUT_FIELD(FShaderParameter, interlace_vector);

	LAYOUT_FIELD(FShaderParameter, interlace_matrixA);
	LAYOUT_FIELD(FShaderParameter, interlace_matrixB);
	LAYOUT_FIELD(FShaderParameter, interlace_matrixC);
	LAYOUT_FIELD(FShaderParameter, interlace_matrixD);
};

IMPLEMENT_SHADER_TYPE(, FInterlacePS, TEXT("/Plugin/LeiaCamera/InterlacePS.usf"), TEXT("MainPS"), SF_Pixel);


void computeSharpening2D(FLinearColor sharpeningShaderInputValues[18], int& sharpeningShaderInputValueCount, float& sharpeningShaderCenter, float& beta, float& gamma)
{
	const int actCount = 6;

	// These are the values that need to eventually be in the config and be pulled dynamically:
	beta = 2.0f;
	gamma = 2.0f;
	const float actValues[actCount] = { 0.096f, 0.009f, 0.017f, 0.008f, 0.008f, 0.004f };
	const float x_location[12] = { 0,  0, 0,  0, 1, -1, 1, -1, 1, -1, 2, -2 };
	const float y_location[12] = { 1, -1, 2, -2, 0,  0, 1, -1, 2, -2, 0,  0 };

	// Set how many of the 18 possible samples are actually used.
	sharpeningShaderInputValueCount = 12;

	// Compute normalizer from all act values and beta.
	float normalizer = 1.0f;
	for (int i = 0; i < actCount; i++)
		normalizer -= beta * actValues[i];

	// Set sharpening center.
	sharpeningShaderCenter = 1.0 / normalizer;

	// Compute shader sample inputs.
	for (int i = 0; i < sharpeningShaderInputValueCount; i++)
	{
		float x = x_location[i];
		float y = y_location[i];
		float z = actValues[i / 2] / normalizer;
		sharpeningShaderInputValues[i] = FLinearColor(x, y, z, 0.0f);
	}

	// Clear the unused sample inputs (if any) to zero.
	for (int i = sharpeningShaderInputValueCount; i < 18; i++)
		sharpeningShaderInputValues[i] = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// OVerride using JohnS values. For now disabled because he thinks there are probably wrong.
	const bool debugOverrideWithJohnSValues = false;
	if (debugOverrideWithJohnSValues)
	{
		sharpeningShaderCenter = 1.396648;
		sharpeningShaderInputValues[0] = FLinearColor(2.0f, 0.0f, 0.005586592f, 0.0f);
		sharpeningShaderInputValues[1] = FLinearColor(1.0f, -2.0f, 0.01117318f, 0.0f);
		sharpeningShaderInputValues[2] = FLinearColor(1.0f, -1.0f, 0.01117318f, 0.0f);
		sharpeningShaderInputValues[3] = FLinearColor(1.0f, 0.0f, 0.02374302f, 0.0f);
		sharpeningShaderInputValues[4] = FLinearColor(0.0f, -2.0f, 0.1340782f, 0.0f);
		sharpeningShaderInputValues[5] = FLinearColor(0.0f, -1.0f, 0.1340782f, 0.0f);
		sharpeningShaderInputValues[6] = FLinearColor(0.0f, 1.0f, 0.1340782f, 0.0f);
		sharpeningShaderInputValues[7] = FLinearColor(0.0f, 2.0f, 0.1340782f, 0.0f);
		sharpeningShaderInputValues[8] = FLinearColor(1.0f, 0.0f, 0.02374302f, 0.0f);
		sharpeningShaderInputValues[9] = FLinearColor(1.0f, 1.0f, 0.01117318f, 0.0f);
		sharpeningShaderInputValues[10] = FLinearColor(1.0f, 2.0f, 0.01117318f, 0.0f);
		sharpeningShaderInputValues[11] = FLinearColor(2.0f, 0.0f, 0.005586592f, 0.0f);
		sharpeningShaderInputValues[12] = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		sharpeningShaderInputValues[13] = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		sharpeningShaderInputValues[14] = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		sharpeningShaderInputValues[15] = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		sharpeningShaderInputValues[16] = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		sharpeningShaderInputValues[17] = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	/*
	Based on these inputs:

	"value"      : [0.096, 0.009, 0.017, 0.008, 0.008, 0.004] ,
	"x_location" : [0,  0, 0,  0, 1, -1, 1, -1, 1, -1, 2, -2] ,
	"y_location" : [1, -1, 2, -2, 0,  0, 1, -1, 2, -2, 0,  0] ,
													   X
	{ 0,  1, 0.096 -> 0.1340782}
	{ 0, -1, 0.096 -> 0.1340782}
	{ 0,  2, 0.009 -> 0.01256983}
	{ 0, -2, 0.009 -> 0.01256983}
	{ 1,  0, 0.017 -> 0.02374302}
	{-1,  0, 0.017 -> 0.02374302}
	{ 1,  1, 0.008 -> 0.01117318}
	{-1, -1, 0.008 -> 0.01117318}
	{ 1,  2, 0.008 -> 0.01117318}
	{-1, -2, 0.008 -> 0.01117318}
	{ 2,  0, 0.004 -> 0.005586592}
	{-2,  0, 0.004 -> 0.005586592}

	bVector[0] with beta 2 and gamma 2 is 1.396648 -> center
	bVector[1] with beta 2 and gamma 2 is 0.1340782
	bVector[2] with beta 2 and gamma 2 is 0.01256983
	bVector[3] with beta 2 and gamma 2 is 0.02374302
	bVector[4] with beta 2 and gamma 2 is 0.01117318
	bVector[5] with beta 2 and gamma 2 is 0.01117318
	bVector[6] with beta 2 and gamma 2 is 0.005586592
	*/
}

class FSharpenPS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FSharpenPS, Global, LEIACAMERA_API);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }

	FSharpenPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		texture_0.Bind(Initializer.ParameterMap, TEXT("texture_0"));
		texture_0Sampler.Bind(Initializer.ParameterMap, TEXT("texture_0Sampler"));

		EShaderParameterFlags flags = SPF_Optional;
		gamma.Bind(Initializer.ParameterMap, TEXT("gamma"), flags);
		sharpeningCenter.Bind(Initializer.ParameterMap, TEXT("sharpeningCenter"), flags);
		//sharpeningSize.Bind(Initializer.ParameterMap, TEXT("sharpeningSize"), flags);
		//sharpeningX.Bind(Initializer.ParameterMap, TEXT("sharpeningX"), flags);
		//sharpeningY.Bind(Initializer.ParameterMap, TEXT("sharpeningY"), flags);
		textureInvSize.Bind(Initializer.ParameterMap, TEXT("textureInvSize"), flags);
		sharpeningValues.Bind(Initializer.ParameterMap, TEXT("sharpeningValues"), flags);
		sharpeningValueCount.Bind(Initializer.ParameterMap, TEXT("sharpeningValueCount"), flags);
	}
	FSharpenPS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FTexture* Texture)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), texture_0, texture_0Sampler, Texture);
	}

	void SetParameters(FRHICommandList& RHICmdList, FRHISamplerState* SamplerStateRHI, FRHITexture* TextureRHI)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), texture_0, texture_0Sampler, SamplerStateRHI, TextureRHI);
	}

	void SetParameters(FRHICommandList& RHICmdList, SharpenParams const& sharpenParams)
	{
		// Hack-in new sharpening values. The calculation is actually pretty quick and it's called once per frame, so until
		// properly implemented, it shouldn't affect performance.
		FLinearColor sharpeningShaderInputValues[18];
		int          sharpeningShaderInputValueCount = 0;
		float		 sharpeningShaderCenter = 0;
		float        sharpeningShaderBeta = 2.0;
		float        sharpeningShaderGamma = 2.0;
		//computeSharpening2D(sharpeningShaderInputValues, sharpeningShaderInputValueCount, sharpeningShaderCenter, sharpeningShaderBeta, sharpeningShaderGamma);

		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), gamma, sharpeningShaderGamma);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningCenter, sharpeningShaderCenter);
		//SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningSize, sharpenParams.sharpeningSize);
		//SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningX, sharpenParams.sharpeningX);
		//SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningY, sharpenParams.sharpeningY);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), textureInvSize, sharpenParams.textureInvSize);
		SetShaderValueArray(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningValues, sharpeningShaderInputValues, 18);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningValueCount, float(sharpeningShaderInputValueCount));
	}

private:
	LAYOUT_FIELD(FShaderResourceParameter, texture_0);
	LAYOUT_FIELD(FShaderResourceParameter, texture_0Sampler);
	LAYOUT_FIELD(FShaderParameter, gamma);
	LAYOUT_FIELD(FShaderParameter, sharpeningCenter);
	LAYOUT_FIELD(FShaderParameter, textureInvSize);
	//LAYOUT_FIELD(FShaderParameter, sharpeningX);
	//LAYOUT_FIELD(FShaderParameter, sharpeningY);
	//LAYOUT_FIELD(FShaderParameter, sharpeningSize);
	LAYOUT_FIELD(FShaderParameter, sharpeningValues);
	LAYOUT_FIELD(FShaderParameter, sharpeningValueCount);
};

IMPLEMENT_SHADER_TYPE(, FSharpenPS, TEXT("/Plugin/LeiaCamera/SharpenPS.usf"), TEXT("MainPS"), SF_Pixel);


TSharedPtr<class FLeiaStereoRenderingDevice, ESPMode::ThreadSafe> GFakeStereoDevice;

class FLeiaStereoLayers : public FDefaultStereoLayers
{
public:
	FLeiaStereoLayers(const class FAutoRegister& AutoRegister, class FHeadMountedDisplayBase* InHmd)
		: FDefaultStereoLayers(AutoRegister, InHmd)
		, DefaultX(0)
		, DefaultY(0)
		, DefaultZ(100)
		, DefaultWidth(1280)
		, DefaultHeight(720)
	{
	}

public:
	virtual bool ShouldCopyDebugLayersToSpectatorScreen() const override { return false; }
	//~ IStereoLayers interface
	virtual FLayerDesc GetDebugCanvasLayerDesc(FTextureRHIRef Texture) override;


protected:
	const float DefaultX;
	const float DefaultY;
	const float DefaultZ;
	const float DefaultWidth;
	const float DefaultHeight;
};


class FLeiaStereoRenderingDevice : public FHeadMountedDisplayBase, public FXRRenderTargetManager
{
public:

	ALeiaCameraPawn * leiaPawn = NULL;

	FLeiaStereoRenderingDevice(int32 inNumViews = 8, int32 inViewResolutionX = 1280, int32 inViewResolutionY = 720, int32 inPanelResolutionX = 3840, int32 inPanelResolutionY = 2160)
		: FHeadMountedDisplayBase(NULL)
		, leiaPawn(NULL)
		, FOVInDegrees(60)
	{
		NumViews = inNumViews;
		Width = inViewResolutionX;
		Height = inViewResolutionY;
		PanelResolutionX = inPanelResolutionX;
		PanelResolutionY = inPanelResolutionY;

		static TAutoConsoleVariable<float> CVarEmulateStereoFOV(TEXT("r.StereoEmulationFOV"), 0, TEXT("FOV in degrees, of the imaginable HMD for stereo emulation"));
		static TAutoConsoleVariable<int32> CVarEmulateStereoWidth(TEXT("r.StereoEmulationWidth"), 0, TEXT("Width of the imaginable HMD for stereo emulation"));
		static TAutoConsoleVariable<int32> CVarEmulateStereoHeight(TEXT("r.StereoEmulationHeight"), 0, TEXT("Height of the imaginable HMD for stereo emulation"));
		static TAutoConsoleVariable<int32> CVarEmulateStereoNumViews(TEXT("r.StereoEmulationNumViews"), 0, TEXT("Number of views, of the imaginable HMD for stereo emulation"));
		float FOV = CVarEmulateStereoFOV.GetValueOnAnyThread();
		if (FOV != 0)
		{
			FOVInDegrees = FMath::Clamp(FOV, 20.f, 300.f);
		}
		int32 W = CVarEmulateStereoWidth.GetValueOnAnyThread();
		int32 H = CVarEmulateStereoHeight.GetValueOnAnyThread();
		int32 V = CVarEmulateStereoNumViews.GetValueOnAnyThread();
		if (W != 0)
		{
			Width = FMath::Clamp(W, 100, 10000);
		}
		if (H != 0)
		{
			Height = FMath::Clamp(H, 100, 10000);
		}
		if (V != 0)
		{
			NumViews = FMath::Clamp(V, 0, 32);
		}
	}
#if LEIA_IS_UE5
	virtual FTexture2DRHIRef GetOverlayLayerTarget_RenderThread(int32 viewIndex, FIntRect& InOutViewport) override
#else
	virtual FTexture2DRHIRef GetOverlayLayerTarget_RenderThread(EStereoscopicPass StereoPass, FIntRect& InOutViewport) override
#endif
	{
		return  OverlayTargetableTexture;
	}

	virtual TSharedPtr< class IStereoRendering, ESPMode::ThreadSafe > GetStereoRenderingDevice() override
	{
		return GFakeStereoDevice;
	}

	virtual class IHeadMountedDisplay* GetHMDDevice() override
	{
		return  this;
	}

	virtual FIntPoint GetIdealDebugCanvasRenderTargetSize() const override
	{
		if (leiaPawn == NULL)
		{
			return  FIntPoint(1280, 720);
		}
		//InOutSizeX = leiaPawn->ConstructionInfo.RenderTextureResolutionWidth * NumViews;
		//InOutSizeY = leiaPawn->ConstructionInfo.RenderTextureResolutionHeight;
		//3840, 2160
		//return FIntPoint(leiaPawn->ConstructionInfo.RenderTextureResolutionWidth * NumViews, leiaPawn->ConstructionInfo.RenderTextureResolutionHeight);
		return FIntPoint(1280, 720); // replace with backbuffer size
	}
	virtual ~FLeiaStereoRenderingDevice() {}

	virtual bool IsStereoEnabled() const override { return (leiaPawn != NULL); }

	virtual bool EnableStereo(bool stereo = true) override { return (leiaPawn != NULL); }

	virtual int32 GetDesiredNumberOfViews(bool bStereoRequested) const { return (bStereoRequested && leiaPawn != NULL) ? NumViews : 1; }
#if LEIA_IS_UE5
	virtual EStereoscopicPass GetViewPassForIndex(bool bStereoRequested, int32 ViewIndex) const
#else
	virtual EStereoscopicPass GetViewPassForIndex(bool bStereoRequested, uint32 ViewIndex) const
#endif
	{
		if (!bStereoRequested)
			return EStereoscopicPass::eSSP_FULL;

#if LEIA_IS_UE5
		return static_cast<EStereoscopicPass>((int)EStereoscopicPass::eSSP_PRIMARY + ViewIndex);
#else
		return static_cast<EStereoscopicPass>(eSSP_LEFT_EYE + ViewIndex);
#endif

	}

#if !LEIA_IS_UE5
	virtual uint32 GetViewIndexForPass(EStereoscopicPass StereoPassType) const
	{
		switch (StereoPassType)
		{
		case eSSP_LEFT_EYE:
		case eSSP_FULL:
			return 0;

		case eSSP_RIGHT_EYE:
			return 1;

		case eSSP_LEFT_EYE_SIDE:
			return 2;

		case eSSP_RIGHT_EYE_SIDE:
			return 3;

		default:
		{
			//UE_LOG(LogLeia, Log, TEXT("View index Default Hit %d"), StereoPassType); Commented-out because it's working correctly and spams the log making it hard to notice messages
			return StereoPassType - eSSP_LEFT_EYE;
		}
		}
	}
#endif


	virtual bool DeviceIsAPrimaryPass(EStereoscopicPass Pass)
	{
#if LEIA_IS_UE5
		return Pass == EStereoscopicPass::eSSP_FULL || Pass == EStereoscopicPass::eSSP_PRIMARY;
#else
		return Pass == EStereoscopicPass::eSSP_FULL || Pass == EStereoscopicPass::eSSP_LEFT_EYE;
#endif		
		//return Pass == EStereoscopicPass::eSSP_FULL || (GetViewIndexForPass(Pass) % 2 == 0);
	}

#if !LEIA_IS_UE5
	virtual bool DeviceIsAPrimaryView(const FSceneView& View) override
	{
#if WITH_MGPU
		// We have to do all the work for secondary views that are on a different GPU than
		// the primary view. NB: This assumes that the primary view is assigned to the
		// first GPU of the AFR group. See FSceneRenderer::ComputeViewGPUMasks.
		if (DeviceIsStereoEyeView(View) && AFRUtils::GetIndexWithinGroup(View.GPUMask.ToIndex()) != 0)
		{
			return true;
		}
#endif
		return DeviceIsAPrimaryPass(View.StereoPass);
	}
#endif

#if LEIA_IS_UE5
	virtual void AdjustViewRect(const int32 currX, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override
#else
	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override
#endif
	{
		SizeX = Width;
		SizeY = Height;
#if !LEIA_IS_UE5
		int currX = GetViewIndexForPass(StereoPass);
#endif
		X += SizeX * currX;
	}

#if LEIA_IS_UE5
	virtual void CalculateStereoViewOffset(const int32 viewIndex, FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) override
#else
	virtual void CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) override
#endif
	{
#if !LEIA_IS_UE5
		if (StereoPassType == eSSP_FULL)
		{
			return;
		}
		auto viewIndex = GetViewIndexForPass(StereoPassType);;
#endif

#if LEIA_IS_UE5

		if (viewIndex < 0)
		{
			return;
		}


		if (leiaPawn && viewIndex < leiaPawn->Cameras.Num())
#else
		if (leiaPawn && viewIndex < (unsigned)leiaPawn->Cameras.Num())
#endif
		{
			auto CapComp = ((CaptureActor*)leiaPawn->Cameras[viewIndex])->GetCaptureComponent2D();

			ViewLocation = CapComp->GetComponentLocation();
		}
	}

	virtual bool DeviceIsAnAdditionalView(const FSceneView& View)
	{
		return (int)View.StereoPass > NumViews;
	}

	virtual bool DeviceIsASecondaryView(const FSceneView& View)
	{
#if LEIA_IS_UE5
		return !DeviceIsAPrimaryPass(View.StereoPass);
#else		
		return !DeviceIsAPrimaryView(View);
#endif
	}

	void GetEyeRenderParams_RenderThread(const FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override
	{
	}

#if LEIA_IS_UE5
	virtual FMatrix GetStereoProjectionMatrix(const int32 viewIndex) const override
#else
	virtual FMatrix GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const override
#endif
	
	{
#if !LEIA_IS_UE5
		const int viewIndex = GetViewIndexForPass(StereoPassType);
#endif


		if (leiaPawn && leiaPawn->Cameras.IsValidIndex(viewIndex) && leiaPawn->Cameras[viewIndex] != nullptr) //viewIndex
		{
			return ((CaptureActor *)leiaPawn->Cameras[viewIndex])->GetCaptureComponent2D()->CustomProjectionMatrix;
		}
		else
		{
			const float HalfFov = FMath::DegreesToRadians(FOVInDegrees) / 2.f;
			const float InWidth = Width;
			const float InHeight = Height;
			const float XS = 1.0f / FMath::Tan(HalfFov);
			const float YS = InWidth / FMath::Tan(HalfFov) / InHeight;
			const float NearZ = GNearClippingPlane;

			return FMatrix(
				FPlane(XS, 0.0f, 0.0f, 0.0f),
				FPlane(0.0f, YS, 0.0f, 0.0f),
				FPlane(0.0f, 0.0f, 0.0f, 1.0f),
				FPlane(0.0f, 0.0f, NearZ, 0.0f));
		}
	}

	virtual void InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) override
	{
	}

	virtual IStereoRenderTargetManager* GetRenderTargetManager() override
	{
		return this;
	}

	virtual bool AllocateRenderTargetTexture(uint32 Index, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, ETextureCreateFlags Flags, ETextureCreateFlags TargetableTextureFlags, FTexture2DRHIRef& OutTargetableTexture, FTexture2DRHIRef& OutShaderResourceTexture, uint32 NumSamples = 1) override
	{
		if (!IsStereoEnabled())
		{
			return false;
		}

		//maybe pass in Format
		ETextureRenderTargetFormat RTFormat = ETextureRenderTargetFormat::RTF_RGBA32f;

		FRHIResourceCreateInfo CreateInfo(TEXT("Leia Intermediate Megatexture"));
		RHICreateTargetableShaderResource2D(SizeX, SizeY, GetPixelFormatFromRenderTargetFormat(RTFormat), 1, TexCreate_None, TexCreate_RenderTargetable, false, CreateInfo, OutTargetableTexture, OutShaderResourceTexture);

		FRHIResourceCreateInfo CreateInfo2(TEXT("Leia Staging Target"));
		RHICreateTargetableShaderResource2D(PanelResolutionX, PanelResolutionY, GetPixelFormatFromRenderTargetFormat(RTF_RGBA8), 1, TexCreate_None, TexCreate_RenderTargetable, false, CreateInfo2, StagingTargetableTexture, StagingShaderResourceTexture);

		FRHIResourceCreateInfo CreateInfo3(TEXT("Leia Overlay Target"));
		RHICreateTargetableShaderResource2D(PanelResolutionX, PanelResolutionY, GetPixelFormatFromRenderTargetFormat(RTFormat), 1, TexCreate_None, TexCreate_RenderTargetable, false, CreateInfo3, OverlayTargetableTexture, OverlayShaderResourceTexture);

		return true;
	}

	/**
 * Updates viewport for direct rendering of distortion. Should be called on a game thread.
 *
 * @param bUseSeparateRenderTarget	Set to true if a separate render target will be used. Can potentiallt be true even if ShouldUseSeparateRenderTarget() returned false earlier.
 * @param Viewport					The Viewport instance calling this method.
 * @param ViewportWidget			(optional) The Viewport widget containing the view. Can be used to access SWindow object.
 */
	virtual void UpdateViewport(bool bUseSeparateRenderTarget, const class FViewport& Viewport, class SViewport* ViewportWidget = nullptr) override
	{

	}

	/**
	 * Calculates dimensions of the render target texture for direct rendering of distortion.
	 * This implementation calculates the size based on the current value of vr.pixeldensity.
	 */
	virtual void CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) override
	{
		InOutSizeX = Width * NumViews;
		InOutSizeY = Height;
	}

	/**
	* Returns true, if render target texture must be re-calculated.
	*/
	virtual bool NeedReAllocateViewportRenderTarget(const class FViewport& Viewport) override
	{
		return false;
	}

	virtual void UpdateViewportRHIBridge(bool bUseSeparateRenderTarget, const class FViewport& Viewport, FRHIViewport* const ViewportRHI) override
	{
	}

	virtual bool ShouldUseSeparateRenderTarget() const override
	{
		return IsStereoEnabled();
	}

	virtual void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture2D* BackBuffer, FRHITexture2D* SrcTexture, FVector2D WindowSize) const override
	{
		check(IsInRenderingThread());

		bool debugAreOn12VDisplay = NumViews == 12;
		bool debugEnableSharpening = false;

		const FVector2D res = GetGameResolution();

		{
			FRHITexture2D* RenderTarget = BackBuffer;
			if (debugEnableSharpening) {
				RenderTarget = StagingTargetableTexture;
			}


			// The debug viewport is the mirror window (if any).
			const uint32 ViewportWidth = RenderTarget->GetSizeX();
			const uint32 ViewportHeight = RenderTarget->GetSizeY();

			FRHIRenderPassInfo RPInfo(RenderTarget, ERenderTargetActions::Clear_Store);
			RHICmdList.BeginRenderPass(RPInfo, TEXT("Leia Interlacing (RenderTexture_RenderThread)"));

			FRHITransitionInfo Transitions[] = {
				FRHITransitionInfo(SrcTexture, ERHIAccess::Unknown, ERHIAccess::SRVMask),
				FRHITransitionInfo(RenderTarget, ERHIAccess::Unknown, ERHIAccess::RTV)
			};
			RHICmdList.Transition(MakeArrayView(Transitions, UE_ARRAY_COUNT(Transitions)));


			DrawClearQuad(RHICmdList, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));

			RHICmdList.SetViewport(0, 0, 0, ViewportWidth, ViewportHeight, 1.0f);


			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			const auto FeatureLevel = GMaxRHIFeatureLevel;
			auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
			TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
			TShaderMapRef<FInterlacePS> PixelShader(ShaderMap);

			GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(), SrcTexture);
			PixelShader->SetParameters(RHICmdList, leiaPawn->interlaceShaderParams);

			leiaPawn->RendererModule->DrawRectangle(
				RHICmdList,
				0, 0,
				ViewportWidth, ViewportHeight,
				0.0f, 0.0f,
				1.0f, 1.0f,
				FIntPoint(ViewportWidth, ViewportHeight),
				FIntPoint(1, 1),
				VertexShader,
				EDRF_Default);


			RHICmdList.EndRenderPass();
		}

		//Now Do ZDP + Sharpening
		if(debugEnableSharpening)
		{
			// The debug viewport is the mirror window (if any).
			const uint32 ViewportWidth = BackBuffer->GetSizeX();
			const uint32 ViewportHeight = BackBuffer->GetSizeY();

			FRHIRenderPassInfo RPInfo(BackBuffer, ERenderTargetActions::Clear_Store);
			RHICmdList.BeginRenderPass(RPInfo, TEXT("Leia Sharpening (RenderTexture_RenderThread)"));

			FRHITransitionInfo Transitions[] = {
				FRHITransitionInfo(StagingTargetableTexture, ERHIAccess::Unknown, ERHIAccess::SRVMask),
				FRHITransitionInfo(BackBuffer, ERHIAccess::Unknown, ERHIAccess::RTV)
			};
			RHICmdList.Transition(MakeArrayView(Transitions, UE_ARRAY_COUNT(Transitions)));

			DrawClearQuad(RHICmdList, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));

			RHICmdList.SetViewport(0, 0, 0, BackBuffer->GetSizeX(), BackBuffer->GetSizeY(), 1.0f);

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			const auto FeatureLevel = GMaxRHIFeatureLevel;
			auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
			TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
			TShaderMapRef<FSharpenPS> PixelShader(ShaderMap);

			GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(), StagingShaderResourceTexture);
			PixelShader->SetParameters(RHICmdList, leiaPawn->sharpenShaderParams);

			leiaPawn->RendererModule->DrawRectangle(
				RHICmdList,
				0, 0,
				ViewportWidth, ViewportHeight,
				0.0f, 0.0f,
				1.0f, 1.0f,
				FIntPoint(ViewportWidth, ViewportHeight),
				FIntPoint(1, 1),
				VertexShader,
				EDRF_Default);

			RHICmdList.EndRenderPass();
		}

		//Render Overlay/Eye Locked textures AFTER Leia Post Stack directly to 'back buffer'
		bool renderOverlays = true;
		if (renderOverlays)
		{
			FRHIRenderPassInfo RPInfo(BackBuffer, ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(RPInfo, TEXT("RenderTexture_RenderThread_Overlays"));

			const uint32 ViewportWidth = BackBuffer->GetSizeX();
			const uint32 ViewportHeight = BackBuffer->GetSizeY();

			const auto FeatureLevel = GMaxRHIFeatureLevel;
			auto ShaderMap = GetGlobalShaderMap(FeatureLevel);


			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);


			TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
			TShaderMapRef<FScreenPS> PixelShader(ShaderMap);

			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_Zero, BF_One>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			auto HMDDevice = GEngine->StereoRenderingDevice;
			auto StereoLayers = HMDDevice->GetStereoLayers();

			if (StereoLayers && OverlayShaderResourceTexture)
			{
				const FIntRect DstRect(0, 0, BackBuffer->GetSizeX(), BackBuffer->GetSizeY());
				PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), OverlayShaderResourceTexture);

				leiaPawn->RendererModule->DrawRectangle(
					RHICmdList,
					0, 0,
					ViewportWidth, ViewportHeight,
					0.0f, 0.0f,
					1.0f, 1.0f,
					FIntPoint(ViewportWidth, ViewportHeight),
					FIntPoint(1, 1),
					VertexShader,
					EDRF_Default);
			}

			RHICmdList.EndRenderPass();
		}

		// The debug viewport is the mirror window (if any).
		const uint32 ViewportWidth = BackBuffer->GetSizeX();
		const uint32 ViewportHeight = BackBuffer->GetSizeY();
		RHICmdList.SetViewport(0, 0, 0, ViewportWidth, ViewportHeight, 1.0f);

	}

	mutable TSharedPtr<class FDefaultStereoLayers, ESPMode::ThreadSafe> DefaultStereoLayers;

	IStereoLayers* GetStereoLayers() override
	{
		if (!DefaultStereoLayers.IsValid())
		{
			TSharedPtr<FLeiaStereoLayers, ESPMode::ThreadSafe> NewLayersPtr = FSceneViewExtensions::NewExtension<FLeiaStereoLayers>(this);
			DefaultStereoLayers = StaticCastSharedPtr<FDefaultStereoLayers>(NewLayersPtr);
		}
		return DefaultStereoLayers.Get();
	}


	virtual FName GetSystemName() const override
	{
		return TEXT("LeiaDevice");
	}

	int32 GetXRSystemFlags() const override { return 0;  }
	bool EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type /*= EXRTrackedDeviceType::Any*/) { return false;  };
	bool GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition) override { return false;  }

	float GetWorldToMetersScale() const override { return 100.0f; }
	void ResetOrientationAndPosition(float yaw = 0.f) override {  }
	void ResetOrientation(float Yaw = 0.f) override {}
	bool IsHMDConnected() override { return (leiaPawn != NULL);  }
	bool IsHMDEnabled() const override { return true;  }
	void EnableHMD(bool allow = true) override {}
	bool GetHMDMonitorInfo(MonitorInfo&) override { return false; }
	void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override {}
	void SetInterpupillaryDistance(float NewInterpupillaryDistance) override {}
	float GetInterpupillaryDistance() const override { return 0.0f;  }
	virtual bool GetHMDDistortionEnabled(EShadingPath ShadingPath) const override { return false;  }
	bool IsChromaAbCorrectionEnabled() const override { return false; };

	float			 FOVInDegrees;					// max(HFOV, VFOV) in degrees of imaginable HMD
	int32			 Width;							// Render-target width
	int32			 Height;						// Render-target height
	int32			 PanelResolutionX;				// Panel horizontal resolution.
	int32			 PanelResolutionY;				// Panel vertical resolution.
	int32			 NumViews;						// Number of views.
	FTexture2DRHIRef StagingTargetableTexture;		// Temporary render-target used as output of interlacing shader and input of sharpening shader
	FTexture2DRHIRef StagingShaderResourceTexture;	// Temporary render-target used as output of interlacing shader and input of sharpening shader
	FTexture2DRHIRef OverlayTargetableTexture;		// Temporary render-target used as output of interlacing shader and input of sharpening shader
	FTexture2DRHIRef OverlayShaderResourceTexture;	// Temporary render-target used as output of interlacing shader and input of sharpening shader
};


// Sets default values
ALeiaCameraPawn::ALeiaCameraPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PostUpdateWork;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(FName(TEXT("Scene Root")));
	SetRootComponent(SceneRoot);

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ALeiaCameraPawn::SetupPlayerInputComponent(UInputComponent* InputComp)
{
	Super::SetupPlayerInputComponent(InputComp);

	InputComp->BindAction("ToggleACT", IE_Released, this, &ALeiaCameraPawn::ToggleACT);

}

void ALeiaCameraPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Device->SetBacklightMode(BacklightMode::MODE_2D);
}

void ALeiaCameraPawn::ToggleACT()
{
	useACT = !useACT;

	GEngine->AddOnScreenDebugMessage(123213, 5.0, FColor::Cyan, FString::Printf(TEXT("ACT %s"), useACT ? TEXT("ON") : TEXT("OFF")), true, FVector2D(2.0, 2.0));
}


void ALeiaCameraPawn::SetDeviceOverride()
{
	EViewOverrideMode overrideMode = EViewOverrideMode::LumePad;
	switch (DeviceConfigOverrideMode)
	{
	case EViewMode::LumePad:
		overrideMode = EViewOverrideMode::LumePad;
		break;
	case EViewMode::Windows_12p5_8V:
		overrideMode = EViewOverrideMode::Windows_12p5_8V;
		break;
	case EViewMode::Windows_15p6_12V:
		overrideMode = EViewOverrideMode::Windows_15p6_12V;
		break;
	case EViewMode::Windows_15p6_13V:
		overrideMode = EViewOverrideMode::Windows_15p6_13V;
		break;
	case EViewMode::AndroidPegasus_12p5_8V:
		overrideMode = EViewOverrideMode::AndroidPegasus_12p5_8V;
		break;
	case EViewMode::AndroidPegasus_12p3_8V:
		overrideMode = EViewOverrideMode::AndroidPegasus_12p3_8V;
		break;
	case EViewMode::None:
		overrideMode = EViewOverrideMode::None;
		break;
	default:
		overrideMode = EViewOverrideMode::LumePad;
		break;
	}
	Device->SetOverride(overrideMode);
}

void ALeiaCameraPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UWorld* const world = GetWorld();
	if (world == nullptr)
	{
		return;
	}

	SetDeviceOverride();

	SetConstructionInfo(ConstructionInfo);
}

void ALeiaCameraPawn::OnScreenOrientationChanged(EScreenOrientation::Type type)
{
	type = EScreenOrientation::LandscapeRight;
	CurrentScreenOrientation = type;

	if (type == EScreenOrientation::Portrait || type == EScreenOrientation::PortraitUpsideDown)
	{
		FDisplayConfig displayConfig = Device->GetDisplayConfig();
		SetACTCoEfficents(displayConfig.actCoefficients[0], displayConfig.actCoefficients[1]);

		FLeiaCameraConstructionInfo constructionInfo;
		constructionInfo.GridWidth = ConstructionInfo.GridWidth;
		constructionInfo.RenderTextureResolutionWidth = ConstructionInfo.RenderTextureResolutionHeight;
		constructionInfo.RenderTextureResolutionHeight = ConstructionInfo.RenderTextureResolutionWidth;

		CreateCameraGrid(constructionInfo);
	}
	else if (type == EScreenOrientation::LandscapeLeft || type == EScreenOrientation::LandscapeRight || type == EScreenOrientation::Unknown)
	{
		FDisplayConfig displayConfig = Device->GetDisplayConfig();
		SetACTCoEfficents(displayConfig.actCoefficients[2], displayConfig.actCoefficients[3]);
		CreateCameraGrid(ConstructionInfo);
	}

	GetInterlaceParams(type, interlaceShaderParams);
	SetInterlaceMaterialParams(type);
	GetViewSharpeningParams(Device->GetDisplayConfig(), sharpenShaderParams);
	SetViewSharpeningMaterialParams();

	if (Cameras.Num() == 0)
	{
		CreateCameraGrid(ConstructionInfo);
	}

#if !LEIA_STEREO_PATH //TODO We need to calculate zdp stuff to use later for Stereo Path, similar to interlace params
	if (ZdpInfo.bUseZdpShear)
	{

		for (int camIndex = 0; camIndex < Cameras.Num(); ++camIndex)
		{
			CaptureComponent* const camComponent = Cast<CaptureComponent>(Cameras[camIndex]->GetComponentByClass(CaptureComponent::StaticClass()));

			if (MatInstancesDynamicZDP.Num() == Cameras.Num() && MatInstancesDynamicZDP[camIndex] != nullptr)
			{
				camComponent->PostProcessSettings.AddBlendable(MatInstancesDynamicZDP[camIndex], 1.0f);
			}
			else
			{
				UMaterialInstanceDynamic* const zdpMatInst = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialZDP, this);
				MatInstancesDynamicZDP.Add(zdpMatInst);
				camComponent->PostProcessSettings.AddBlendable(zdpMatInst, 1.0f);
			}

			camComponent->PostProcessSettings.AddBlendable(MatInstanceDynamicPositiveDOF, 1.0f);
		}
	}


	for (int32 camIndex = 0; camIndex < ConstructionInfo.GridWidth; camIndex++)
	{
		FString paramName = "CamInput_";
		paramName.AppendInt(camIndex);

		MatInstanceDynamicViewInterlacing->SetTextureParameterValue(*paramName, ((CaptureActor*)Cameras[camIndex])->GetCaptureComponent2D()->TextureTarget);
	}

#endif

#if LEIA_STEREO_PATH
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 75.0");
#else
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 100.0");
#endif
	//FString setResCommand = FString::Printf(TEXT("setres %dx%d"), 2304, 900);
	//UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), setResCommand);
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "r.MobileContentScaleFactor 4.0");

	RefreshCameraGrid();
}

void ALeiaCameraPawn::AppEnterBackground()
{
	Device->DesiredLightFieldMode = Device->GetBacklightMode();
	Device->SetBacklightMode(BacklightMode::MODE_2D);
}

void ALeiaCameraPawn::AppEnterForeground()
{
	Device->SetBacklightMode(Device->DesiredLightFieldMode);
	Device->DesiredLightFieldMode = BacklightMode::MODE_3D;
}

// Called when the game starts or when spawned
void ALeiaCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	SetDeviceOverride();

	SetConstructionInfo(ConstructionInfo);

	CommonMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(CommmonMatParamCollection);
	InterlaceMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(InterlaceMatParamCollection);
	ViewSharpeningMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(ViewSharpeningeMatParamCollection);

	MatInstanceDynamicPositiveDOF = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialPositiveDOF, this);
	MatInstanceDynamicViewInterlacing = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialViewInterlacing, this);
	MatInstanceDynamicViewSharpening = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialViewSharpening, this);

	UPlatformGameInstance* const platformGameInst = Cast<UPlatformGameInstance>(GetGameInstance());
	if (platformGameInst != nullptr && platformGameInst->IsValidLowLevel())
	{
		platformGameInst->ApplicationReceivedScreenOrientationChangedNotificationDelegate.AddDynamic(this, &ALeiaCameraPawn::OnScreenOrientationChanged);
		platformGameInst->ApplicationWillEnterBackgroundDelegate.AddDynamic(this, &ALeiaCameraPawn::AppEnterBackground);
		platformGameInst->ApplicationHasEnteredForegroundDelegate.AddDynamic(this, &ALeiaCameraPawn::AppEnterForeground);
		platformGameInst->ApplicationWillDeactivateDelegate.AddDynamic(this, &ALeiaCameraPawn::AppEnterBackground);
		platformGameInst->ApplicationHasReactivatedDelegate.AddDynamic(this, &ALeiaCameraPawn::AppEnterForeground);
		platformGameInst->ApplicationWillTerminateDelegate.AddDynamic(this, &ALeiaCameraPawn::AppEnterBackground);
	}

	OnScreenOrientationChanged(UBlueprintPlatformLibrary::GetDeviceOrientation());

#if !LEIA_STEREO_PATH
	InitialPostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
	//had to disable , its causing really bad artifacting right now
	//InitialPostProcessSettings.AddBlendable(MatInstanceDynamicViewSharpening, 1.0f);


	if (TargetCamera != nullptr)
	{
		UCameraComponent* const TargetCameraComponent_Temp_Camera = Cast<UCameraComponent>(TargetCamera->GetComponentByClass(UCameraComponent::StaticClass()));
		if (TargetCameraComponent_Temp_Camera != nullptr)
		{
			TargetCameraComponent = TargetCameraComponent_Temp_Camera;
			CachedPostProcessSettings = TargetCameraComponent_Temp_Camera->PostProcessSettings;
			TargetCameraComponent_Temp_Camera->PostProcessSettings = InitialPostProcessSettings;
			TargetCameraComponent_Temp_Camera->PostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
			//TargetCameraComponent_Temp_Camera->PostProcessSettings.AddBlendable(MatInstanceDynamicViewSharpening, 1.0f);
		}

	}

	for (int camIndex = 0; camIndex < Cameras.Num(); ++camIndex)
	{
		CaptureComponent* const camComponent = Cast<CaptureComponent>(Cameras[camIndex]->GetComponentByClass(CaptureComponent::StaticClass()));

		if (MatInstancesDynamicZDP.Num() == Cameras.Num() && MatInstancesDynamicZDP[camIndex] != nullptr)
		{

			camComponent->PostProcessSettings.AddBlendable(MatInstancesDynamicZDP[camIndex], 1.0f);
		}
		else
		{
			UMaterialInstanceDynamic* const zdpMatInst = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialZDP, this);
			MatInstancesDynamicZDP.Add(zdpMatInst);
			camComponent->PostProcessSettings.AddBlendable(zdpMatInst, 1.0f);
		}

		camComponent->PostProcessSettings.AddBlendable(MatInstanceDynamicPositiveDOF, 1.0f);
	}
#endif

#if LEIA_STEREO_PATH
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 75.0");
#else
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 100.0");
#endif
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "r.MobileContentScaleFactor 4.0");

	Device->SetBacklightMode(BacklightMode::MODE_3D);

	//Need this to render full screen quad
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

#if LEIA_STEREO_PATH
	const FDisplayConfig config = Device->GetDisplayConfig();

	if (GEngine && GEngine->StereoRenderingDevice)
	{
		auto stereoDevice = static_cast<FLeiaStereoRenderingDevice*>(GEngine->StereoRenderingDevice.Get());
		stereoDevice->NumViews = ConstructionInfo.GridWidth;
		stereoDevice->Width = ConstructionInfo.RenderTextureResolutionWidth;
		stereoDevice->Height = ConstructionInfo.RenderTextureResolutionHeight;
		stereoDevice->PanelResolutionX = config.panelResolution[0];
		stereoDevice->PanelResolutionY = config.panelResolution[1];

		stereoDevice->leiaPawn = this;
		GEngine->StereoRenderingDevice->EnableStereo(true);
	}

#endif

#if !WITH_EDITOR
	bDisplayFrustum = false;
#endif
}

void ALeiaCameraPawn::RefreshCameraGrid()
{
	float wOffset;
	CalculateGridOffset(ConstructionInfo, wOffset);

	if (Cameras.Num() > 0)
	{
		for (int wIndex = 0; wIndex < ConstructionInfo.GridWidth; wIndex++)
		{
			if (Cameras.IsValidIndex(wIndex) && Cameras[wIndex] != nullptr)
			{
				auto sceneCaptureComponent = ((CaptureActor*)Cameras[wIndex])->GetCaptureComponent2D();
				if (sceneCaptureComponent != nullptr)
				{
#if LEIA_STEREO_PATH
					sceneCaptureComponent->CustomProjectionMatrix = CalculateProjectionMatrix(ConstructionInfo, RenderingInfo,
						Cameras[wIndex]->GetRootComponent()->GetRelativeLocation());
#else
					sceneCaptureComponent->CustomProjectionMatrix = CalculateProjectionMatrix(ConstructionInfo, RenderingInfo,
						Cameras[wIndex]->GetRootComponent()->GetRelativeLocation());
#endif
#if !LEIA_STEREO_PATH
					sceneCaptureComponent->TextureTarget->UpdateResourceImmediate();
#endif
				}
			}
		}
	}

}

void ALeiaCameraPawn::CreateCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo)
{
	DestroyCamerasAndReleaseRenderTargets();
	SpawnCameraGrid(constructionInfo, RenderingInfo);
}

class UMaterialInstanceDynamic* ALeiaCameraPawn::GetPositiveDOFMaterialInst() const
{
	return MatInstanceDynamicPositiveDOF;
}

class UMaterialInstanceDynamic* ALeiaCameraPawn::GetViewSharpeningMaterialInst() const
{
	return MatInstanceDynamicViewSharpening;
}

class UMaterialInstanceDynamic* ALeiaCameraPawn::GetViewInterlacingMaterialInst() const
{
	return MatInstanceDynamicViewInterlacing;
}

bool ALeiaCameraPawn::IsZdpShearEnabled() const
{
	return ZdpInfo.bUseZdpShear;
}

void ALeiaCameraPawn::SetZdpShearEnabled(bool isEnabled)
{
	ZdpInfo.bUseZdpShear = isEnabled;
}

void ALeiaCameraPawn::SetACTCoEfficents(float A, float B)
{
	MatInstanceDynamicViewSharpening->SetScalarParameterValue("A", A);
	MatInstanceDynamicViewSharpening->SetScalarParameterValue("B", B);
}

void ALeiaCameraPawn::ShowCalibrationSqaure(bool show)
{
	MatInstanceDynamicViewInterlacing->SetScalarParameterValue("UseCalibrationSquare", (float)show);
}

float ALeiaCameraPawn::GetCurrentShearValue() const
{
	return ZdpInfo.ZDPShear;
}

bool ALeiaCameraPawn::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ALeiaCameraPawn::SpawnCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo)
{
	float wOffset;
	CalculateGridOffset(constructionInfo, wOffset);

	MatInstanceDynamicPositiveDOF = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialPositiveDOF, this);

	for (int wIndex = 0; wIndex < constructionInfo.GridWidth; wIndex++)
	{
		FActorSpawnParameters params;
		params.Owner = this;
		const FString cameraName = GetCameraNameFromGridIndex(constructionInfo, wIndex);

		auto cameraObj = GetWorld()->SpawnActor<CaptureActor>(CaptureActor::StaticClass(), params);
		if (cameraObj == nullptr)
		{
			UE_LOG(LogLeia, Log, TEXT("FATAL - Couldnt spawn actor"));
			return;
		}

#if WITH_EDITOR
		cameraObj->SetActorLabel(*cameraName);
#endif
		if (cameraObj)
		{
			cameraObj->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			cameraObj->SetActorRelativeLocation({ 0.0f, renderingInfo.Baseline * (wIndex - wOffset), 0.0f });

			CaptureComponent * const sceneCaptureComponent = cameraObj->GetCaptureComponent2D();
#if !LEIA_STEREO_PATH
			sceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
			sceneCaptureComponent->bCaptureEveryFrame = true;
			sceneCaptureComponent->bCaptureOnMovement = false;
			sceneCaptureComponent->bUseCustomProjectionMatrix = true;
			sceneCaptureComponent->bAlwaysPersistRenderingState = false;

			CreateAndSetRenderTarget(this, sceneCaptureComponent, constructionInfo, Gamma);

			sceneCaptureComponent->TextureTarget->UpdateResourceImmediate();
#endif
			sceneCaptureComponent->CustomProjectionMatrix = CalculateProjectionMatrix(constructionInfo, renderingInfo, { 0.0f, renderingInfo.Baseline * (wIndex - wOffset), 0.0f });

			Cameras.Add(cameraObj);
		}
	}
}

void ALeiaCameraPawn::DestroyCamerasAndReleaseRenderTargets()
{
	if (Cameras.Num() > 0)
	{
		// Go through spawned cameras and destroy them
		for (int cameraIndex = 0; cameraIndex < Cameras.Num(); ++cameraIndex)
		{
			if (Cameras[cameraIndex] && !Cameras[cameraIndex]->IsUnreachable() && !Cameras[cameraIndex]->IsPendingKill())
			{
#if !LEIA_STEREO_PATH
				UKismetRenderingLibrary::ReleaseRenderTarget2D(((CaptureActor*)Cameras[cameraIndex])->GetCaptureComponent2D()->TextureTarget);
#endif
				Cameras[cameraIndex]->Destroy();
			}
		}

		Cameras.Empty();
	}
}

// Called every frame
void ALeiaCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (TargetCamera == nullptr)
	{
		return;
	}

	if (TargetCameraComponent == nullptr || (TargetCameraComponent != nullptr && (TargetCameraComponent->IsBeingDestroyed() || TargetCameraComponent->IsUnreachable() || TargetCameraComponent->IsPendingKill())))
	{
		TargetCameraComponent = Cast<UCameraComponent>(TargetCamera->GetComponentByClass(UCameraComponent::StaticClass()));

#if !LEIA_STEREO_PATH
		TargetCameraComponent->PostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
		//TargetCameraComponent->PostProcessSettings.AddBlendable(MatInstanceDynamicViewSharpening, 1.0f);
#endif
	}

	if (TargetCameraComponent == nullptr)
	{
		return;
	}

    RenderingInfo.ProjectionType = TargetCameraComponent->ProjectionMode;

    if (RenderingInfo.ProjectionType == ECameraProjectionMode::Orthographic)
    {
        RenderingInfo.OrthographicWidth = TargetCameraComponent->OrthoWidth;
        RenderingInfo.NearClipPlane = TargetCameraComponent->OrthoNearClipPlane;
    }
	else
	{
		RenderingInfo.FieldOfView = TargetCameraComponent->FieldOfView;
	}

	SetActorTransform(TargetCameraComponent->GetComponentTransform());

	if (ZdpInfo.bUseZdpShear)
	{
		if (Cameras.Num() > 0)
		{
			for (int index = 0; index < ConstructionInfo.GridWidth; index++)
			{
				if (Cameras.IsValidIndex(index) && Cameras[index] != nullptr)
				{
					Cameras[index]->SetActorRelativeLocation({ 0.0f, UpdateViews(index, RenderingInfo, ConstructionInfo), 0.0f });
#if !LEIA_STEREO_PATH
					auto capComp = ((CaptureActor*)Cameras[index]);
					int32 blendableCount = capComp->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array.Num();

					for (int32 blendableIndex = 0; blendableIndex < blendableCount; blendableIndex++)
					{
						capComp->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array[blendableIndex].Weight = 1.0f;
					}

					SetPostProcessingValuesFromTargetCamera(capComp->GetCaptureComponent2D(), Cast<UCameraComponent>(TargetCameraComponent));
#endif
				}
			}
		}

		SetConvergenceValueFromRaycast(TargetCamera->GetActorLocation(), TargetCamera->GetActorForwardVector(), ZdpInfo, RenderingInfo, GetWorld());

		if (MatInstanceDynamicPositiveDOF != nullptr && MatInstancesDynamicZDP.Num() > 0 && Cameras.Num() > 2)
		{

			auto camA = ((CaptureActor*)Cameras[0])->GetCaptureComponent2D();
			auto camB = ((CaptureActor*)Cameras[1])->GetCaptureComponent2D();

			const float interviewDistance = GetInterviewDistanceUsingLeiaCamera(Cameras[0]->GetRootComponent()->GetRelativeLocation(),
				Cameras[1]->GetRootComponent()->GetRelativeLocation());

			if (ZdpInfo.bAutoZDP)
			{
				ZdpInfo.ZDPShear = -CalculateAutoZdpShearValue(ConstructionInfo, RenderingInfo, ZdpInfo, TargetCamera->GetTransform(), interviewDistance, camA, camB) * 100.0f;
			}

			//TODO Lee need to bring this whole thing over to stereo path
			SetCommonZdpMaterialParams(RenderingInfo, ConstructionInfo, ZdpInfo, camA, camB, TargetCamera->GetTransform());

			for (int index = 0; index < ConstructionInfo.GridWidth; ++index)
			{
				//TODO Lee need to bring this whole thing over to stereo path
				MatInstancesDynamicZDP[index]->SetScalarParameterValue("SignedViewDistanceToCenter",
					GetSignedViewDistanceToCenter(Cameras[index]->GetRootComponent()->GetRelativeLocation()));
			}
		}
	}
	else
	{
		if (Cameras.Num() > 0)
		{
			float wOffset;
			CalculateGridOffset(ConstructionInfo, wOffset);
			for (int index = 0; index < Cameras.Num(); index++)
			{
				if (Cameras[index] != nullptr)
				{
#if !LEIA_STEREO_PATH
					auto CapComp = ((CaptureActor*)Cameras[index]);
					int32 blendableCount = CapComp->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array.Num();
					for (int32 blendableIndex = 0; blendableIndex < blendableCount; blendableIndex++)
					{
						CapComp->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array[blendableIndex].Weight = 0.0f;
					}
					SetPostProcessingValuesFromTargetCamera(CapComp->GetCaptureComponent2D(), Cast<UCameraComponent>(TargetCameraComponent));
#endif
					Cameras[index]->SetActorRelativeLocation({ 0.0f, RenderingInfo.Baseline * (index - wOffset), 0.0f });
				}
			}
			RefreshCameraGrid();
		}
	}

	if (PrevUseZdpShear != ZdpInfo.bUseZdpShear || PrevScreenOrientation != CurrentScreenOrientation)
	{
		RefreshCameraGrid();
		PrevUseZdpShear = ZdpInfo.bUseZdpShear;
		PrevScreenOrientation = CurrentScreenOrientation;
	}

	if (bDisplayFrustum)
	{
		DisplayCameraFrustum(GetTransform(), TargetCameraComponent->GetComponentTransform(), ConstructionInfo, RenderingInfo, GetWorld());
	}

	switch (CurrentRefreshState)
	{
	case RefreshState::INTIAL:
		CurrentRefreshState = RefreshState::READY;
		break;
	case RefreshState::READY:
		RefreshCameraGrid();
		CurrentRefreshState = RefreshState::COMPLETED;
		break;
	case RefreshState::COMPLETED:
	default:
		break;
	}

}

void ALeiaCameraPawn::Destroyed()
{
	Super::Destroyed();
	DestroyCamerasAndReleaseRenderTargets();
}

FVector2D GetGameViewportSize()
{
	FVector2D Result = FVector2D(1, 1);

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize( /*out*/Result);
	}

	return Result;
}

FVector2D GetGameResolution()
{
	FVector2D Result = FVector2D(1, 1);

	Result.X = GSystemResolution.ResX;
	Result.Y = GSystemResolution.ResY;

	return Result;
}



IStereoLayers::FLayerDesc FLeiaStereoLayers::GetDebugCanvasLayerDesc(FTextureRHIRef Texture)
{
	IStereoLayers::FLayerDesc StereoLayerDesc;
	const float DebugCanvasX =  DefaultX;
	const float DebugCanvasY =  DefaultY;
	float DebugCanvasZ =  DefaultZ;

	StereoLayerDesc.Transform = FTransform(FVector(DebugCanvasZ, DebugCanvasX, DebugCanvasY));
	if (IsOpenGLPlatform(GMaxRHIShaderPlatform))
	{
		StereoLayerDesc.Transform.SetScale3D(FVector(1.f, 1.f, -1.f));
	}
	const float DebugCanvasWidth = DefaultWidth;
	const float DebugCanvasHeight = DefaultHeight;
	StereoLayerDesc.QuadSize = FVector2D(DebugCanvasWidth, DebugCanvasHeight);
	StereoLayerDesc.PositionType = IStereoLayers::ELayerType::FaceLocked;
	StereoLayerDesc.Texture = GFakeStereoDevice ? (FRHITexture2D*)GFakeStereoDevice->OverlayShaderResourceTexture : (FRHITexture2D*)Texture->GetTexture2D();
	StereoLayerDesc.Flags = IStereoLayers::ELayerFlags::LAYER_FLAG_TEX_CONTINUOUS_UPDATE;
	StereoLayerDesc.Flags |= IStereoLayers::ELayerFlags::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO;
	StereoLayerDesc.Flags |= IStereoLayers::ELayerFlags::LAYER_FLAG_TEX_EXTERNAL;

	return StereoLayerDesc;
}


#define LOCTEXT_NAMESPACE "FLeiaCameraModule"

void FLeiaCameraModule::StartupModule()
{
	IHeadMountedDisplayModule::StartupModule();
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("LeiaCamera"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/LeiaCamera"), PluginShaderDir);
}

void FLeiaCameraModule::ShutdownModule()
{
	IHeadMountedDisplayModule::ShutdownModule();
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}



TSharedPtr< class IXRTrackingSystem, ESPMode::ThreadSafe > FLeiaCameraModule::CreateTrackingSystem()
{
#if LEIA_STEREO_PATH
	GFakeStereoDevice = MakeShared<FLeiaStereoRenderingDevice, ESPMode::ThreadSafe>();

	return GFakeStereoDevice;
#else
	return NULL;
#endif
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLeiaCameraModule, LeiaCamera)
