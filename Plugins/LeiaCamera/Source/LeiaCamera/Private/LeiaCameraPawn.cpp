// Fill out your copyright notice in the Description page of Project Settings.


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
#include "WindowsLeiaDevice.h"



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
float scaleToRange(float unscaledNum, float minAllowed, float maxAllowed, float min, float max);

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

		EShaderParameterFlags flags = SPF_Optional;
		viewsX.Bind(Initializer.ParameterMap, TEXT("viewsX"), flags);
		viewsY.Bind(Initializer.ParameterMap, TEXT("viewsY"), SPF_Optional); //it's often assumed to always be 1
		viewResX.Bind(Initializer.ParameterMap, TEXT("viewResX"), flags);
		viewResY.Bind(Initializer.ParameterMap, TEXT("viewResY"), flags);

        n.Bind(Initializer.ParameterMap, TEXT("n"), flags);
        d_over_n.Bind(Initializer.ParameterMap, TEXT("d_over_n"), flags);
        faceX.Bind(Initializer.ParameterMap, TEXT("faceX"), flags);
        faceY.Bind(Initializer.ParameterMap, TEXT("faceY"), flags);
        faceZ.Bind(Initializer.ParameterMap, TEXT("faceZ"), flags);
        pixelPitch.Bind(Initializer.ParameterMap, TEXT("pixelPitch"), flags);
        du.Bind(Initializer.ParameterMap, TEXT("du"), flags);
        dv.Bind(Initializer.ParameterMap, TEXT("dv"), flags);
        s.Bind(Initializer.ParameterMap, TEXT("s"), flags);
        cos_theta.Bind(Initializer.ParameterMap, TEXT("cos_theta"), flags);
        sin_theta.Bind(Initializer.ParameterMap, TEXT("sin_theta"), flags);
        No.Bind(Initializer.ParameterMap, TEXT("No"), flags);
        peelOffset.Bind(Initializer.ParameterMap, TEXT("peelOffset"), flags);
        viewPeeling.Bind(Initializer.ParameterMap, TEXT("viewPeeling"), flags);
        displayResX.Bind(Initializer.ParameterMap, TEXT("displayResX"), flags);
        displayResY.Bind(Initializer.ParameterMap, TEXT("displayResY"), flags);
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

        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), n, interlaceParams.n);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), d_over_n, interlaceParams.d_over_n);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), faceX, interlaceParams.faceX);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), faceY, interlaceParams.faceY);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), faceZ, interlaceParams.faceZ);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), pixelPitch, interlaceParams.pixelPitch);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), du, interlaceParams.du);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), dv, interlaceParams.dv);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), s, interlaceParams.s);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), cos_theta, interlaceParams.cos_theta);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), sin_theta, interlaceParams.sin_theta);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), No, interlaceParams.No);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), peelOffset, interlaceParams.peelOffset);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), viewPeeling, interlaceParams.viewPeeling);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), displayResX, interlaceParams.displayResX);
        SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), displayResY, interlaceParams.displayResY);
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, texture_0);
	LAYOUT_FIELD(FShaderResourceParameter, texture_0Sampler);

	LAYOUT_FIELD(FShaderParameter, viewsX);
	LAYOUT_FIELD(FShaderParameter, viewsY);
	LAYOUT_FIELD(FShaderParameter, viewResX);
	LAYOUT_FIELD(FShaderParameter, viewResY);

    LAYOUT_FIELD(FShaderParameter, n);
    LAYOUT_FIELD(FShaderParameter, d_over_n);
    LAYOUT_FIELD(FShaderParameter, faceX);
    LAYOUT_FIELD(FShaderParameter, faceY);
    LAYOUT_FIELD(FShaderParameter, faceZ);
    LAYOUT_FIELD(FShaderParameter, pixelPitch);
    LAYOUT_FIELD(FShaderParameter, du);
    LAYOUT_FIELD(FShaderParameter, dv);
    LAYOUT_FIELD(FShaderParameter, s);
    LAYOUT_FIELD(FShaderParameter, cos_theta);
    LAYOUT_FIELD(FShaderParameter, sin_theta);
    LAYOUT_FIELD(FShaderParameter, No);
    LAYOUT_FIELD(FShaderParameter, peelOffset);
    LAYOUT_FIELD(FShaderParameter, viewPeeling);
    LAYOUT_FIELD(FShaderParameter, displayResX);
    LAYOUT_FIELD(FShaderParameter, displayResY);
};

