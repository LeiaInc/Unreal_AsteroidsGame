/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SceneViewExtension.h"
#include "LeiaCameraBase.h"
#include "LeiaCameraPawn.generated.h"




UCLASS()
class LEIACAMERA_API ALeiaCameraPawn : public APawn, public LeiaCameraBase
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ALeiaCameraPawn();

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool useACT = true;
	void ToggleACT();

	/** The Actor that contains a CameraComponent or SceneCaptureComponent2D */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Leia Camera Pawn Setup")
	AActor* TargetCamera = nullptr;

	/** TargetCamera component that will receive post-processed output*/
	UPROPERTY(VisibleAnywhere, Category = "Leia Camera Pawn Setup")
	class UCameraComponent* TargetCameraComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Leia Camera Pawn Setup")
	FLeiaCameraConstructionInfo ConstructionInfo;

	UPROPERTY(EditAnywhere, Category = "Leia Camera Pawn Setup")
	EViewMode DeviceConfigOverrideMode = EViewMode::AndroidPegasus_12p3_8V;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Pawn Setup")
	FLeiaCameraRenderingInfo RenderingInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Pawn Setup")
	FLeiaCameraZdpInfo ZdpInfo;

	/** Gamma value applied to render texture */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leia Camera Pawn Setup", meta = (ClampMin = "0.01"))
	float Gamma = 2.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leia Camera Pawn Setup")
	UMaterialParameterCollection* CommmonMatParamCollection = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leia Camera Pawn Setup")
	UMaterialParameterCollection* InterlaceMatParamCollection = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leia Camera Pawn Setup")
	UMaterialParameterCollection* ViewSharpeningeMatParamCollection = nullptr;
	/** Array of cameras in the generated grid */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Leia Camera Pawn Setup")
	TArray<class AActor*> Cameras;

	/** Can be called after modification of RenderingInfo to apply changes */
	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	void RefreshCameraGrid();

	/** When called, A new set of cameras will be generated based on provided Construction Info */
	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	void CreateCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo_);

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	class UMaterialInstanceDynamic* GetPositiveDOFMaterialInst() const;

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	class UMaterialInstanceDynamic* GetViewSharpeningMaterialInst() const;

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	class UMaterialInstanceDynamic* GetViewInterlacingMaterialInst() const;

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	bool IsZdpShearEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	void SetZdpShearEnabled(bool isEnabled);

	/** Allows to set the parameters for the ACT/View sharpening material */
	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	void SetACTCoEfficents(float A, float B);

	/** Toggle calibration square */
	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	void ShowCalibrationSqaure(bool show);

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Pawn Setup")
	float GetCurrentShearValue() const;

	IRendererModule* RendererModule;
	InterlaceParams interlaceShaderParams;
	SharpenParams sharpenShaderParams;
protected:

	UPROPERTY(VisibleDefaultsOnly, Category = "Leia Camera Pawn Setup")
	USceneComponent* SceneRoot = nullptr;


	UPROPERTY(EditAnywhere, Category = "Leia Camera Pawn Setup")
	bool bDisplayFrustum = true;

	const FName PropertiesThatRegenerateGrid = GET_MEMBER_NAME_CHECKED(ALeiaCameraPawn, ConstructionInfo);
	const FName PropertiesThatRequireProjectionMatrixRecalculation = GET_MEMBER_NAME_CHECKED(ALeiaCameraPawn, RenderingInfo);

	UFUNCTION()
	void OnScreenOrientationChanged(EScreenOrientation::Type type);

	UFUNCTION()
	void AppEnterBackground();

	UFUNCTION()
	void AppEnterForeground();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Should tick be called even if actor is in the viewport and not in game */
	virtual bool ShouldTickIfViewportsOnly() const override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;

	void OnConstruction(const FTransform& Transform) override;

	void SetDeviceOverride();

private:

	void SpawnCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo);
	void DestroyCamerasAndReleaseRenderTargets();

};
