/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/


#include "LeiaCameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraComponent.h"
#include "Components/LineBatchComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/BlueprintPlatformLibrary.h"
#include "Engine/Engine.h"


// Sets default values for this component's properties
ULeiaCameraComponent::ULeiaCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void ULeiaCameraComponent::SetDeviceOverride()
{
	EViewOverrideMode overrideMode = EViewOverrideMode::LumePad;
	switch (ViewMode)
	{
	case EViewMode::LumePad:
		overrideMode = EViewOverrideMode::LumePad;
		break;
	case EViewMode::Windows_12p5_8V:
		overrideMode = EViewOverrideMode::Windows_12p5_8V;
		break;
	case EViewMode::AndroidPegasus_12p5_8V:
		overrideMode = EViewOverrideMode::AndroidPegasus_12p5_8V;
		break;
	case EViewMode::AndroidPegasus_12p3_8V:
		overrideMode = EViewOverrideMode::AndroidPegasus_12p3_8V;
		break;
	case EViewMode::Windows_15p6_12V:
		overrideMode = EViewOverrideMode::Windows_15p6_12V;
		break;
	case EViewMode::Windows_15p6_13V:
		overrideMode = EViewOverrideMode::Windows_15p6_13V;
		break;
	case EViewMode::None:
		overrideMode = EViewOverrideMode::None;
		break;
	default:
		overrideMode = EViewOverrideMode::None;
		break;
	}
	Device->SetOverride(overrideMode);
}
void ULeiaCameraComponent::OnScreenOrientationChanged(EScreenOrientation::Type type)
{
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

	SetInterlaceMaterialParams(type);
	SetViewSharpeningMaterialParams();

	if (Cameras.Num() == 0)
	{
		CreateCameraGrid(ConstructionInfo);
	}

	for (int32 camIndex = 0; camIndex < ConstructionInfo.GridWidth; camIndex++)
	{
		FString paramName = "CamInput_";
		paramName.AppendInt(camIndex);
		MatInstanceDynamicViewInterlacing->SetTextureParameterValue(*paramName, Cameras[camIndex]->TextureTarget);
	}

	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 100.0");
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "r.MobileContentScaleFactor 4.0");
}

void ULeiaCameraComponent::AppEnterBackground()
{
	Device->DesiredLightFieldMode = Device->GetBacklightMode();
	Device->SetBacklightMode(BacklightMode::MODE_2D);
}

void ULeiaCameraComponent::AppEnterForeground()
{
	Device->SetBacklightMode(Device->DesiredLightFieldMode);
	Device->DesiredLightFieldMode = BacklightMode::MODE_3D;
}