IMPLEMENT_SHADER_TYPE(, FInterlacePS, TEXT("/Plugin/LeiaCamera/InterlacePS.usf"), TEXT("MainPS"), SF_Pixel);

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
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), gamma, sharpenParams.gamma);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningCenter, sharpenParams.sharpeningCenter);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), textureInvSize, sharpenParams.textureInvSize);
		SetShaderValueArray(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningValues, sharpenParams.sharpeningValues, 18);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), sharpeningValueCount, float(sharpenParams.sharpeningValueCount));
	}
	
private:
	LAYOUT_FIELD(FShaderResourceParameter, texture_0);
	LAYOUT_FIELD(FShaderResourceParameter, texture_0Sampler);
	LAYOUT_FIELD(FShaderParameter, gamma);
	LAYOUT_FIELD(FShaderParameter, sharpeningCenter);
	LAYOUT_FIELD(FShaderParameter, textureInvSize);
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

	virtual FTexture2DRHIRef GetOverlayLayerTarget_RenderThread(EStereoscopicPass StereoPass, FIntRect& InOutViewport) override
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
		return FIntPoint(1280, 720); //making this bigger will reduce UI legibility
	}
	virtual ~FLeiaStereoRenderingDevice() {}

	virtual bool IsStereoEnabled() const override { return (leiaPawn != NULL); }

	virtual bool EnableStereo(bool stereo = true) override { return (leiaPawn != NULL); }

	virtual int32 GetDesiredNumberOfViews(bool bStereoRequested) const { return (bStereoRequested && leiaPawn != NULL) ? NumViews : 1; }

	virtual EStereoscopicPass GetViewPassForIndex(bool bStereoRequested, uint32 ViewIndex) const
	{
		if (!bStereoRequested)
			return EStereoscopicPass::eSSP_FULL;

		return static_cast<EStereoscopicPass>(eSSP_LEFT_EYE + ViewIndex);
	}

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


	virtual bool DeviceIsAPrimaryPass(EStereoscopicPass Pass)
	{
		return Pass == EStereoscopicPass::eSSP_FULL || Pass == EStereoscopicPass::eSSP_LEFT_EYE;
		//return Pass == EStereoscopicPass::eSSP_FULL || (GetViewIndexForPass(Pass) % 2 == 0);
	}

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

	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override
	{
		SizeX = Width;
		SizeY = Height;
		int currX = GetViewIndexForPass(StereoPass);
		X += SizeX * currX;
	}

	virtual void CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) override
	{
		if (StereoPassType != eSSP_FULL)
		{
			auto viewIndex = GetViewIndexForPass(StereoPassType);;
			
			if (leiaPawn && viewIndex < (unsigned)leiaPawn->Cameras.Num())
			{
				auto CapComp = ((CaptureActor *)leiaPawn->Cameras[viewIndex])->GetCaptureComponent2D();

				ViewLocation = CapComp->GetComponentLocation();
			}
		}
	}

	virtual bool DeviceIsAnAdditionalView(const FSceneView& View)
	{
		return View.StereoPass > NumViews;
	}

	virtual bool DeviceIsASecondaryView(const FSceneView& View)
	{
		return !DeviceIsAPrimaryView(View);
	}

	void GetEyeRenderParams_RenderThread(const FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override
	{
	}


	virtual FMatrix GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const override
	{

		const int viewIndex = GetViewIndexForPass(StereoPassType);


		if (leiaPawn && leiaPawn->Cameras.IsValidIndex(viewIndex) && leiaPawn->Cameras[viewIndex] != nullptr) //viewIndex
		{	
#if LEIA_STEREO_PATH
			return ((CaptureActor*)leiaPawn->Cameras[viewIndex])->GetCaptureComponent2D()->ShiftedProjectionMatrix;
#else
			return ((CaptureActor *)leiaPawn->Cameras[viewIndex])->GetCaptureComponent2D()->CustomProjectionMatrix;
#endif
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
	virtual void OnBeginRendering_GameThread() override
	{
		IXRTrackingSystem::OnBeginRendering_GameThread();

		leiaPawn->interlaceShaderParams_RenderThread[leiaPawn->gameThreadParams] = leiaPawn->interlaceShaderParams_GameThread;
		leiaPawn->gameThreadParams++;

		if (leiaPawn->gameThreadParams >= BUFFERED_PARAMS)
		{
			leiaPawn->gameThreadParams = 0;
		}
	}
	virtual bool DoesSupportLateUpdate() const override  { return true; }


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

		FRHIResourceCreateInfo CreateInfo;
		RHICreateTargetableShaderResource2D(SizeX, SizeY, GetPixelFormatFromRenderTargetFormat(RTFormat), 1, TexCreate_None, TexCreate_RenderTargetable, false, CreateInfo, OutTargetableTexture, OutShaderResourceTexture);

		FRHIResourceCreateInfo CreateInfo2;
		RHICreateTargetableShaderResource2D(PanelResolutionX, PanelResolutionY, GetPixelFormatFromRenderTargetFormat(RTF_RGBA8), 1, TexCreate_None, TexCreate_RenderTargetable, false, CreateInfo2, StagingTargetableTexture, StagingShaderResourceTexture);

		FRHIResourceCreateInfo CreateInfo3;
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
        bool debugEnableSharpening = debugAreOn12VDisplay && leiaPawn->useACT; //Dont turn back on until thoroughly tested on 4v-12 with different content. it has many artifact's issues.

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

			PixelShader->SetParameters(RHICmdList, leiaPawn->interlaceShaderParams_RenderThread[leiaPawn->renderThreadParams]);

			leiaPawn->renderThreadParams++;
			if (leiaPawn->renderThreadParams >= BUFFERED_PARAMS)
			{
				leiaPawn->renderThreadParams = 0;
			}

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
	InputComp->BindAction("ToggleFaceTracking", IE_Released, this, &ALeiaCameraPawn::ToggleFaceTracking);
	InputComp->BindAction("ToggleViewPeeling", IE_Released, this, &ALeiaCameraPawn::ToggleViewPeeling);

	InputComp->BindAction("ConvergenceUp", IE_Released, this, &ALeiaCameraPawn::ConvergenceUp);
	InputComp->BindAction("ConvergenceDown", IE_Released, this, &ALeiaCameraPawn::ConvergenceDown);

	InputComp->BindAction("BaselineUp", IE_Released, this, &ALeiaCameraPawn::BaselineUp);
	InputComp->BindAction("BaselineDown", IE_Released, this, &ALeiaCameraPawn::BaselineDown);

	InputComp->BindAction("AdjustViewOffsetUp", IE_Released, this, &ALeiaCameraPawn::AdjustViewOffsetUp);
	InputComp->BindAction("AdjustViewOffsetDown", IE_Released, this, &ALeiaCameraPawn::AdjustViewOffsetDown);
}

void ALeiaCameraPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Device->SetBacklightMode(BacklightMode::MODE_2D);
	blinkClass.Shutdown();
}

void ALeiaCameraPawn::ToggleACT()
{
	useACT = !useACT;

	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("ACT %s"), useACT ? TEXT("ON") : TEXT("OFF")), true, FVector2D(2.0, 2.0));
}

