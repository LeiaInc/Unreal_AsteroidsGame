// Fill out your copyright notice in the Description page of Project Settings.

#include "LeiaCameraBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Components/LineBatchComponent.h"
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
#elif PLATFORM_WINDOWS && !WITH_EDITOR
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

		FVector FLTintersectAtFarPlane, FRTintersectAtFarPlane, FLBintersectAtFarPlane, FRBintersectAtFarPlane;

		FMath::IntersectPlanes3(FLTintersectAtFarPlane, farPlane, lftPlane, topPlane);
		FMath::IntersectPlanes3(FRTintersectAtFarPlane, farPlane, rgtPlane, topPlane);
		FMath::IntersectPlanes3(FLBintersectAtFarPlane, farPlane, lftPlane, btmPlane);
		FMath::IntersectPlanes3(FRBintersectAtFarPlane, farPlane, rgtPlane, btmPlane);

		FVector FLTintersectAtNearPlane, FRTintersectAtNearPlane, FLBintersectAtNearPlane, FRBintersectAtNearPlane;

		FMath::IntersectPlanes3(FLTintersectAtNearPlane, nearPlane, lftPlane, topPlane);
		FMath::IntersectPlanes3(FRTintersectAtNearPlane, nearPlane, rgtPlane, topPlane);
		FMath::IntersectPlanes3(FLBintersectAtNearPlane, nearPlane, lftPlane, btmPlane);
		FMath::IntersectPlanes3(FRBintersectAtNearPlane, nearPlane, rgtPlane, btmPlane);

		FVector FLTintersectAtConvergence, FRTintersectAtConvergence, FLBintersectAtConvergence, FRBintersectAtConvergence;

		FMath::IntersectPlanes3(FLTintersectAtConvergence, convergencePlane, lftPlane, topPlane);
		FMath::IntersectPlanes3(FRTintersectAtConvergence, convergencePlane, rgtPlane, topPlane);
		FMath::IntersectPlanes3(FLBintersectAtConvergence, convergencePlane, lftPlane, btmPlane);
		FMath::IntersectPlanes3(FRBintersectAtConvergence, convergencePlane, rgtPlane, btmPlane);

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
		finalBatchedLines.Push({ FLTintersectAtFarPlane, FRTintersectAtFarPlane, FLinearColor::Green, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FRTintersectAtFarPlane, FRBintersectAtFarPlane, FLinearColor::Green, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FRBintersectAtFarPlane, FLBintersectAtFarPlane, FLinearColor::Green, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FLBintersectAtFarPlane, FLTintersectAtFarPlane, FLinearColor::Green, 2.0f, -1.0f, 77 });

		finalBatchedLines.Push({ FLTintersectAtFarPlane, (isOrthoMode) ? FLTintersectAtNearPlane : cameraLocation, FLinearColor::Green, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FRTintersectAtFarPlane, (isOrthoMode) ? FRTintersectAtNearPlane : cameraLocation, FLinearColor::Green, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FRBintersectAtFarPlane, (isOrthoMode) ? FRBintersectAtNearPlane : cameraLocation, FLinearColor::Green, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FLBintersectAtFarPlane, (isOrthoMode) ? FLBintersectAtNearPlane : cameraLocation, FLinearColor::Green, 2.0f, -1.0f, 77 });

		finalBatchedLines.Push({ FLTintersectAtNearPlane, FRTintersectAtNearPlane, FLinearColor::Yellow, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FRTintersectAtNearPlane, FRBintersectAtNearPlane, FLinearColor::Yellow, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FLBintersectAtNearPlane, FLTintersectAtNearPlane, FLinearColor::Yellow, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FRBintersectAtNearPlane, FLBintersectAtNearPlane, FLinearColor::Yellow, 2.0f, -1.0f, 77 });

		finalBatchedLines.Push({ FLTintersectAtConvergence, FRTintersectAtConvergence, FLinearColor::Yellow, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FRTintersectAtConvergence, FRBintersectAtConvergence, FLinearColor::Yellow, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FLBintersectAtConvergence, FLTintersectAtConvergence, FLinearColor::Yellow, 2.0f, -1.0f, 77 });
		finalBatchedLines.Push({ FRBintersectAtConvergence, FLBintersectAtConvergence, FLinearColor::Yellow, 2.0f, -1.0f, 77 });
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
	return (renderingInfo.Baseline * renderingInfo.ConvergenceDistance) / (renderingInfo.Baseline + 1.0f);
}

float LeiaCameraBase::GetFarPlaneDistance(const FLeiaCameraRenderingInfo& renderingInfo) const
{
	float farPlaneDistance = ((renderingInfo.Baseline + 1.0f) * renderingInfo.ConvergenceDistance) / renderingInfo.Baseline;
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

	FMath::IntersectPlanes3(FLTintersectAtConvergence, convergencePlane, lftPlane, topPlane);
	FMath::IntersectPlanes3(FRTintersectAtConvergence, convergencePlane, rgtPlane, topPlane);
	FMath::IntersectPlanes3(FLBintersectAtConvergence, convergencePlane, lftPlane, btmPlane);
	FMath::IntersectPlanes3(FRBintersectAtConvergence, convergencePlane, rgtPlane, btmPlane);

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

FVector LeiaCameraBase::WorldToScreenPoint(const FVector& worldPos, CaptureComponent* camera) const
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
	CaptureComponent* camA , CaptureComponent* camB) const
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

