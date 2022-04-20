/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Containers/UnrealString.h"
#include "AbstractLeiaDevice.h"
#include "Kismet/BlueprintPlatformLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Engine/EngineTypes.h"
#include "Camera/CameraTypes.h"
#include "LeiaCameraBase.generated.h"

class UMaterialInstanceDynamic;

class ASceneCapture2D;
class USceneComponent;

#define LEIA_STEREO_PATH (0)



UCLASS()
class LEIACAMERA_API UViewDataComp : public USceneComponent
{
	GENERATED_BODY()

public:
	FMatrix CustomProjectionMatrix;
};

UCLASS()
class LEIACAMERA_API AViewData : public AActor
{
	GENERATED_BODY()

public:
	AViewData(const FObjectInitializer& ObjectInitializer);
	void Serialize(FArchive& Ar);

	//Makes bouncing between 7summits and this path easier
	UViewDataComp* GetCaptureComponent2D() { return sceneComp; }

	UViewDataComp* sceneComp;
};

#if LEIA_STEREO_PATH
typedef AViewData CaptureActor;
typedef UViewDataComp CaptureComponent;
#else
typedef ASceneCapture2D CaptureActor;
typedef USceneCaptureComponent2D CaptureComponent;
#endif


UENUM(BlueprintType)
enum class EViewMode : uint8 {
	LumePad UMETA(DisplayName = "LumePad"),
	Windows_12p5_8V UMETA(DisplayName = "Windows_12p5_8V"),
	AndroidPegasus_12p5_8V UMETA(DisplayName = "AndroidPegasus_12p5_8V"),
	AndroidPegasus_12p3_8V UMETA(DisplayName = "AndroidPegasus_12p3_8V"),
	Windows_15p6_12V UMETA(DisplayName = "Windows_15p6_12V"),
	Windows_15p6_13V UMETA(DisplayName = "Windows_15p6_13V"),
	None UMETA(DisplayName = "NoOverride_UseFirmware")
};

USTRUCT(BlueprintType)
struct FLeiaCameraConstructionInfo
{
	GENERATED_BODY()