void ALeiaCameraPawn::ToggleFaceTracking()
{
	isFaceTracking = !isFaceTracking;

	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("FaceTracking %s"), isFaceTracking ? TEXT("ON") : TEXT("OFF")), true, FVector2D(2.0, 2.0));
}

void ALeiaCameraPawn::ToggleViewPeeling()
{
	isViewPeeling = !isViewPeeling;

	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("View Peeling %s"), isViewPeeling ? TEXT("ON") : TEXT("OFF")), true, FVector2D(2.0, 2.0));
}

void ALeiaCameraPawn::ConvergenceUp()
{
	RenderingInfo.ConvergenceDistance += 1.0f;

	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("Convergence %.2f"), RenderingInfo.ConvergenceDistance), true, FVector2D(2.0, 2.0));
}

void ALeiaCameraPawn::ConvergenceDown()
{
	RenderingInfo.ConvergenceDistance -= 1.0f;

	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("Convergence %.2f"), RenderingInfo.ConvergenceDistance), true, FVector2D(2.0, 2.0));
}

void ALeiaCameraPawn::BaselineUp()
{
	RenderingInfo.Baseline += .5f;

	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("Baseline %.2f"), RenderingInfo.Baseline), true, FVector2D(2.0, 2.0));
}
void ALeiaCameraPawn::BaselineDown()
{
	RenderingInfo.Baseline -= .5f;

	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("Baseline %.2f"), RenderingInfo.Baseline), true, FVector2D(2.0, 2.0));
}
void ALeiaCameraPawn::AdjustViewOffsetUp()
{
	interlaceShaderParams_GameThread.No += 1.0f;


	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("interlaceShaderParams.No %2.2f"), interlaceShaderParams_GameThread.No), true, FVector2D(2.0, 2.0));
}
void ALeiaCameraPawn::AdjustViewOffsetDown()
{
	interlaceShaderParams_GameThread.No -= 1.0f;

	GEngine->AddOnScreenDebugMessage(1000, 5.0, FColor::Cyan, FString::Printf(TEXT("interlaceShaderParams.No %2.2f"), interlaceShaderParams_GameThread.No), true, FVector2D(2.0, 2.0));
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
	CurrentScreenOrientation = type;

	if (type == EScreenOrientation::Portrait || type == EScreenOrientation::PortraitUpsideDown)
	{
		//FDisplayConfig displayConfig = Device->GetDisplayConfig();
		//SetACTCoEfficents(displayConfig.actCoefficients[0], displayConfig.actCoefficients[1]);

		FLeiaCameraConstructionInfo constructionInfo;
		constructionInfo.GridWidth = ConstructionInfo.GridWidth;
		constructionInfo.RenderTextureResolutionWidth = ConstructionInfo.RenderTextureResolutionHeight;
		constructionInfo.RenderTextureResolutionHeight = ConstructionInfo.RenderTextureResolutionWidth;

		CreateCameraGrid(constructionInfo);
	}
	else if (type == EScreenOrientation::LandscapeLeft || type == EScreenOrientation::LandscapeRight || type == EScreenOrientation::Unknown)
	{
		//FDisplayConfig displayConfig = Device->GetDisplayConfig();
		//SetACTCoEfficents(displayConfig.actCoefficients[2], displayConfig.actCoefficients[3]);
		CreateCameraGrid(ConstructionInfo);
	}
	FDisplayConfig displayConfig = Device->GetDisplayConfig();
	displayCamCenterX = displayConfig.cameraCenterX;
	displayCamCenterY = displayConfig.cameraCenterY;
	displayConvergence = displayConfig.convergenceDistance;

	GetInterlaceParams(type, interlaceShaderParams_GameThread);

	SetInterlaceMaterialParams(type);
	GetViewSharpeningParams(displayConfig, sharpenShaderParams);
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

#if LEIA_USE_SERVICE
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "setres 3840x2160f");
#endif

