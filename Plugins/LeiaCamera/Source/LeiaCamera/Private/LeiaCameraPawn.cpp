/****************************************************************
*
* Copyright 2022 � Leia Inc.
*
****************************************************************
*/


#include "LeiaCameraPawn.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraComponent.h"
#include "Engine/SceneCapture2D.h"
#include "Components/LineBatchComponent.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"

// Sets default values
ALeiaCameraPawn::ALeiaCameraPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PostUpdateWork;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(FName(TEXT("Scene Root")));
	SetRootComponent(SceneRoot);
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
		overrideMode = EViewOverrideMode::None;
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
		FDisplayConfig displayConfig = Device->GetDisplayConfig();
		SetACTCoEfficents(displayConfig.actCoefficients[0], displayConfig.actCoefficients[1]);
		FLeiaCameraConstructionInfo constructionInfo;
		ConstructionInfo.PanelResolutionHeight = displayConfig.panelResolution[0];
		ConstructionInfo.PanelResolutionWidth = displayConfig.panelResolution[1];
		if (displayMode == DisplayMode::MODE_3D) {
			ConstructionInfo.GridWidth = savedGridWidth;
			constructionInfo.GridWidth = ConstructionInfo.GridWidth;
			constructionInfo.RenderTextureResolutionWidth = ConstructionInfo.RenderTextureResolutionWidth;
			constructionInfo.RenderTextureResolutionHeight = ConstructionInfo.RenderTextureResolutionHeight;

		}
		else {
			constructionInfo.GridWidth = 1;
			savedGridWidth = ConstructionInfo.GridWidth;
			ConstructionInfo.GridWidth = 1;
			constructionInfo.RenderTextureResolutionWidth = ConstructionInfo.PanelResolutionHeight;
			constructionInfo.RenderTextureResolutionHeight = ConstructionInfo.PanelResolutionWidth;
		}

		CreateCameraGrid(constructionInfo);
	}
	else if (type == EScreenOrientation::LandscapeLeft || type == EScreenOrientation::LandscapeRight || type == EScreenOrientation::Unknown)
	{
		FDisplayConfig displayConfig = Device->GetDisplayConfig();
		SetACTCoEfficents(displayConfig.actCoefficients[2], displayConfig.actCoefficients[3]);
		FLeiaCameraConstructionInfo constructionInfo = ConstructionInfo;
		ConstructionInfo.PanelResolutionHeight = displayConfig.panelResolution[1];
		ConstructionInfo.PanelResolutionWidth = displayConfig.panelResolution[0];
		if (displayMode == DisplayMode::MODE_3D) {
			ConstructionInfo.GridWidth = savedGridWidth;
			constructionInfo.GridWidth = ConstructionInfo.GridWidth;
			constructionInfo.RenderTextureResolutionHeight = ConstructionInfo.RenderTextureResolutionHeight;
			constructionInfo.RenderTextureResolutionWidth = ConstructionInfo.RenderTextureResolutionWidth;
		}
		else {
			constructionInfo.GridWidth = 1;
			savedGridWidth = ConstructionInfo.GridWidth;
			ConstructionInfo.GridWidth = 1;
			constructionInfo.RenderTextureResolutionHeight = displayConfig.panelResolution[1];
			constructionInfo.RenderTextureResolutionWidth = displayConfig.panelResolution[0];
		}
		CreateCameraGrid(constructionInfo);
	}

	if (displayMode == DisplayMode::MODE_3D) {
		SetInterlaceMaterialParams(type);
	} else{
		InterlaceMatParamCollectionInst->SetScalarParameterValue("viewsX", static_cast<float>(1));
		InterlaceMatParamCollectionInst->SetScalarParameterValue("viewsY", static_cast<float>(1));
	}

	SetViewSharpeningMaterialParams();

	if (Cameras.Num() == 0)
	{
		CreateCameraGrid(ConstructionInfo);
	}

	if (ZdpInfo.bUseZdpShear)
	{

		for (int camIndex = 0; camIndex < Cameras.Num(); ++camIndex)
		{
			USceneCaptureComponent2D* const camComponent = Cast<USceneCaptureComponent2D>(Cameras[camIndex]->GetComponentByClass(USceneCaptureComponent2D::StaticClass()));

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
		MatInstanceDynamicViewInterlacing->SetTextureParameterValue(*paramName, Cameras[camIndex]->GetCaptureComponent2D()->TextureTarget);
	}

	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 100.0");
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
	if (displayMode == DisplayMode::MODE_3D) {
		Device->DesiredLightFieldMode = Device->GetBacklightMode();
		Device->SetBacklightMode(BacklightMode::MODE_3D);
	}
	else {
		Device->DesiredLightFieldMode = Device->GetBacklightMode();
		Device->SetBacklightMode(BacklightMode::MODE_2D);
	}
}

