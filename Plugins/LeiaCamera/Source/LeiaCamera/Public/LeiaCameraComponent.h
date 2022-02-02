// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeiaCameraBase.h"
#include "Components/SceneComponent.h"
#include "LeiaCameraComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LEIACAMERA_API ULeiaCameraComponent : public USceneComponent, public LeiaCameraBase
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULeiaCameraComponent();

	/** TargetCamera component that will receive post-processed output*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Camera Grid Setup")
	class UCameraComponent* TargetCamera = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Camera Grid Setup")
	FLeiaCameraConstructionInfo ConstructionInfo;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, NonTransactional)
	TArray<class USceneCaptureComponent2D*> Cameras;

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

protected:

	UPROPERTY(VisibleDefaultsOnly)
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(EditAnywhere, Category = "Debug Display")
	bool bDisplayFrustum = true;

	UPROPERTY(VisibleAnywhere, Category = "Debug Display")
	EViewMode ViewMode = EViewMode::FourView;

	UFUNCTION()
	void OnScreenOrientationChanged(EScreenOrientation::Type type);

	UFUNCTION()
	void AppEnterBackground();

	UFUNCTION()
	void AppEnterForeground();

	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void OnRegister() override;

	virtual void OnUnregister() override;

	virtual void DestroyComponent(bool bPromoteChildren = false) override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

	void SpawnCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo);
	void DestroyCamerasAndReleaseRenderTargets();
};