// Called when the game starts
void ULeiaCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	CommonMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(CommmonMatParamCollection);
	InterlaceMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(InterlaceMatParamCollection);
	ViewSharpeningMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(ViewSharpeningeMatParamCollection);

	InitialPostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
	InitialPostProcessSettings.AddBlendable(MatInstanceDynamicViewSharpening, 1.0f);

	if (TargetCamera == nullptr)
	{
		if (GetOwner()->HasActiveCameraComponent())
		{
			TargetCamera = Cast<UCameraComponent>(GetOwner()->GetComponentByClass(UCameraComponent::StaticClass()));
		}
		else
		{
			AActor* actor = GetOwner()->GetAttachParentActor();
			while (actor != nullptr && !actor->HasActiveCameraComponent())
			{
				actor = actor->GetAttachParentActor();
			}

			if (actor != nullptr)
			{
				TargetCamera = Cast<UCameraComponent>(actor->GetComponentByClass(UCameraComponent::StaticClass()));
			}
		}
	}
	SetDeviceOverride();

	if(TargetCamera != nullptr)
	{
		CachedPostProcessSettings = TargetCamera->PostProcessSettings;
		TargetCamera->PostProcessSettings = InitialPostProcessSettings;
	}

	SetConstructionInfo(ConstructionInfo);

	MatInstanceDynamicPositiveDOF = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialPositiveDOF, this);
	MatInstanceDynamicViewInterlacing = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialViewInterlacing, this);
	MatInstanceDynamicViewSharpening = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialViewSharpening, this);

	UPlatformGameInstance* const platformGameInst = Cast<UPlatformGameInstance>(GetOwner()->GetGameInstance());
	if (platformGameInst != nullptr && platformGameInst->IsValidLowLevel())
	{
		platformGameInst->ApplicationReceivedScreenOrientationChangedNotificationDelegate.AddDynamic(this, &ULeiaCameraComponent::OnScreenOrientationChanged);
		platformGameInst->ApplicationWillEnterBackgroundDelegate.AddDynamic(this, &ULeiaCameraComponent::AppEnterBackground);
		platformGameInst->ApplicationHasEnteredForegroundDelegate.AddDynamic(this, &ULeiaCameraComponent::AppEnterForeground);
		platformGameInst->ApplicationWillDeactivateDelegate.AddDynamic(this, &ULeiaCameraComponent::AppEnterBackground);
		platformGameInst->ApplicationHasReactivatedDelegate.AddDynamic(this, &ULeiaCameraComponent::AppEnterForeground);
		platformGameInst->ApplicationWillTerminateDelegate.AddDynamic(this, &ULeiaCameraComponent::AppEnterBackground);
	}

	OnScreenOrientationChanged(UBlueprintPlatformLibrary::GetDeviceOrientation());

	if (TargetCamera != nullptr)
	{
		TargetCamera->PostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
		TargetCamera->PostProcessSettings.AddBlendable(MatInstanceDynamicViewSharpening, 1.0f);
	}

	for (int camIndex = 0; camIndex < Cameras.Num(); ++camIndex)
	{
		if (MatInstancesDynamicZDP.Num() == Cameras.Num() && MatInstancesDynamicZDP[camIndex] != nullptr)
		{
			Cameras[camIndex]->PostProcessSettings.AddBlendable(MatInstancesDynamicZDP[camIndex], 1.0f);
		}
		else
		{
			UMaterialInstanceDynamic* const zdpMatInst = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialZDP, this);
			MatInstancesDynamicZDP.Add(zdpMatInst);
			Cameras[camIndex]->PostProcessSettings.AddBlendable(zdpMatInst, 1.0f);
		}

		Cameras[camIndex]->PostProcessSettings.AddBlendable(MatInstanceDynamicPositiveDOF, 1.0f);
	}

	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 100.0");
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "r.MobileContentScaleFactor 4.0");
	Device->SetBacklightMode(BacklightMode::MODE_3D);

#if !WITH_EDITOR
	bDisplayFrustum = false;
#endif
}

void ULeiaCameraComponent::OnRegister()
{
	Super::OnRegister();
	SetDeviceOverride();
	SetConstructionInfo(ConstructionInfo);

	if (Cameras.Num() == 0)
	{
		CreateCameraGrid(ConstructionInfo);
	}
}

void ULeiaCameraComponent::OnUnregister()
{
	Super::OnUnregister();
}

void ULeiaCameraComponent::DestroyComponent(bool bPromoteChildren /*= false*/)
{
	DestroyCamerasAndReleaseRenderTargets();
	Super::DestroyComponent(bPromoteChildren);
}




