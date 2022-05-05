/****************************************************************
*
* Copyright 2022 © Leia Inc.
*
****************************************************************
*/

#include "LeiaCameraBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
//#include "Components/USceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Components/LineBatchComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#if PLATFORM_ANDROID
#include "AndroidLeiaDevice.h"
#elif PLATFORM_WINDOWS && !WITH_EDITOR
#include "WindowsLeiaDevice.h"
#endif
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Engine/SceneCapture2D.h"
#include "ILeiaDevice.h"
#include "DrawDebugHelpers.h"

struct CameraCalculatedParams
{
	float ScreenHalfHeight;
	float ScreenHalfWidth;
	float EmissionRescalingFactor = 1.0f;

	CameraCalculatedParams(FLeiaCameraRenderingInfo renderingInfo, const FDisplayConfig::OrientationConfig& display, bool emissionRescalingEnabled = true)
	{
		ScreenHalfHeight = renderingInfo.ConvergenceDistance * FMath::Tan(renderingInfo.FieldOfView);
		ScreenHalfWidth = (display.panelResolution[0] / display.panelResolution[1]) * ScreenHalfHeight;
		float f = (display.viewResolution[1] / 1.0f) / 2.0f / FMath::Tan(renderingInfo.FieldOfView);
		if (emissionRescalingEnabled)
		{
			EmissionRescalingFactor = display.systemDisparityPixels * renderingInfo.Baseline * renderingInfo.ConvergenceDistance / f;
		}
	}
};

LeiaCameraBase::LeiaCameraBase()
{
#if PLATFORM_ANDROID
	Device = new AndroidLeiaDevice();
#elif LEIA_USE_SERVICE
	Device = new WindowsLeiaDevice();
#else
	Device = new AbstractLeiaDevice();
#endif
}

LeiaCameraBase::~LeiaCameraBase()
{
	delete Device;
}