// Called when the game starts or when spawned
void ALeiaCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	SetDeviceOverride();

	SetConstructionInfo(ConstructionInfo);
	savedGridWidth = ConstructionInfo.GridWidth;
	CommonMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(CommmonMatParamCollection);
	InterlaceMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(InterlaceMatParamCollection);
	ViewSharpeningMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(ViewSharpeningeMatParamCollection);
	isGameStarted = true;


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

	InitialPostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
	InitialPostProcessSettings.AddBlendable(MatInstanceDynamicViewSharpening, 1.0f);

	if (TargetCamera != nullptr)
	{
		UCameraComponent* const TargetCameraComponent_Temp_Camera = Cast<UCameraComponent>(TargetCamera->GetComponentByClass(UCameraComponent::StaticClass()));
		if (TargetCameraComponent_Temp_Camera != nullptr)
		{
			TargetCameraComponent = TargetCameraComponent_Temp_Camera;
			CachedPostProcessSettings = TargetCameraComponent_Temp_Camera->PostProcessSettings;
			TargetCameraComponent_Temp_Camera->PostProcessSettings = InitialPostProcessSettings;
			TargetCameraComponent->FieldOfView = RenderingInfo.FieldOfView;
			//TargetCameraComponent_Temp_Camera->PostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
			//TargetCameraComponent_Temp_Camera->PostProcessSettings.AddBlendable(MatInstanceDynamicViewSharpening, 1.0f);
		}

	}

	for (int camIndex = 0; camIndex < Cameras.Num(); ++camIndex)
	{
		USceneCaptureComponent2D* const camComponent = Cast<USceneCaptureComponent2D>(Cameras[camIndex]->GetComponentByClass(USceneCaptureComponent2D::StaticClass()));

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

	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "sg.resolutionquality 100.0");
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "r.MobileContentScaleFactor 4.0");

	Device->SetBacklightMode(BacklightMode::MODE_3D);
    bDisplayFrustumTemp = bDisplayFrustum;
    bDisplayFrustum = false;
#if !WITH_EDITOR
	bDisplayFrustum = false;
#endif
}

void ALeiaCameraPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (EndPlayReason == EEndPlayReason::Type::EndPlayInEditor || EndPlayReason == EEndPlayReason::Type::Quit)
    {
        bDisplayFrustum = bDisplayFrustumTemp;
    }
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
				USceneCaptureComponent2D* const sceneCaptureComponent = Cameras[wIndex]->GetCaptureComponent2D();
				if (sceneCaptureComponent != nullptr)
				{
					sceneCaptureComponent->CustomProjectionMatrix = CalculateProjectionMatrix(ConstructionInfo, RenderingInfo,
						Cameras[wIndex]->GetRootComponent()->GetRelativeLocation());
					sceneCaptureComponent->TextureTarget->UpdateResourceImmediate();
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
		ASceneCapture2D* const cameraObj = GetWorld()->SpawnActor<ASceneCapture2D>(CameraObjRef, params);
		if (cameraObj == nullptr)
		{
			return;
		}

#if WITH_EDITOR
		cameraObj->SetActorLabel(*cameraName);
#endif
		if (cameraObj)
		{

			cameraObj->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			cameraObj->SetActorRelativeLocation({ 0.0f, renderingInfo.Baseline * (wIndex - wOffset), 0.0f });

			USceneCaptureComponent2D* const sceneCaptureComponent = cameraObj->GetCaptureComponent2D();
			sceneCaptureComponent->FOVAngle = this->RenderingInfo.FieldOfView;

			sceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
			sceneCaptureComponent->bCaptureEveryFrame = true;
			sceneCaptureComponent->bCaptureOnMovement = false;
			sceneCaptureComponent->bAlwaysPersistRenderingState = false;
			if (displayMode == DisplayMode::MODE_3D) {
				sceneCaptureComponent->bUseCustomProjectionMatrix = true;
				sceneCaptureComponent->CustomProjectionMatrix = CalculateProjectionMatrix(constructionInfo, renderingInfo, { 0.0f, renderingInfo.Baseline * (wIndex - wOffset), 0.0f });
			}
			else {
				sceneCaptureComponent->bUseCustomProjectionMatrix = false;
			}


			Cameras.Add(cameraObj);
			if (displayMode == DisplayMode::MODE_2D) {
				CreateAndSetRenderTarget(this, sceneCaptureComponent, constructionInfo, 0.85f);
			}
			else {
				CreateAndSetRenderTarget(this, sceneCaptureComponent, constructionInfo, Gamma);
			}

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
			if (Cameras[cameraIndex] && !Cameras[cameraIndex]->IsUnreachable() && IsValid(Cameras[cameraIndex]))
			{
				UKismetRenderingLibrary::ReleaseRenderTarget2D(Cameras[cameraIndex]->GetCaptureComponent2D()->TextureTarget);
				Cameras[cameraIndex]->Destroy();
			}
		}

		Cameras.Empty();
	}
}