// Called every frame
void ULeiaCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (TargetCamera == nullptr)
	{
		return;
	}

	RenderingInfo.ProjectionType = TargetCamera->ProjectionMode;

	if (RenderingInfo.ProjectionType == ECameraProjectionMode::Orthographic)
	{
		RenderingInfo.OrthographicWidth = TargetCamera->OrthoWidth;
		RenderingInfo.NearClipPlane = TargetCamera->OrthoNearClipPlane;
	}
	else
	{
		RenderingInfo.FieldOfView = TargetCamera->FieldOfView;
	}

	if (ZdpInfo.bUseZdpShear)
	{
		if (Cameras.Num() > 0)
		{
			for (int wIndex = 0; wIndex < ConstructionInfo.GridWidth; wIndex++)
			{
				Cameras[wIndex]->SetRelativeLocation({ 0.0f, UpdateViews(wIndex, RenderingInfo, ConstructionInfo), 0.0f });

				int32 blendableCount = Cameras[wIndex]->PostProcessSettings.WeightedBlendables.Array.Num();
				for (int32 blendableIndex = 0; blendableIndex < blendableCount; blendableIndex++)
				{
					Cameras[wIndex]->PostProcessSettings.WeightedBlendables.Array[blendableIndex].Weight = 1.0f;
				}
				SetPostProcessingValuesFromTargetCamera(Cameras[wIndex], TargetCamera);
			}
		}

		SetConvergenceValueFromRaycast(TargetCamera->GetComponentLocation(), TargetCamera->GetForwardVector(), ZdpInfo, RenderingInfo, GetWorld());

		if (MatInstanceDynamicPositiveDOF != nullptr && MatInstancesDynamicZDP.Num() > 0 && Cameras.Num() > 2)
		{
			USceneCaptureComponent2D* const camA = Cameras[0];
			USceneCaptureComponent2D* const camB = Cameras[1];

			const float interviewDistance = GetInterviewDistanceUsingLeiaCamera(camA->GetComponentLocation(), camB->GetComponentLocation());

			if (ZdpInfo.bAutoZDP)
			{
				ZdpInfo.ZDPShear = -CalculateAutoZdpShearValue(ConstructionInfo, RenderingInfo, ZdpInfo, TargetCamera->GetComponentTransform(), interviewDistance, camA, camB) * 100.0f;
			}

			SetCommonZdpMaterialParams(RenderingInfo, ConstructionInfo, ZdpInfo, camA, camB, TargetCamera->GetComponentTransform());

			for (int index = 0; index < ConstructionInfo.GridWidth; ++index)
			{
				MatInstancesDynamicZDP[index]->SetScalarParameterValue("SignedViewDistanceToCenter",
					GetSignedViewDistanceToCenter(Cameras[index]->GetRelativeLocation()));
			}
		}
	}
	else
	{
		if (Cameras.Num() > 0)
		{
			ZdpInfo.ConvergenceOffset = 0.0f;
			float wOffset;
			CalculateGridOffset(ConstructionInfo, wOffset);
			for (int index = 0; index < Cameras.Num(); index++)
			{
				Cameras[index]->SetRelativeLocation({ 0.0f, RenderingInfo.Baseline * (index - wOffset), 0.0f });
				int32 blendableCount = Cameras[index]->PostProcessSettings.WeightedBlendables.Array.Num();
				for (int32 blendableIndex = 0; blendableIndex < blendableCount; blendableIndex++)
				{
					Cameras[index]->PostProcessSettings.WeightedBlendables.Array[blendableIndex].Weight = 0.0f;
				}
				SetPostProcessingValuesFromTargetCamera(Cameras[index], TargetCamera);
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
		DisplayCameraFrustum(GetComponentTransform(), TargetCamera->GetComponentTransform(), ConstructionInfo, RenderingInfo, GetWorld());
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

void ULeiaCameraComponent::RefreshCameraGrid()
{
	float wOffset;
	CalculateGridOffset(ConstructionInfo, wOffset);

	if (Cameras.Num() > 0)
	{
		for (int wIndex = 0; wIndex < ConstructionInfo.GridWidth; wIndex++)
		{
			USceneCaptureComponent2D* const camComponent = Cameras[wIndex];
			camComponent->CustomProjectionMatrix = CalculateProjectionMatrix(ConstructionInfo, RenderingInfo, camComponent->GetRelativeLocation());
			camComponent->TextureTarget->UpdateResourceImmediate();
			camComponent->UpdateComponentToWorld();
		}
	}
}

void ULeiaCameraComponent::CreateCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo_)
{
	DestroyCamerasAndReleaseRenderTargets();
	SpawnCameraGrid(constructionInfo_, RenderingInfo);
}

class UMaterialInstanceDynamic* ULeiaCameraComponent::GetPositiveDOFMaterialInst() const
{
	return MatInstanceDynamicPositiveDOF;
}

UMaterialInstanceDynamic* ULeiaCameraComponent::GetViewSharpeningMaterialInst() const
{
	return MatInstanceDynamicViewInterlacing;
}

UMaterialInstanceDynamic* ULeiaCameraComponent::GetViewInterlacingMaterialInst() const
{
	return MatInstanceDynamicViewSharpening;
}

bool ULeiaCameraComponent::IsZdpShearEnabled() const
{
	return ZdpInfo.bUseZdpShear;
}

void ULeiaCameraComponent::SetZdpShearEnabled(bool isEnabled)
{
	ZdpInfo.bUseZdpShear = isEnabled;
}

void ULeiaCameraComponent::SetACTCoEfficents(float A, float B)
{
	MatInstanceDynamicViewSharpening->SetScalarParameterValue("A", A);
	MatInstanceDynamicViewSharpening->SetScalarParameterValue("B", B);
}

void ULeiaCameraComponent::ShowCalibrationSqaure(bool show)
{
	MatInstanceDynamicViewInterlacing->SetScalarParameterValue("UseCalibrationSquare", (float)show);
}

void ULeiaCameraComponent::SpawnCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo)
{
	AActor* const MyOwner = GetOwner();

	if (MyOwner == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	float wOffset;
	CalculateGridOffset(constructionInfo, wOffset);

	if (MatInstancesDynamicZDP.Num() > 0)
	{
		for (int index = 0; index < MatInstancesDynamicZDP.Num(); index++)
		{
			MatInstancesDynamicZDP[index] = nullptr;
		}
	}

	MatInstancesDynamicZDP.Empty();

	for (int wIndex = 0; wIndex < constructionInfo.GridWidth; wIndex++)
	{
		USceneCaptureComponent2D* const camComponent = NewObject<USceneCaptureComponent2D>(this);
		camComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		camComponent->RegisterComponent();
		camComponent->RegisterComponentWithWorld(GetWorld());
		camComponent->SetRelativeLocation({ 0.0f, renderingInfo.Baseline * (wIndex - wOffset), 0.0f });

		camComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		camComponent->bCaptureEveryFrame = true;
		camComponent->bCaptureOnMovement = false;
		camComponent->bUseCustomProjectionMatrix = true;
		camComponent->bAlwaysPersistRenderingState = false;
		camComponent->CustomProjectionMatrix = CalculateProjectionMatrix(constructionInfo, renderingInfo, { 0.0f, renderingInfo.Baseline * (wIndex - wOffset), 0.0f });
		CreateAndSetRenderTarget(this, camComponent, constructionInfo, Gamma);

		UMaterialInstanceDynamic* const zdpMatInst = UMaterialInstanceDynamic::Create(RenderingInfo.PostProcessMaterialZDP, this->GetOwner());
		camComponent->PostProcessSettings.AddBlendable(zdpMatInst, 1.0f);
		camComponent->PostProcessSettings.AddBlendable(MatInstanceDynamicPositiveDOF, 1.0f);
		MatInstancesDynamicZDP.Add(zdpMatInst);
		Cameras.Add(camComponent);
	}
}

void ULeiaCameraComponent::DestroyCamerasAndReleaseRenderTargets()
{
	if (Cameras.Num() > 0)
	{
		// Go through spawned cameras and destroy them
		for (int cameraIndex = 0; cameraIndex < Cameras.Num(); ++cameraIndex)
		{
			if (Cameras[cameraIndex] && !Cameras[cameraIndex]->IsUnreachable() && IsValid(Cameras[cameraIndex]))
			{
				UKismetRenderingLibrary::ReleaseRenderTarget2D(Cameras[cameraIndex]->TextureTarget);
				Cameras[cameraIndex]->DestroyComponent();
			}
		}

		Cameras.Empty();
	}
}

#if WITH_EDITOR
void ULeiaCameraComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SetDeviceOverride();
	SetConstructionInfo(ConstructionInfo);
	CreateCameraGrid(ConstructionInfo);
}
#endif