void LeiaCameraBase::SetCommonZdpMaterialParams(const FLeiaCameraRenderingInfo& renderingInfo, const FLeiaCameraConstructionInfo& constructionInfo, const FLeiaCameraZdpInfo& zdpInfo, CaptureComponent* camA, CaptureComponent* camB, const FTransform& targetCamTransform)
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
	InterlaceParams interlaceShaderParams;
	GetInterlaceParams(orientation, interlaceShaderParams);

	InterlaceMatParamCollectionInst->SetScalarParameterValue("viewsX",          interlaceShaderParams.viewsX);
	InterlaceMatParamCollectionInst->SetScalarParameterValue("viewsY",          interlaceShaderParams.viewsY);
	InterlaceMatParamCollectionInst->SetVectorParameterValue("viewRes",         interlaceShaderParams.viewRes);
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceVector", interlaceShaderParams.intVecs);
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceMatA",   interlaceShaderParams.matA);
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceMatB",   interlaceShaderParams.matB);
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceMatC",   interlaceShaderParams.matC);
	InterlaceMatParamCollectionInst->SetVectorParameterValue("interlaceMatD",   interlaceShaderParams.matD);
}

void LeiaCameraBase::GetInterlaceParams(EScreenOrientation::Type orientation, InterlaceParams& interlaceParams)
{
	const FDisplayConfig config = Device->GetDisplayConfig();
	const FDisplayConfig::OrientationConfig orientationConfig = Device->GetDisplayConfig().ToScreenOrientationConfig(orientation);

	interlaceParams.viewsX  = static_cast<float>(config.numViews[0]);
	interlaceParams.viewsY  = static_cast<float>(config.numViews[1]);
	interlaceParams.viewRes = FLinearColor(static_cast<float>(config.viewResolution[0]), static_cast<float>(config.viewResolution[1]), 0.0f, 0.0f);
	interlaceParams.intVecs = FLinearColor(orientationConfig.interlacingVector);
	interlaceParams.matA    = FLinearColor(orientationConfig.interlacingMatrix.M[0][0], orientationConfig.interlacingMatrix.M[0][1], orientationConfig.interlacingMatrix.M[0][2], orientationConfig.interlacingMatrix.M[0][3]);
	interlaceParams.matB    = FLinearColor(orientationConfig.interlacingMatrix.M[1][0], orientationConfig.interlacingMatrix.M[1][1], orientationConfig.interlacingMatrix.M[1][2], orientationConfig.interlacingMatrix.M[1][3]);
	interlaceParams.matC    = FLinearColor(orientationConfig.interlacingMatrix.M[2][0], orientationConfig.interlacingMatrix.M[2][1], orientationConfig.interlacingMatrix.M[2][2], orientationConfig.interlacingMatrix.M[2][3]);
	interlaceParams.matD    = FLinearColor(static_cast<int>(orientationConfig.interlacingMatrix.M[3][0]), orientationConfig.interlacingMatrix.M[3][1], orientationConfig.interlacingMatrix.M[3][2], orientationConfig.interlacingMatrix.M[3][3]);
}

void LeiaCameraBase::GetViewSharpeningParams(const FDisplayConfig& config, SharpenParams& sharpenShaderParams)
{
	sharpenShaderParams.gamma            = config.gamma;
	sharpenShaderParams.sharpeningCenter = 1.0f;
	sharpenShaderParams.sharpeningSize   = FLinearColor(config.sharpeningKernelX[0], config.sharpeningKernelY[0], 0, 0);
	sharpenShaderParams.sharpeningX      = FLinearColor(config.sharpeningKernelX[1], config.sharpeningKernelX[2], config.sharpeningKernelX[3], config.sharpeningKernelX[4]);
	sharpenShaderParams.sharpeningY      = FLinearColor(config.sharpeningKernelY[1], config.sharpeningKernelY[2], config.sharpeningKernelY[3], config.sharpeningKernelY[4]);
	sharpenShaderParams.textureInvSize   = FLinearColor(1.0f / config.panelResolution[0], 1.0f / config.panelResolution[1], 0, 0);
}

void LeiaCameraBase::SetViewSharpeningMaterialParams()
{
	SharpenParams sharpenShaderParams;
	GetViewSharpeningParams(Device->GetDisplayConfig(), sharpenShaderParams);

	ViewSharpeningMatParamCollectionInst->SetScalarParameterValue("gamma",            sharpenShaderParams.gamma);
	ViewSharpeningMatParamCollectionInst->SetScalarParameterValue("sharpeningCenter", sharpenShaderParams.sharpeningCenter);
	ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningSize",   sharpenShaderParams.sharpeningSize);
	ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningX",      sharpenShaderParams.sharpeningX);
	ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("sharpeningY",      sharpenShaderParams.sharpeningY);
	ViewSharpeningMatParamCollectionInst->SetVectorParameterValue("textureInvSize",   sharpenShaderParams.textureInvSize);
}