// Check if display mode value changed recently (return true on the next frame after changing value)

bool ALeiaCameraPawn::checkIfDisplayModeChanged() {
	if (displayMode == lastDisplayMode) {
		SetConstructionInfo(ConstructionInfo);
	}
	else {
		isDisplayModeChangedRecently = true;
		lastDisplayMode = displayMode;
	}

	if (isDisplayModeChangedRecently) {
		isDisplayModeChangedRecently = false;
		return true;
	}
	else {
		return false;
	}
}

// Change display mode to match current 3D or 2D mode

void ALeiaCameraPawn::changeDisplayMode() {
	if (displayMode == DisplayMode::MODE_3D) {
		OnScreenOrientationChanged(UBlueprintPlatformLibrary::GetDeviceOrientation());


		Device->SetBacklightMode(BacklightMode::MODE_3D);
		Device->DesiredLightFieldMode = Device->GetBacklightMode();
	}
	else {
		OnScreenOrientationChanged(UBlueprintPlatformLibrary::GetDeviceOrientation());

		Device->SetBacklightMode(BacklightMode::MODE_2D);
		Device->DesiredLightFieldMode = Device->GetBacklightMode();
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

	CommonMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(CommmonMatParamCollection);
	InterlaceMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(InterlaceMatParamCollection);
	ViewSharpeningMatParamCollectionInst = GetWorld()->GetParameterCollectionInstance(ViewSharpeningeMatParamCollection);


	if (checkIfDisplayModeChanged() && isGameStarted) {
		changeDisplayMode();
	}

	if (TargetCameraComponent == nullptr || (TargetCameraComponent != nullptr && (TargetCameraComponent->IsBeingDestroyed() || TargetCameraComponent->IsUnreachable() || !IsValid(TargetCameraComponent))))
	{
		TargetCameraComponent = Cast<UCameraComponent>(TargetCamera->GetComponentByClass(UCameraComponent::StaticClass()));
		TargetCameraComponent->PostProcessSettings.AddBlendable(MatInstanceDynamicViewInterlacing, 1.0f);
		TargetCameraComponent->PostProcessSettings.AddBlendable(MatInstanceDynamicViewSharpening, 1.0f);
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
					int32 blendableCount = Cameras[index]->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array.Num();
					for (int32 blendableIndex = 0; blendableIndex < blendableCount; blendableIndex++)
					{
						Cameras[index]->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array[blendableIndex].Weight = 1.0f;
					}
					SetPostProcessingValuesFromTargetCamera(Cameras[index]->GetCaptureComponent2D(), Cast<UCameraComponent>(TargetCameraComponent));
				}
			}
		}

		SetConvergenceValueFromRaycast(TargetCamera->GetActorLocation(), TargetCamera->GetActorForwardVector(), ZdpInfo, RenderingInfo, GetWorld());

		if (MatInstanceDynamicPositiveDOF != nullptr && MatInstancesDynamicZDP.Num() > 0 && Cameras.Num() > 2)
		{
			USceneCaptureComponent2D* const camA = Cameras[0]->GetCaptureComponent2D();
			USceneCaptureComponent2D* const camB = Cameras[1]->GetCaptureComponent2D();

			const float interviewDistance = GetInterviewDistanceUsingLeiaCamera(Cameras[0]->GetRootComponent()->GetRelativeLocation(),
				Cameras[1]->GetRootComponent()->GetRelativeLocation());

			if (ZdpInfo.bAutoZDP)
			{
				ZdpInfo.ZDPShear = -CalculateAutoZdpShearValue(ConstructionInfo, RenderingInfo, ZdpInfo, TargetCamera->GetTransform(), interviewDistance, camA, camB) * 100.0f;
			}

			SetCommonZdpMaterialParams(RenderingInfo, ConstructionInfo, ZdpInfo, camA, camB, TargetCamera->GetTransform());

			for (int index = 0; index < ConstructionInfo.GridWidth; ++index)
			{
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
					int32 blendableCount = Cameras[index]->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array.Num();
					for (int32 blendableIndex = 0; blendableIndex < blendableCount; blendableIndex++)
					{
						Cameras[index]->GetCaptureComponent2D()->PostProcessSettings.WeightedBlendables.Array[blendableIndex].Weight = 0.0f;
					}
					SetPostProcessingValuesFromTargetCamera(Cameras[index]->GetCaptureComponent2D(), Cast<UCameraComponent>(TargetCameraComponent));
					if (displayMode == DisplayMode::MODE_3D) {
						Cameras[index]->SetActorRelativeLocation({ 0.0f, RenderingInfo.Baseline * (index - wOffset), 0.0f });
					}
					else {
						Cameras[index]->SetActorRelativeLocation({0,0,0});
					}

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

	if (bDisplayFrustum && !(displayMode == DisplayMode::MODE_2D))
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
