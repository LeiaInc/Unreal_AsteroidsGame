/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

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
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Leia Camera Component Setup")
	class UCameraComponent* TargetCamera = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Leia Camera Component Setup")
	FLeiaCameraConstructionInfo ConstructionInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Component Setup")
	FLeiaCameraRenderingInfo RenderingInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Component Setup")
	FLeiaCameraZdpInfo ZdpInfo;

	/** Gamma value applied to render texture */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Leia Camera Component Setup", meta = (ClampMin = "0.01"))
	float Gamma = 2.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leia Camera Component Setup")
	UMaterialParameterCollection* CommmonMatParamCollection = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leia Camera Component Setup")
	UMaterialParameterCollection* InterlaceMatParamCollection = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leia Camera Component Setup")
	UMaterialParameterCollection* ViewSharpeningeMatParamCollection = nullptr;

	/** Array of cameras in the generated grid */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, NonTransactional, Category = "Leia Camera Component Setup")
	TArray<class USceneCaptureComponent2D*> Cameras;

	/** Can be called after modification of RenderingInfo to apply changes */
	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	void RefreshCameraGrid();

	/** When called, A new set of cameras will be generated based on provided Construction Info */
	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	void CreateCameraGrid(const FLeiaCameraConstructionInfo& constructionInfo_);

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	class UMaterialInstanceDynamic* GetPositiveDOFMaterialInst() const;

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	class UMaterialInstanceDynamic* GetViewSharpeningMaterialInst() const;

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	class UMaterialInstanceDynamic* GetViewInterlacingMaterialInst() const;

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	bool IsZdpShearEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	void SetZdpShearEnabled(bool isEnabled);

	/** Allows to set the parameters for the ACT/View sharpening material */
	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	void SetACTCoEfficents(float A, float B);

	/** Toggle calibration square */
	UFUNCTION(BlueprintCallable, Category = "Leia Camera Component Setup")
	void ShowCalibrationSqaure(bool show);

	void SetDeviceOverride();

protected:

	UPROPERTY(VisibleDefaultsOnly, Category = "Leia Camera Component Setup")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(EditAnywhere, Category = "Leia Camera Component Setup")
	bool bDisplayFrustum = true;

	UPROPERTY(VisibleAnywhere, Category = "Leia Camera Component Setup")
	EViewMode ViewMode = EViewMode::None;

	UFUNCTION(Category = "Leia Camera Component Setup")
	void OnScreenOrientationChanged(EScreenOrientation::Type type);

	UFUNCTION(Category = "Leia Camera Component Setup")
	void AppEnterBackground();

	UFUNCTION(Category = "Leia Camera Component Setup")
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