#else
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 100.0");
#endif
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

	blinkClass.Start();


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
			//TargetCameraComponent_Temp_Camera->PostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
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

	auto targetTransform = TargetCameraComponent->GetComponentTransform();

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
					sceneCaptureComponent->ShiftedProjectionMatrix = CalculateProjectionMatrix(ConstructionInfo, RenderingInfo,
						Cameras[wIndex]->GetRootComponent()->GetRelativeLocation(), targetTransform.ToMatrixNoScale());
#else
					sceneCaptureComponent->CustomProjectionMatrix = CalculateProjectionMatrix(ConstructionInfo, RenderingInfo,
						Cameras[wIndex]->GetRootComponent()->GetRelativeLocation(), targetTransform.ToMatrixNoScale());
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
#if LEIA_STEREO_PATH
			sceneCaptureComponent->ShiftedProjectionMatrix = sceneCaptureComponent->CustomProjectionMatrix;
#endif
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
			if (Cameras[cameraIndex] && !Cameras[cameraIndex]->IsPendingKillOrUnreachable())
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

	FVector localFace;
	auto facesFound = blinkClass.Tick(localFace, DeltaTime, displayCamCenterX, displayCamCenterY);

	if (facesFound == 0 || !isFaceTracking)
	{
		localFace = FVector(0.0f, 0.0f, displayConvergence); //need to pull from config
	}

	//Use mouse pos instead of face for cyrus 
	//localFace = GetMousePositionForEmulatingFaceTracking();


	interlaceShaderParams_GameThread.faceX = localFace.X;
	interlaceShaderParams_GameThread.faceY = localFace.Y;
	interlaceShaderParams_GameThread.faceZ = localFace.Z;

	float peelOffset = getN(localFace.X, localFace.Y, localFace.Z, 0.0f, 0.0f) - interlaceShaderParams_GameThread.No;

	if (isViewPeeling)
	{
		peelOffset = roundf(peelOffset);
	}

	interlaceShaderParams_GameThread.peelOffset = peelOffset;

