// Fill out your copyright notice in the Description page of Project Settings.

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

	/** The Actor that contains a CameraComponent or SceneCaptureComponent2D */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Camera Grid Setup")
	AActor* TargetCamera = nullptr;

	/** TargetCamera component that will receive post-processed output*/
	UPROPERTY(VisibleAnywhere, Category = "Camera Grid Setup")
	class UCameraComponent* TargetCameraComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Camera Grid Setup")
	FLeiaCameraConstructionInfo ConstructionInfo;

	UPROPERTY(EditAnywhere, Category = "Camera Grid Setup")
	EViewMode DeviceConfigOverrideMode = EViewMode::Windows_15p6_12V;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Grid Setup")
	FLeiaCameraRenderingInfo RenderingInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Grid Setup")
	FLeiaCameraZdpInfo ZdpInfo;

	/** Gamma value applied to render texture */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Grid Setup", meta = (ClampMin = "0.01"))
	float Gamma = 2.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Grid Setup")
	UMaterialParameterCollection* CommmonMatParamCollection = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Grid Setup")
	UMaterialParameterCollection* InterlaceMatParamCollection = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Grid Setup")
	UMaterialParameterCollection* ViewSharpeningeMatParamCollection = nullptr;
	/** Array of cameras in the generated grid */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<class AActor*> Cameras;


	/** Can be called after modification of RenderingInfo to apply changes */
	UFUNCTION(BlueprintCallable)
	void RefreshCameraGrid();

	/** When called, A new set of cameras will be generated based on provided Construction Info */
	UFUNCTION(BlueprintCallable)
	void CreateCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo_);

	UFUNCTION(BlueprintCallable)
	class UMaterialInstanceDynamic* GetPositiveDOFMaterialInst() const;

	UFUNCTION(BlueprintCallable)
	class UMaterialInstanceDynamic* GetViewSharpeningMaterialInst() const;

	UFUNCTION(BlueprintCallable)
	class UMaterialInstanceDynamic* GetViewInterlacingMaterialInst() const;

	UFUNCTION(BlueprintCallable)
	bool IsZdpShearEnabled() const;

	UFUNCTION(BlueprintCallable)
	void SetZdpShearEnabled(bool isEnabled);

	/** Allows to set the parameters for the ACT/View sharpening material */
	UFUNCTION(BlueprintCallable)
	void SetACTCoEfficents(float A, float B);

	/** Toggle calibration square */
	UFUNCTION(BlueprintCallable)
	void ShowCalibrationSqaure(bool show);

	UFUNCTION(BlueprintCallable)
	float GetCurrentShearValue() const;

	IRendererModule* RendererModule;
	InterlaceParams interlaceShaderParams;
	SharpenParams sharpenShaderParams;
protected:

	UPROPERTY(VisibleDefaultsOnly)
	USceneComponent* SceneRoot = nullptr;


	UPROPERTY(EditAnywhere, Category = "Debug Display")
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

	void Destroyed() override;

	void OnConstruction(const FTransform& Transform) override;

	void SetDeviceOverride();

private:

	void SpawnCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo);
	void DestroyCamerasAndReleaseRenderTargets();

};