void LeiaCameraBase::DisplayCameraFrustum(const FTransform& localTransform, const FTransform& targetCamTransform, const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo, UWorld* const world) const
{
	if (world == nullptr)
	{
		return;
	}
	FLinearColor nearComfortZoneColor = FLinearColor::Green;
	FLinearColor farComfortZoneColor = FLinearColor::Blue;
	FLinearColor convergencePlaneColor = FLinearColor::Yellow;
	float batchedLineLifeTime = 2.0f;
	float batchedLineThickness = -5.0f;
	uint8 batchedLineDepthPriority = 77;

	TArray<FBatchedLine> finalBatchedLines;
	finalBatchedLines.Reserve(16 * constructionInfo.GridWidth);

	const FVector forwardVec = targetCamTransform.GetUnitAxis(EAxis::X);
	const FVector rightVec = targetCamTransform.GetUnitAxis(EAxis::Y);
	const bool isOrthoMode = renderingInfo.ProjectionType == ECameraProjectionMode::Orthographic;

	float aspectRatio = 0.0f;
	const FDisplayConfig config = Device->GetDisplayConfig();

	if (CurrentScreenOrientation == EScreenOrientation::LandscapeLeft || CurrentScreenOrientation == EScreenOrientation::LandscapeRight || CurrentScreenOrientation == EScreenOrientation::Unknown)
	{
		aspectRatio = static_cast<float>(config.viewResolution[1]) / config.viewResolution[0];
	}
	else if (CurrentScreenOrientation == EScreenOrientation::Portrait || CurrentScreenOrientation == EScreenOrientation::PortraitUpsideDown)
	{
		aspectRatio = static_cast<float>(config.viewResolution[0]) / config.viewResolution[1];
	}

	const float height = (isOrthoMode) ? renderingInfo.OrthographicWidth * 0.5f : 1.0f;
	const float width = (isOrthoMode) ? height * (1.0f / aspectRatio) : 1.0f;

	for (int32 camIndex = 0; camIndex < constructionInfo.GridWidth; camIndex++)
	{
		const FVector relativeLocalPos = { 0.0f, UpdateViews(camIndex, renderingInfo, constructionInfo, false), 0.0f };
		const FMatrix projectionMat = CalculateProjectionMatrix(constructionInfo, renderingInfo, relativeLocalPos);
		const FVector cameraLocation = localTransform.TransformPositionNoScale(relativeLocalPos);

		FPlane topPlane, btmPlane, lftPlane, rgtPlane, farPlane, nearPlane, convergencePlane;

		const FMatrix viewMat = localTransform.ToMatrixNoScale().ConcatTranslation(relativeLocalPos);
		const FMatrix viewProjectionMatrix = viewMat.Inverse() * projectionMat;

		viewProjectionMatrix.GetFrustumTopPlane(topPlane);
		viewProjectionMatrix.GetFrustumBottomPlane(btmPlane);
		viewProjectionMatrix.GetFrustumLeftPlane(lftPlane);
		viewProjectionMatrix.GetFrustumRightPlane(rgtPlane);
		viewProjectionMatrix.GetFrustumFarPlane(farPlane);

		// Need offset along normal away from origin

		topPlane.W = height;
		btmPlane.W = height;
		lftPlane.W = width;
		rgtPlane.W = width;

		SetProperPlaneOrientation(forwardVec, rightVec, topPlane);
		SetProperPlaneOrientation(forwardVec, rightVec, btmPlane);
		SetProperPlaneOrientation(forwardVec, rightVec, lftPlane);
		SetProperPlaneOrientation(forwardVec, rightVec, rgtPlane);

		if (isOrthoMode)
		{
			farPlane = FPlane(forwardVec, GetFarPlaneDistance(renderingInfo));
		}
		else
		{
			farPlane.W = (GetFarPlaneDistance(renderingInfo) > 0.0f) ? -GetFarPlaneDistance(renderingInfo) : GetFarPlaneDistance(renderingInfo);
		}

        
		nearPlane = farPlane;
        nearPlane.W = ((isOrthoMode) ? 1 : -1) * GetNearPlaneDistance(renderingInfo);

		convergencePlane = farPlane;
		convergencePlane.W = ((isOrthoMode) ? 1 : -1) * renderingInfo.ConvergenceDistance;

		if (!isOrthoMode)
		{
			SetProperPlaneOrientation(forwardVec, rightVec, farPlane);
			SetProperPlaneOrientation(forwardVec, rightVec, nearPlane);
			SetProperPlaneOrientation(forwardVec, rightVec, convergencePlane);
		}

        //F: front, L: left, R: right, T: top, B: bottom

		FVector FLTintersectAtFarPlane, FRTintersectAtFarPlane, FLBintersectAtFarPlane, FRBintersectAtFarPlane;

		if (!FMath::IntersectPlanes3(FLTintersectAtFarPlane, farPlane, lftPlane, topPlane))	{ return; }
		if (!FMath::IntersectPlanes3(FRTintersectAtFarPlane, farPlane, rgtPlane, topPlane)) { return; }
		if (!FMath::IntersectPlanes3(FLBintersectAtFarPlane, farPlane, lftPlane, btmPlane)) { return; }
		if (!FMath::IntersectPlanes3(FRBintersectAtFarPlane, farPlane, rgtPlane, btmPlane))	{ return; };

		FVector FLTintersectAtNearPlane, FRTintersectAtNearPlane, FLBintersectAtNearPlane, FRBintersectAtNearPlane;

		if (!FMath::IntersectPlanes3(FLTintersectAtNearPlane, nearPlane, lftPlane, topPlane)) { return; }
		if (!FMath::IntersectPlanes3(FRTintersectAtNearPlane, nearPlane, rgtPlane, topPlane))	{ return; }
		if (!FMath::IntersectPlanes3(FLBintersectAtNearPlane, nearPlane, lftPlane, btmPlane))	{ return; }
		if (!FMath::IntersectPlanes3(FRBintersectAtNearPlane, nearPlane, rgtPlane, btmPlane))	{ return; }

		FVector FLTintersectAtConvergence, FRTintersectAtConvergence, FLBintersectAtConvergence, FRBintersectAtConvergence;

		if (!FMath::IntersectPlanes3(FLTintersectAtConvergence, convergencePlane, lftPlane, topPlane))	{ return; }
		if (!FMath::IntersectPlanes3(FRTintersectAtConvergence, convergencePlane, rgtPlane, topPlane))	{ return; }
		if (!FMath::IntersectPlanes3(FLBintersectAtConvergence, convergencePlane, lftPlane, btmPlane))	{ return; }
		if (!FMath::IntersectPlanes3(FRBintersectAtConvergence, convergencePlane, rgtPlane, btmPlane))	{ return; }

		FLTintersectAtFarPlane += cameraLocation;
		FRTintersectAtFarPlane += cameraLocation;
		FLBintersectAtFarPlane += cameraLocation;
		FRBintersectAtFarPlane += cameraLocation;

		FLTintersectAtNearPlane += cameraLocation;
		FRTintersectAtNearPlane += cameraLocation;
		FLBintersectAtNearPlane += cameraLocation;
		FRBintersectAtNearPlane += cameraLocation;

		FLTintersectAtConvergence += cameraLocation;
		FRTintersectAtConvergence += cameraLocation;
		FLBintersectAtConvergence += cameraLocation;
		FRBintersectAtConvergence += cameraLocation;

		// Combine all lines into a single array of batched lines
		
		//farComfortZone - back plane
		finalBatchedLines.Push({ FLTintersectAtFarPlane, FRTintersectAtFarPlane, farComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FRTintersectAtFarPlane, FRBintersectAtFarPlane, farComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FRBintersectAtFarPlane, FLBintersectAtFarPlane, farComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FLBintersectAtFarPlane, FLTintersectAtFarPlane, farComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });

		//farComfortZone - convergence to back plane
		finalBatchedLines.Push({ FLTintersectAtFarPlane, FLTintersectAtConvergence, farComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FRTintersectAtFarPlane, FRTintersectAtConvergence, farComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FRBintersectAtFarPlane, FRBintersectAtConvergence, farComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FLBintersectAtFarPlane, FLBintersectAtConvergence, farComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		
		//nearComfortZone - front plane
		finalBatchedLines.Push({ FLTintersectAtNearPlane, FRTintersectAtNearPlane, nearComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FRTintersectAtNearPlane, FRBintersectAtNearPlane, nearComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FLBintersectAtNearPlane, FLTintersectAtNearPlane, nearComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FRBintersectAtNearPlane, FLBintersectAtNearPlane, nearComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
        
		//nearComfortZone - front plane to convergence plane
        finalBatchedLines.Push({ FRTintersectAtNearPlane, FRTintersectAtConvergence, nearComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
        finalBatchedLines.Push({ FRBintersectAtNearPlane, FRBintersectAtConvergence, nearComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
        finalBatchedLines.Push({ FLTintersectAtNearPlane, FLTintersectAtConvergence, nearComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
        finalBatchedLines.Push({ FLBintersectAtNearPlane, FLBintersectAtConvergence, nearComfortZoneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
        
		//convergencePlane
		finalBatchedLines.Push({ FLTintersectAtConvergence, FRTintersectAtConvergence, convergencePlaneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FRTintersectAtConvergence, FRBintersectAtConvergence, convergencePlaneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FLBintersectAtConvergence, FLTintersectAtConvergence, convergencePlaneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
		finalBatchedLines.Push({ FRBintersectAtConvergence, FLBintersectAtConvergence, convergencePlaneColor, batchedLineLifeTime, batchedLineThickness, batchedLineDepthPriority });
        
        //UKismetSystemLibrary::DrawDebugPlane(world, topPlane, nearPlane, 100, FLinearColor::White, 1000);
        //DrawDebugPoint(world, farPlane, 200, FColor(52, 220,239), true);
        
	}

    
	world->LineBatcher->DrawLines(finalBatchedLines);
}

void LeiaCameraBase::SetConstructionInfo(FLeiaCameraConstructionInfo& constructionInfo)
{
	const FDisplayConfig displayConfig = Device->GetDisplayConfig();
	constructionInfo.RenderTextureResolutionWidth = displayConfig.viewResolution[0];
	constructionInfo.RenderTextureResolutionHeight = displayConfig.viewResolution[1];
	constructionInfo.GridWidth = displayConfig.numViews[0];
}

void LeiaCameraBase::CreateAndSetRenderTarget(UObject* const worldContext, USceneCaptureComponent2D* const scenecaptureComp, const FLeiaCameraConstructionInfo& constructionInfo, float gamma /* = 2.2f*/) const
{
	float resX = 0.0f, resY = 0.0f;
	if (CurrentScreenOrientation == EScreenOrientation::LandscapeLeft || CurrentScreenOrientation == EScreenOrientation::LandscapeRight || CurrentScreenOrientation == EScreenOrientation::Unknown)
	{
		resX = constructionInfo.RenderTextureResolutionWidth;
		resY = constructionInfo.RenderTextureResolutionHeight;
	}
	else if (CurrentScreenOrientation == EScreenOrientation::Portrait || CurrentScreenOrientation == EScreenOrientation::PortraitUpsideDown)
	{
		resX = constructionInfo.RenderTextureResolutionHeight;
		resY = constructionInfo.RenderTextureResolutionWidth;
	}

	UTextureRenderTarget2D* const renderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(worldContext, resX, resY, ETextureRenderTargetFormat::RTF_RGBA32f);

	if (renderTarget)
	{
		renderTarget->Filter = TextureFilter::TF_Nearest;
		renderTarget->bAutoGenerateMips = false;
		renderTarget->LODGroup = TextureGroup::TEXTUREGROUP_Pixels2D;
		renderTarget->UpdateResourceImmediate(true);
		renderTarget->TargetGamma = gamma;
		scenecaptureComp->TextureTarget = renderTarget;
	}
}

void LeiaCameraBase::SetPostProcessingValuesFromTargetCamera(USceneCaptureComponent2D* const scenecaptureComp, UCameraComponent* const targetCamera)
{
	FPostProcessSettings& DstPPSettings = scenecaptureComp->PostProcessSettings;

	FWeightedBlendables DstWeightedBlendables = DstPPSettings.WeightedBlendables;

	// Copy all of the post processing settings
	DstPPSettings = CachedPostProcessSettings;

	// But restore the original blendables
	DstPPSettings.WeightedBlendables = DstWeightedBlendables;

	scenecaptureComp->PostProcessSettings = DstPPSettings;
}

void LeiaCameraBase::CalculateGridOffset(const FLeiaCameraConstructionInfo& constructionInfo, float& widthOffset) const
{
	// If GridWidth is even then return half of the next lower odd number
	// If GridWidth is odd then return half of GridWidth but floor it's value to a whole number
	widthOffset = (constructionInfo.GridWidth % 2 == 0) ? (constructionInfo.GridWidth - 1) * 0.5f : FMath::FloorToFloat(constructionInfo.GridWidth * 0.5f);
}

FString LeiaCameraBase::GetCameraNameFromGridIndex(const FLeiaCameraConstructionInfo& constructionInfo, int32 index) const
{
	//Sample output SceneCaptureCamera_[Row Num]x[Col Num],  Row Num is adjusted to have it so that 0th row is at top and nth row is at bottom
	return "SceneCaptureCamera_" + FString::FromInt(index);
}

FMatrix LeiaCameraBase::CalculateProjectionMatrix(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo, const FVector& localCamPos) const
{
	float aspectRatio = 0.0f;

	if (CurrentScreenOrientation == EScreenOrientation::LandscapeLeft || CurrentScreenOrientation == EScreenOrientation::LandscapeRight || CurrentScreenOrientation == EScreenOrientation::Unknown)
	{
		aspectRatio = static_cast<float>(constructionInfo.RenderTextureResolutionHeight) / constructionInfo.RenderTextureResolutionWidth;
	}
	else if (CurrentScreenOrientation == EScreenOrientation::Portrait || CurrentScreenOrientation == EScreenOrientation::PortraitUpsideDown)
	{
		aspectRatio = static_cast<float>(constructionInfo.RenderTextureResolutionWidth) / constructionInfo.RenderTextureResolutionHeight;
	}

	float wOffset;
	CalculateGridOffset(constructionInfo, wOffset);

	FMatrix projectionMatrix;

	if (renderingInfo.ProjectionType == ECameraProjectionMode::Type::Perspective)
	{
		projectionMatrix = FReversedZPerspectiveMatrix(FMath::DegreesToRadians(renderingInfo.FieldOfView * 0.5f), 1.0f, aspectRatio, renderingInfo.NearClipPlane, -renderingInfo.FarClipPlane);
	}
	else
	{
		const float orthoHeight = renderingInfo.OrthographicWidth * 0.5f;
		const float orthoWidth = orthoHeight * (1.0f / aspectRatio);
		const float farPlane = WORLD_MAX / 8.0f;

		const float zScale = 1.0f / (farPlane - renderingInfo.NearClipPlane);
		const float zOffset = -renderingInfo.NearClipPlane;

		projectionMatrix = FReversedZOrthoMatrix(orthoWidth, orthoHeight, zScale, zOffset);
	}

	const float gridPositionZ = 0.0f;

	/* We want to shear the frustum towards the center so that at the convergence distance all the frustums coincide with the (imaginary)
	center camera's symmetric frustum. In world units, the shear coefficient to achieve this is gridPositionY / ConvergenceDistance.
	Since the projection matrix is also scaling the yz-axes (using entries M[0][0] and M[1][1]), the shearing must also be scaled by the same amount. */

	const float shearY = projectionMatrix.M[0][0] * localCamPos.Y / renderingInfo.ConvergenceDistance;
	const float shearZ = projectionMatrix.M[1][1] * gridPositionZ / renderingInfo.ConvergenceDistance;

	projectionMatrix.M[2][0] = shearY;
	projectionMatrix.M[2][1] = shearZ;

	return projectionMatrix;
}

void LeiaCameraBase::SetProperPlaneOrientation(const FVector& forward, const FVector& right, FPlane& plane) const
{
	// Re-orient the plane's normal to match up with actor's world space rotation

	// In the plane's co-ordinates it's up direction is (0, 1, 0), right is (1, 0, 0) and forward is (0, 0, 1)
	// And actor up direction is (0, 0, 1), right is (0, 1, 0), forward is (1, 0, 0)

	FVector reorientedNormal = plane.RotateAngleAxis(90.0f, right).GetSafeNormal();
	reorientedNormal = reorientedNormal.RotateAngleAxis(90.0f, forward).GetSafeNormal();

	plane.X = reorientedNormal.X;
	plane.Y = reorientedNormal.Y;
	plane.Z = reorientedNormal.Z;
}

FString LeiaCameraBase::GetDeviceName() const
{
#if PLATFORM_ANDROID
	return FAndroidMisc::GetDeviceModel();
#else
	return "Other";
#endif
}

float LeiaCameraBase::GetNearPlaneDistance(const FLeiaCameraRenderingInfo& renderingInfo) const
{
	return (renderingInfo.Baseline * renderingInfo.ConvergenceDistance * 0.5f) / (renderingInfo.Baseline + 1.0f);
}

float LeiaCameraBase::GetFarPlaneDistance(const FLeiaCameraRenderingInfo& renderingInfo) const
{
	float farPlaneDistance = ((renderingInfo.Baseline + 1.0f) * renderingInfo.ConvergenceDistance * 2.0f) / renderingInfo.Baseline;
	const float MAX_FAR_PLANE = 8388608.0f;
	return FMath::Min(MAX_FAR_PLANE, farPlaneDistance);
}

// Utility Func
float LerpShear(float min, float max, float value, float minShear, float maxShear)
{
	value = FMath::Clamp(value, min, max);
	float step = (value - min) / (max - min);
	float shear = FMath::Lerp(minShear, maxShear, step);
	return shear;
}

float LeiaCameraBase::GetZdpShearScaler(const FLeiaCameraRenderingInfo& renderingInfo) const
{
	float baseline = renderingInfo.Baseline;
	float shearScaler = 0.0f;

	// use H1 values, and will scale at end if A1/Tablet
	if (baseline <= 0.25f)
	{
		shearScaler = LerpShear(0.0f, 0.25f, baseline, 500, 225);
	}
	else if (baseline <= 0.5f)
	{
		shearScaler = LerpShear(0.25f, 0.5f, baseline, 225, 150);
	}
	else if (baseline <= 1.0f)
	{
		shearScaler = LerpShear(0.5f, 1.0f, baseline, 150, 120);
	}
	else if (baseline <= 1.5f)
	{
		shearScaler = LerpShear(1.0f, 1.5f, baseline, 120, 105);
	}
	else if (baseline <= 2.0f)
	{
		shearScaler = LerpShear(1.5f, 2.0f, baseline, 105, 99);
	}
	else if (baseline <= 3.0f)
	{
		shearScaler = LerpShear(2.0f, 3.0f, baseline, 99, 95);
	}
	else
	{
		shearScaler = LerpShear(3.0f, 10.0f, baseline, 95, 85);
	}

	// scale for screen y height (1440 is reference above)
	// H1:  1440/1440 = 1.0
	// A1:  2160/1440 = 1.5
	// Tab: 1600/1440 = 1.11
	const float deviceScreenHeight = 1440.0f;
	shearScaler *= (Device->GetDisplayConfig().ToScreenOrientationConfig(CurrentScreenOrientation).panelResolution[1] / deviceScreenHeight);

	return shearScaler;
}

float LeiaCameraBase::GetInterviewDistanceUsingLeiaCamera(const FVector& camAlocalPos, const FVector& camBlocalPos) const
{
	const float panelRes = Device->GetDisplayConfig().ToScreenOrientationConfig(CurrentScreenOrientation).panelResolution[0];
	const float locDiff = FVector::Distance(camAlocalPos, camBlocalPos);
	return FMath::Abs(locDiff) / panelRes;
}

float LeiaCameraBase::GetSignedViewDistanceToCenter(const FVector& localPos) const
{
	return localPos.Y / Device->GetDisplayConfig().ToScreenOrientationConfig(CurrentScreenOrientation).panelResolution[0];
}

FVector LeiaCameraBase::GetPointOnCircle(float degrees, float distance, const FVector& center) const
{
	FVector circlePoint = center;
	circlePoint.Y += distance * FMath::Cos(degrees);
	circlePoint.Z += distance * FMath::Sin(degrees);

	return circlePoint;
}

float LeiaCameraBase::GetRaycastDistance(const FVector& raycastPosition, const FVector& raycastDir, UWorld* world, const FLeiaCameraZdpInfo& zdpInfo) const
{
	FHitResult result;
	TEnumAsByte<EObjectTypeQuery> ObjectToTrace = zdpInfo.ObjectTypeToQuery;
	TArray<TEnumAsByte<EObjectTypeQuery> > ObjectsToTraceAsByte;
	ObjectsToTraceAsByte.Add(ObjectToTrace);
	world->LineTraceSingleByObjectType(result, raycastPosition, raycastPosition + raycastDir * 50000.0f, FCollisionObjectQueryParams(ObjectsToTraceAsByte));

	return result.Distance;
}

bool LeiaCameraBase::SetConvergenceValueFromRaycast(const FVector& loc, const FVector& forwardDir, const FLeiaCameraZdpInfo& zdpInfo, FLeiaCameraRenderingInfo& renderingInfo, UWorld* world) const
{
	// center
	float centerDist = GetRaycastDistance(loc, forwardDir, world, zdpInfo);

	// around center to smooth
	float distanceAvg = 0.0f;
	int hitSamples = 0;
	int samplePoints = 12;
	for (int i = 0; i < samplePoints; i++)
	{
		float angle = 360.0f * (float)i / (float)samplePoints;
		FVector samplePoint = GetPointOnCircle(angle, 100.0f, loc);
		float hitDist = GetRaycastDistance(samplePoint, forwardDir, world, zdpInfo);
		if (hitDist > 0.0f)
		{
			distanceAvg += hitDist;
			hitSamples++;
		}
	}

	float sampleWeight = 0.25f;

	if (hitSamples > 0)
	{
		renderingInfo.ConvergenceDistance = zdpInfo.ConvergenceOffset + ((1.0f - sampleWeight) * centerDist) + (sampleWeight * (distanceAvg / (float)hitSamples));
		return true;
	}

	return false;
}

FVector LeiaCameraBase::GetCameraFrustumNearCenter(UWorld* world, const FTransform& targetCamera, const FMatrix& customProjectionMatrix, const FLeiaCameraRenderingInfo& renderingInfo, const FLeiaCameraZdpInfo& zdpInfo) const
{
	FPlane topPlane, btmPlane, lftPlane, rgtPlane, farPlane, nearPlane, convergencePlane;
	const FMatrix viewProjectionMatrix = targetCamera.ToInverseMatrixWithScale() * customProjectionMatrix;

	const FVector camPos = targetCamera.GetLocation();
	const FVector forwardVec = targetCamera.GetUnitAxis(EAxis::X);
	const FVector rightVec = targetCamera.GetUnitAxis(EAxis::Y);
	const FVector upVec = targetCamera.GetUnitAxis(EAxis::Z);

	const bool isOrthoMode = renderingInfo.ProjectionType == ECameraProjectionMode::Orthographic;

	float aspectRatio = 0.0f;
	const FDisplayConfig config = Device->GetDisplayConfig();

	if (CurrentScreenOrientation == EScreenOrientation::LandscapeLeft || CurrentScreenOrientation == EScreenOrientation::LandscapeRight || CurrentScreenOrientation == EScreenOrientation::Unknown)
	{
		aspectRatio = static_cast<float>(config.viewResolution[1]) / config.viewResolution[0];
	}
	else if (CurrentScreenOrientation == EScreenOrientation::Portrait || CurrentScreenOrientation == EScreenOrientation::PortraitUpsideDown)
	{
		aspectRatio = static_cast<float>(config.viewResolution[0]) / config.viewResolution[1];
	}

	const float height = (isOrthoMode) ? renderingInfo.OrthographicWidth * 0.5f : 1.0f;
	const float width = (isOrthoMode) ? height * (1.0f / aspectRatio) : 1.0f;

	viewProjectionMatrix.GetFrustumTopPlane(topPlane);
	viewProjectionMatrix.GetFrustumBottomPlane(btmPlane);
	viewProjectionMatrix.GetFrustumLeftPlane(lftPlane);
	viewProjectionMatrix.GetFrustumRightPlane(rgtPlane);
	viewProjectionMatrix.GetFrustumFarPlane(farPlane);

	// Need offset along normal away from origin
	topPlane.W = height;
	btmPlane.W = height;
	lftPlane.W = width;
	rgtPlane.W = width;

	SetProperPlaneOrientation(forwardVec, rightVec, topPlane);
	SetProperPlaneOrientation(forwardVec, rightVec, btmPlane);
	SetProperPlaneOrientation(forwardVec, rightVec, lftPlane);
	SetProperPlaneOrientation(forwardVec, rightVec, rgtPlane);

	if (isOrthoMode)
	{
		farPlane = FPlane(forwardVec, renderingInfo.ConvergenceDistance + renderingInfo.FarClipPlane);
	}
	else
	{
		farPlane.W = (renderingInfo.FarClipPlane > 0.0f) ? -renderingInfo.FarClipPlane : renderingInfo.FarClipPlane;
	}

	nearPlane = farPlane;
	nearPlane.W = ((isOrthoMode) ? 1 : -1) * renderingInfo.NearClipPlane;

	convergencePlane = farPlane;
	convergencePlane.W = ((isOrthoMode) ? 1 : -1) * renderingInfo.ConvergenceDistance;

	if (!isOrthoMode)
	{
		SetProperPlaneOrientation(forwardVec, rightVec, farPlane);
		SetProperPlaneOrientation(forwardVec, rightVec, nearPlane);
		SetProperPlaneOrientation(forwardVec, rightVec, convergencePlane);
	}

	FVector FLTintersectAtConvergence, FRTintersectAtConvergence, FLBintersectAtConvergence, FRBintersectAtConvergence;

	if (!FMath::IntersectPlanes3(FLTintersectAtConvergence, convergencePlane, lftPlane, topPlane)) { return FLTintersectAtConvergence; }
	if (!FMath::IntersectPlanes3(FRTintersectAtConvergence, convergencePlane, rgtPlane, topPlane)) { return FRTintersectAtConvergence; }
	if (!FMath::IntersectPlanes3(FLBintersectAtConvergence, convergencePlane, lftPlane, btmPlane)) { return FLBintersectAtConvergence; }
	if (!FMath::IntersectPlanes3(FRBintersectAtConvergence, convergencePlane, rgtPlane, btmPlane)) { return FRBintersectAtConvergence; }

	FVector sourceVector = (isOrthoMode) ? forwardVec : ((FLBintersectAtConvergence + FRBintersectAtConvergence) * 0.5f);
	const FVector traceOrigin = (isOrthoMode) ? camPos - (upVec * height) : camPos;
	sourceVector.Normalize();

	FHitResult result;
	TEnumAsByte<EObjectTypeQuery> ObjectToTrace = zdpInfo.ObjectTypeToQuery;
	TArray<TEnumAsByte<EObjectTypeQuery> > ObjectsToTraceAsByte;
	ObjectsToTraceAsByte.Add(ObjectToTrace);
	world->LineTraceSingleByObjectType(result, traceOrigin, traceOrigin + sourceVector * 50000.0f, FCollisionObjectQueryParams(ObjectsToTraceAsByte));

	return result.ImpactPoint;
}

FVector LeiaCameraBase::WorldToScreenPoint(const FVector& worldPos, USceneCaptureComponent2D* camera) const
{
	FMatrix ViewRotationMatrix = FInverseRotationMatrix(camera->GetComponentRotation()) * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));

	FMatrix viewMat = FTranslationMatrix(-camera->GetComponentLocation()) * ViewRotationMatrix;
	FMatrix viewProjMat = viewMat * camera->CustomProjectionMatrix;
	FVector4 outPos = viewProjMat.TransformPosition(worldPos);
	outPos.X = ((outPos.X / outPos.W) + 1.0f) * 0.5f * Device->GetDisplayConfig().ToScreenOrientationConfig(CurrentScreenOrientation).viewResolution[0];
	outPos.Y = ((outPos.Y / outPos.W) + 1.0f) * 0.5f * Device->GetDisplayConfig().ToScreenOrientationConfig(CurrentScreenOrientation).viewResolution[1];

	return FVector(outPos.X, outPos.Y, 0.0f);
}

float LeiaCameraBase::CalculateAutoZdpShearValue(const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraRenderingInfo& renderingInfo,
	const FLeiaCameraZdpInfo& zdpInfo, const FTransform& targetCamTransform, float interviewDistance,
	USceneCaptureComponent2D* camA, USceneCaptureComponent2D* camB) const
{

	float aspectRatio = 0.0f;
	if (CurrentScreenOrientation == EScreenOrientation::LandscapeLeft || CurrentScreenOrientation == EScreenOrientation::LandscapeRight || CurrentScreenOrientation == EScreenOrientation::Unknown)
	{
		aspectRatio = static_cast<float>(constructionInfo.RenderTextureResolutionHeight) / constructionInfo.RenderTextureResolutionWidth;
	}
	else if (CurrentScreenOrientation == EScreenOrientation::Portrait || CurrentScreenOrientation == EScreenOrientation::PortraitUpsideDown)
	{
		aspectRatio = static_cast<float>(constructionInfo.RenderTextureResolutionWidth) / constructionInfo.RenderTextureResolutionHeight;
	}

	FMatrix projectionMatrix;

	if (renderingInfo.ProjectionType == ECameraProjectionMode::Type::Perspective)
	{
		projectionMatrix = FReversedZPerspectiveMatrix(FMath::DegreesToRadians(renderingInfo.FieldOfView * 0.5f), 1.0f, aspectRatio, renderingInfo.NearClipPlane, -renderingInfo.FarClipPlane);
	}
	else
	{
		const float orthoHeight = renderingInfo.OrthographicWidth * 0.5f;
		const float orthoWidth = orthoHeight * (1.0f / aspectRatio);
		const float farPlane = WORLD_MAX / 8.0f;

		const float zScale = 1.0f / (farPlane - renderingInfo.NearClipPlane);
		const float zOffset = -renderingInfo.NearClipPlane;

		projectionMatrix = FReversedZOrthoMatrix(orthoWidth, orthoHeight, zScale, zOffset);
	}

	const FVector nearCenterVector = GetCameraFrustumNearCenter(camA->GetWorld(), targetCamTransform, projectionMatrix, renderingInfo, zdpInfo);
	const FVector screenpointA = WorldToScreenPoint(nearCenterVector, camA);
	const FVector screenpointB = WorldToScreenPoint(nearCenterVector, camB);

	const float shearOffset = FVector::Distance(screenpointA, screenpointB) / Device->GetDisplayConfig().ToScreenOrientationConfig(CurrentScreenOrientation).viewResolution[0]; // now in screen 0..1
	const float shearStrength = -(shearOffset / (0.5f * interviewDistance));

	return shearStrength;
}

float LeiaCameraBase::UpdateViews(int index, const FLeiaCameraRenderingInfo& renderingInfo, const FLeiaCameraConstructionInfo& constructionInfo, bool emissionRescalingEnabled/* = true*/) const
{
	CameraCalculatedParams calculated = CameraCalculatedParams(renderingInfo, Device->GetDisplayConfig().ToScreenOrientationConfig(CurrentScreenOrientation), emissionRescalingEnabled);
	float wOffset;
	CalculateGridOffset(constructionInfo, wOffset);
	float posx = calculated.EmissionRescalingFactor * (renderingInfo.Baseline * (index - wOffset));
	return posx;
}

void LeiaCameraBase::SetCommonZdpMaterialParams(const FLeiaCameraRenderingInfo& renderingInfo, const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraZdpInfo& zdpInfo, USceneCaptureComponent2D* camA, USceneCaptureComponent2D* camB, const FTransform& targetCamTransform)
{
	CommonMatParamCollectionInst->SetScalarParameterValue("ConvergenceDistance", renderingInfo.ConvergenceDistance);
	CommonMatParamCollectionInst->SetScalarParameterValue("FarPlaneDistance", GetFarPlaneDistance(renderingInfo));
	CommonMatParamCollectionInst->SetScalarParameterValue("NearPlaneDistance", GetNearPlaneDistance(renderingInfo));
	CommonMatParamCollectionInst->SetScalarParameterValue("ShearScaler", GetZdpShearScaler(renderingInfo));
	CommonMatParamCollectionInst->SetScalarParameterValue("ShearLineY", 0.5f);

	const float interviewDistance = GetInterviewDistanceUsingLeiaCamera(camA->GetComponentLocation(), camB->GetComponentLocation());
	CommonMatParamCollectionInst->SetScalarParameterValue("InterviewDistance", interviewDistance);

	CommonMatParamCollectionInst->SetScalarParameterValue("ShearPixels", -zdpInfo.ZDPShear * 0.01f);
}

void LeiaCameraBase::SetInterlaceMaterialParams(EScreenOrientation::Type orientation)
{
	FDisplayConfig config = Device->GetDisplayConfig();
	
	InterlaceMatParamCollectionInst->SetScalarParameterValue("viewsX", static_cast<float>(config.numViews[0]));
	InterlaceMatParamCollectionInst->SetScalarParameterValue("viewsY", static_cast<float>(config.numViews[1]));
	InterlaceMatParamCollectionInst->SetVectorParameterValue("viewRes",
	FLinearColor(static_cast<float>(config.viewResolution[0]),
	static_cast<float>(config.viewResolution[1]), 0.0f, 0.0f));	
	
	

	FDisplayConfig::OrientationConfig orientationConfig = Device->GetDisplayConfig().ToScreenOrientationConfig(orientation);

	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceVector", FLinearColor(orientationConfig.interlacingVector));
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceMatA", FLinearColor(orientationConfig.interlacingMatrix.M[0][0], orientationConfig.interlacingMatrix.M[0][1], orientationConfig.interlacingMatrix.M[0][2], orientationConfig.interlacingMatrix.M[0][3]));
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceMatB", FLinearColor(orientationConfig.interlacingMatrix.M[1][0], orientationConfig.interlacingMatrix.M[1][1], orientationConfig.interlacingMatrix.M[1][2], orientationConfig.interlacingMatrix.M[1][3]));
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceMatC", FLinearColor(orientationConfig.interlacingMatrix.M[2][0], orientationConfig.interlacingMatrix.M[2][1], orientationConfig.interlacingMatrix.M[2][2], orientationConfig.interlacingMatrix.M[2][3]));
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceMatD", FLinearColor(static_cast<int>(orientationConfig.interlacingMatrix.M[3][0]), orientationConfig.interlacingMatrix.M[3][1], orientationConfig.interlacingMatrix.M[3][2], orientationConfig.interlacingMatrix.M[3][3]));

}

void LeiaCameraBase::SetViewSharpeningMaterialParams()
{

	FDisplayConfig config = Device->GetDisplayConfig();

	ViewSharpeningMatParamCollectionInst->SetScalarParameterValue("gamma", config.gamma);
	ViewSharpeningMatParamCollectionInst->SetScalarParameterValue("sharpeningCenter", 1.0f);
	ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningSize", FLinearColor(config.sharpeningKernelX[0], config.sharpeningKernelY[0], 0, 0));
	ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningX",
		FLinearColor(config.sharpeningKernelX[1], config.sharpeningKernelX[2],
			config.sharpeningKernelX[3], config.sharpeningKernelX[4]));
	ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningY",
		FLinearColor(config.sharpeningKernelY[1], config.sharpeningKernelY[2],
			config.sharpeningKernelY[3], config.sharpeningKernelY[4]));
/*	const FDisplayConfig displayConfig = Device->GetDisplayConfig();

	SharpenParams sharpenShaderParams;
	GetViewSharpeningParams(displayConfig, sharpenShaderParams);

	const bool is4V = (displayConfig.numViews[0] == 4) && (displayConfig.numViews[1] == 4);
	if (is4V)
	{
		// Apply 4V
		ViewSharpeningMatParamCollectionInst->SetScalarParameterValue("gamma", sharpenShaderParams.gamma);
		ViewSharpeningMatParamCollectionInst->SetScalarParameterValue("sharpeningCenter", sharpenShaderParams.sharpeningCenter);
		ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningSize", sharpenShaderParams.sharpeningSize);
		ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningX", sharpenShaderParams.sharpeningX);
		ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningY", sharpenShaderParams.sharpeningY);
		ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("textureInvSize", sharpenShaderParams.textureInvSize);
	}
	else
	{
		// TODO: Add non-4V sharpening here.
	}*/
}