#if 0
	GEngine->AddOnScreenDebugMessage(123213, 5.0, FColor::Green, FString::Printf(TEXT("Face %2.f %2.f %2.f"), localFace.X, localFace.Y, localFace.Z), true, FVector2D(2.0, 2.0));
	GEngine->AddOnScreenDebugMessage(373737, 5.0, FColor::Red, FString::Printf(TEXT("CenterView %f"), interlaceShaderParams_GameThread.No), true, FVector2D(2.5, 2.5));
	GEngine->AddOnScreenDebugMessage(123214, 5.0, FColor::Cyan, FString::Printf(TEXT("PeelOffset\n %2.2f"), peelOffset), true, FVector2D(2.0, 2.0));
#endif

	if (TargetCamera == nullptr)
	{
		return;
	}

	if (TargetCameraComponent == nullptr || (TargetCameraComponent != nullptr && (TargetCameraComponent->IsBeingDestroyed() || TargetCameraComponent->IsPendingKillOrUnreachable())))
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

	auto targetTransform = TargetCameraComponent->GetComponentTransform();
	SetActorTransform(targetTransform);

	if ( ZdpInfo.bUseZdpShear)
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

					if (isViewPeeling)
					{
						Cameras[index]->SetActorRelativeLocation({ 0.0f, RenderingInfo.Baseline * (index - wOffset + interlaceShaderParams_GameThread.peelOffset), 0 });
					}
					else
					{
						Cameras[index]->SetActorRelativeLocation({ 0.0f, RenderingInfo.Baseline * (index - wOffset), 0 });
					}
					
					//Cameras[index]->SetActorRelativeLocation({ 0.0f, RenderingInfo.Baseline * (index - wOffset) +CameraShift.X, 0 });
					UpdateViews(index, RenderingInfo, ConstructionInfo, true, targetTransform.ToMatrixNoScale());
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
	blinkClass.Shutdown();
}