	/** Number of cameras spawned horizontally in the grid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "4", ClampMax = "8"), Category = "Leia Camera Construction Info")
	int32 GridWidth = 8;

	/** Number of pixels horizontally in a texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "32"), Category = "Leia Camera Construction Info")
	int32 RenderTextureResolutionWidth = 1280;

	/** Number of pixels vertically in a texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "32"), Category = "Leia Camera Construction Info")
	int32 RenderTextureResolutionHeight = 720;
};


USTRUCT(BlueprintType)
struct FLeiaCameraRenderingInfo
{
	GENERATED_BODY()

	/** Distance scaling between spawned cameras */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "128.0"), Category = "Leia Camera Rendering Info")
	float Baseline = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Rendering Info")
	TEnumAsByte<ECameraProjectionMode::Type> ProjectionType = ECameraProjectionMode::Type::Perspective;

	/** Field of view in degrees applied to the cameras when in perspective mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", ClampMax = "170.0"), Category = "Leia Camera Rendering Info")
	float FieldOfView = 60.0f;

	/** Units in view of the camera when in Ortho mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0"), Category = "Leia Camera Rendering Info")
	float OrthographicWidth = 640.0f;

	/** Distance to near clip plane */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Rendering Info")
	float NearClipPlane = 10.0f;

	/** Distance to far clip plane */
	UPROPERTY(BlueprintReadWrite, Category = "Leia Camera Rendering Info")
	float FarClipPlane = 5000.0f;

	/** Distance ahead of each camera where their frustums converge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0"), Category = "Leia Camera Rendering Info")
	float ConvergenceDistance = 200.0f;

	/** PostProcess Material To Use for DOF*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Rendering Info")
	class UMaterialInterface* PostProcessMaterialPositiveDOF = nullptr;

	/** PostProcess Material To Use for ZDP*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Rendering Info")
	class UMaterialInterface* PostProcessMaterialZDP = nullptr;

	/** PostProcess Material To Use for Interlacing*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Rendering Info")
	class UMaterialInterface* PostProcessMaterialViewInterlacing = nullptr;

	/** PostProcess Material To Use For View Sharpening*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera Rendering Info")
	class UMaterialInterface* PostProcessMaterialViewSharpening = nullptr;

};

USTRUCT(BlueprintType)
struct FLeiaCameraZdpInfo
{
	GENERATED_BODY()

	/** Enable ZDP Shear*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera ZDP Info")
	bool bUseZdpShear = false;

	/** Enable Auto ZDP */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera ZDP Info")
	bool bAutoZDP = false;

	/** When AutoZDP is disabled, the ZDPShear value will be used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera ZDP Info")
	float ZDPShear = 0.0f;

	/** Number of pixels vertically in a texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera ZDP Info")
	float ConvergenceOffset = 0.0f;

	/** The ObjectType to consider while raycast is performed to calculate ZDP and convergence value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leia Camera ZDP Info")
	TEnumAsByte<EObjectTypeQuery> ObjectTypeToQuery;
};

struct InterlaceParams
{
	float viewsX; float viewsY; FLinearColor viewOffset; FLinearColor viewRes;
	FLinearColor intVecs; FLinearColor matA; FLinearColor matB; FLinearColor matC; FLinearColor matD;
};

struct SharpenParams
{
	float gamma; 
	float sharpeningCenter;
	FLinearColor sharpeningSize;
	FLinearColor sharpeningX;
	FLinearColor sharpeningY;
	FLinearColor textureInvSize;
};

class LEIACAMERA_API LeiaCameraBase
{
public:
	LeiaCameraBase();
	~LeiaCameraBase();

protected:

	enum class RefreshState { INTIAL, READY, COMPLETED };
	RefreshState CurrentRefreshState = RefreshState::INTIAL;

	/** The abstracted device, allows accessing various info about the device the app is running on as well as setting back-light state */
	AbstractLeiaDevice* Device = nullptr;

	/** Current screen orientation */
	EScreenOrientation::Type CurrentScreenOrientation = EScreenOrientation::Unknown;
	EScreenOrientation::Type PrevScreenOrientation = EScreenOrientation::LandscapeLeft;

	UMaterialInstanceDynamic* MatInstanceDynamicPositiveDOF = nullptr;
	TArray<UMaterialInstanceDynamic*> MatInstancesDynamicZDP;
	UMaterialInstanceDynamic* MatInstanceDynamicViewInterlacing = nullptr;
	UMaterialInstanceDynamic* MatInstanceDynamicViewSharpening = nullptr;

	/** Material parameter collection allows setting of various material parameters across different materials */
	UMaterialParameterCollectionInstance* CommonMatParamCollectionInst = nullptr;
	UMaterialParameterCollectionInstance* InterlaceMatParamCollectionInst = nullptr;
	UMaterialParameterCollectionInstance* ViewSharpeningMatParamCollectionInst = nullptr;

	FPostProcessSettings CachedPostProcessSettings;
	FPostProcessSettings InitialPostProcessSettings;
	float PrevConvergenceDistance = FLT_MAX;
	bool PrevUseZdpShear = false;

	/** Display camera frustum
	* @param camera - custom projection matrix of USceneCaptureComponent2D (camera) is used to displayed the frustum
	*/
	void DisplayCameraFrustum(const FTransform& localTransform, const FTransform& targetCamTransform, const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo, UWorld* const world) const;

	/**
	 * Sets the construction info struct based on what device it's running on
	 * @param constructionInfo - takes constructionInfo as reference from the Actor/Component it needs to set it for
	 */
	void SetConstructionInfo(FLeiaCameraConstructionInfo& constructionInfo);

	/**
	 * Creates and sets up the render texture in the way that's required
	 * @param worldContext - The world context
	 * @param scenecaptureComp - The SceneCaptureComponent2D that will receive the render texture
	 * @param constructionInfo - The Construction Info that should be used while making the render texture
	 * @param gamma - Sets the target gamma value for render texture
	 */
	void CreateAndSetRenderTarget(UObject* const worldContext, class USceneCaptureComponent2D* const scenecaptureComp, const FLeiaCameraConstructionInfo& constructionInfo, float gamma = 2.2f) const;

	void SetPostProcessingValuesFromTargetCamera(USceneCaptureComponent2D* const scenecaptureComp, class UCameraComponent* const targetCamera);

	/**
	 * Calculates the grid width offset based on the provided Construction Info
	 */
	void CalculateGridOffset(const FLeiaCameraConstructionInfo& constructionInfo, float& widthOffset) const;

	/**
	 * Generates a unique name for a camera in a grid based on it's index and provided Construction Info
	 */
	FString GetCameraNameFromGridIndex(const FLeiaCameraConstructionInfo& constructionInfo, int32 wIndex) const;

	/**
	 * Calculates and returns a custom projection matrix based on Construction Info, Rendering Info and Index of camera in grid
	 */
	FMatrix CalculateProjectionMatrix(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo, const FVector& localCamPos) const;

	/**
	 * Re-orient the plane's normal to match up with actor's world space rotation
	 * @param forward - Actor's forward vector
	 * @param right - Actor's right vector
	 */
	void SetProperPlaneOrientation(const FVector& forward, const FVector& right, FPlane& plane) const;

	/**
	 * Returns the name of the Device this is running on. (Works for Android)
	 */
	FString GetDeviceName() const;

	/**
	 * Return calculated near plane distance to be sent as parameter to material
	 */
	float GetNearPlaneDistance(const FLeiaCameraRenderingInfo& renderingInfo) const;

	/**
	 * Return calculated far plane distance to be sent as parameter to material
	 */
	float GetFarPlaneDistance(const FLeiaCameraRenderingInfo& renderingInfo) const;

	/**
	 * Returns ZDP Shear scalar value which is then passed to the material as a parameter
	 */
	float GetZdpShearScaler(const FLeiaCameraRenderingInfo& renderingInfo) const;

	/**
	 * Returns the distance between two cameras divided by the panel resolution
	 * @param camAlocalPos - Relative location of camA, any camera as long as it's adjacent to camB
	 * @param camAlocalPos - Relative location of camB, any camera as long as it's adjacent to camA
	 * @note - cameras at index 0 and 1, as long as it's adjacent
	 */
	float GetInterviewDistanceUsingLeiaCamera(const FVector& camAlocalPos, const FVector& camBlocalPos) const;

	/**
	 * Returns the local position of any camera divided by panel resolution
	 * @param localPos - Relative position of any camera
	 */
	float GetSignedViewDistanceToCenter(const FVector& localPos) const;

	/**
	 * Returns a point on a circle on the Y-Z plane
	 * @param degrees - Angle around the circle to retrieve a point for
	 * @param distance - Distance away from the center to get a point
	 * @param center - location around which a point is to be retrieved
	 */
	FVector GetPointOnCircle(float degrees, float distance, const FVector& center) const;

	/**
	 * Returns the distance of a point from a raycast based on object type filter
	 */
	float GetRaycastDistance(const FVector& raycastPosition, const FVector& raycastDir, UWorld* world, const FLeiaCameraZdpInfo& zdpInfo) const;

	/**
	 * Sets the convergence value based on raycast with object type filter
	 * @param loc - Position from where the raycast starts
	 * @param forwardDir - The direction of the raycast
	 */
	bool SetConvergenceValueFromRaycast(const FVector& loc, const FVector& forwardDir, const FLeiaCameraZdpInfo& zdpInfo, FLeiaCameraRenderingInfo& renderingInfo, UWorld* world) const;

	/**
	 * Returns a vector which lies along the bottom plane of the frustum along the middle
	 */
	FVector GetCameraFrustumNearCenter(UWorld* world, const FTransform& targetCamera, const FMatrix& customProjectionMatrix, const FLeiaCameraRenderingInfo& renderingInfo, const FLeiaCameraZdpInfo& zdpInfo) const;

	/**
	 * Returns screen space location of world space location
	 * @param worldPos - World space location
	 * @param camera - The scene capture component which will be considered while converting world to screen point
	 */
	FVector WorldToScreenPoint(const FVector& worldPos, CaptureComponent* camera) const;

	/**
	 * Returns the value for ZDP while 'auto ZDP' is enabled
	 */
	float CalculateAutoZdpShearValue(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo, 
		const FLeiaCameraZdpInfo& zdpInfo, const FTransform& targetCamTransform, float interviewDistance, 
		CaptureComponent * camA, CaptureComponent * camB)const;

	/** Returns the value for horizontal relative position of the cameras */
	float UpdateViews(int index, const FLeiaCameraRenderingInfo& renderingInfo, const FLeiaCameraConstructionInfo& constructionInfo, bool emissionRescalingEnabled = true) const;

	/**
	 * Sets all common ZDP material params by using a MaterialParameterCollectionInstance
	 */
	void SetCommonZdpMaterialParams(const FLeiaCameraRenderingInfo& renderingInfo, const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraZdpInfo& zdpInfo, CaptureComponent* camA, CaptureComponent* camB, const FTransform& targetCamTransform);

	/**
	 * Sets all Interlacing material params by using a MaterialParameterCollectionInstance
	 */
	 void SetInterlaceMaterialParams(EScreenOrientation::Type orientation);



	 void GetInterlaceParams(EScreenOrientation::Type orientation, InterlaceParams & interlaceParams);

	 void GetViewSharpeningParams(const FDisplayConfig& config, SharpenParams& sharpenShaderParams);



	 /**
	 * Sets all View sharpening material params by using a MaterialParameterCollectionInstance
	 */
	 void SetViewSharpeningMaterialParams();
};