float scaleToRange(float unscaledNum, float minAllowed, float maxAllowed, float min, float max)
{
	return (maxAllowed - minAllowed) * (unscaledNum - min) / (max - min) + minAllowed;
}


FVector ALeiaCameraPawn::GetMousePositionForEmulatingFaceTracking()
{
	FVector retVal;;

	if (UGameplayStatics::GetPlayerController(GWorld, 0))
	{
		UGameplayStatics::GetPlayerController(GWorld, 0)->GetMousePosition(retVal.X, retVal.Y);

		auto screenSize =  GetGameResolution();
		//no face found, scale mouse to compatible values
		retVal.X = scaleToRange(retVal.X, -466.0, 466.0, 0, screenSize.X);
		retVal.Y = scaleToRange(retVal.Y, -262.0, 262.0, 0.0, screenSize.Y);
		retVal.Z = displayConvergence;
	}

	return retVal;
}

float ALeiaCameraPawn::getN(float x, float y, float z, float x0, float y0)
{
    FDisplayConfig displayConfig = Device->GetDisplayConfig();

    //InterlaceParams Default12VParams;        // @Lee: I'm using this structure because it's default constructor sets up 12V values. Need to pull from config eventually.
    double Default12VTheta = 0.3;            // @Lee: Also need to pull from config eventually, add to interlace shader


    float pixelPitch = interlaceShaderParams_GameThread.pixelPitch;
    float fullWidth = displayConfig.panelResolution[0];
    float fullHeight = displayConfig.panelResolution[1];

    float stretch_n = interlaceShaderParams_GameThread.s;
    float theta_n = Default12VTheta / (fullHeight * 3.0f);
    float No = interlaceShaderParams_GameThread.No;// displayPropertyData.display.offsetX; // CenterView !!!!!

    float du = interlaceShaderParams_GameThread.pixelPitch / 3.0f;    // 3.0f probably could be interlacing_matrix[12] * viewCount / panelResolution.x
    float dv = displayConfig.p_over_dv * interlaceShaderParams_GameThread.pixelPitch;// interlaceShaderParams.matD.B > 0.0f ? pixelPitch : -pixelPitch;//  GetDisplayConfig().InterlacingMatrix[14] > 0.0f ? +pixelPitch : -pixelPitch; // "boolean isPositiveSlant" is probably better represented as a float interlacing_matrix[13] * viewCount / panelResolution.y. currently we assume slant is not 0 and has no scale
    // note: sign changed on main for this calc

    // assume there are no issues with angular wrap-around of operations like angle - angle
    float dx = interlaceShaderParams_GameThread.s * x0 + (cos(theta_n) - 1.0f) * x0 - sin(theta_n) * y0;
    float dy = interlaceShaderParams_GameThread.s * y0 + (cos(theta_n) - 1.0f) * y0 + cos(theta_n) * x0;

    float n = interlaceShaderParams_GameThread.n;
    float denom = sqrt(z * z + (1 - 1.0f / (n * n)) * ((x - x0) * (x - x0) + (y - y0) * (y - y0)));

    float u = dx + interlaceShaderParams_GameThread.d_over_n * (x - x0) / denom;
    float v = dy + interlaceShaderParams_GameThread.d_over_n * (y - y0) / denom;

    float N = No + u / du + v / dv;


	//GEngine->AddOnScreenDebugMessage(123215, 5.0, FColor::Cyan, FString::Printf(TEXT("pixelPitch %f\n  fullWidth %f\n fullHeight %f\n stretch_n %f\n  theta_n %f \n dOverN %f"), pixelPitch, fullWidth, fullHeight, stretch_n, theta_n, interlaceShaderParams.d_over_n), true, FVector2D(2.0, 2.0));

    return N;
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
	StereoLayerDesc.Texture = GFakeStereoDevice ? GFakeStereoDevice->OverlayShaderResourceTexture : Texture->GetTexture2D();
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