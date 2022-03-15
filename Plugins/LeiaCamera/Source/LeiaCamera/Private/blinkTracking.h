#pragma once

#ifdef BLINKTRACKINGWRAPPER_EXPORTS
#define TRACKING_API __declspec(dllexport)
#else
#define TRACKING_API __declspec(dllimport)
#endif

#include <vector>

struct LeiaEyeFeature {
	float eyeX, eyeY, eyeZ;
	int frameX, frameY;
	float viewNum;

};

struct LeiaFaceFeature {
	// bounding box of the face detected
	int faceBBLeft, faceBBRight, faceBBTop, faceBBBottom;
	// bounding box of the face detected
	int faceX, faceY;
	int faceWidth, faceHeight;

	// world dimension of the face
	float faceWorldX, faceWorldY, faceWorldZ;
	float faceWorldWidth, faceWorldHeight;

	// confidential value of each face.
	float faceConfidence;

	// left and right eye property
	LeiaEyeFeature leftEye, rightEye;

};
struct LeiaDisplayParameter {
	int viewCountX;							// horizontal view count for display
	int viewCountY;							// vertical view count for display
	double centerViewNumX;					// center view number X
	double centerViewNumY;					// center view number Y
	double convergeDimX;						// x-dimension of eyeboxes in converge plane (unit:mm)
	double convergeDimY;						// y-dimension of eyeboxes in converge plane (unit:mm)
	double convergeDist;						// distance of convergence plane from display plane (unit:mm)
	double viewSlant;						// radient of view slant 
	int displayOption;		// display option for view number query
};

// exposed apis
#ifdef __cplusplus
extern "C" {
#endif

	TRACKING_API void blink_initialize(bool use_rgb);

	TRACKING_API void blink_initialize_config(char input_path[]);

	TRACKING_API void blink_terminate();

	TRACKING_API void blink_startTracking();

	TRACKING_API void blink_stopTracking();

	TRACKING_API void blink_cam_setCameraPos(int x, int y, int z);

	TRACKING_API void blink_cam_setAutoExposure(bool auto_exposure);

	TRACKING_API void blink_cam_setCameraExposure(float exposure);

	TRACKING_API void blink_cam_setCameraGain(float gain);

	TRACKING_API void blink_cam_setEmitterEnable(bool emitter_enable);

	//removing openCV compiler dependencies
	//TRACKING_API cv::Mat blink_debug_getFrame();

	TRACKING_API int blink_getTrackingResult(LeiaFaceFeature** face, LeiaDisplayParameter dispParam);

	TRACKING_API int blink_getTrackingResultVector(std::vector<LeiaFaceFeature>& trackingResult, LeiaDisplayParameter dispParam);

	TRACKING_API int blink_getViewShiftMat(float* viewShift, int buffersize);

	TRACKING_API int blink_getViewShiftMat_new(float* viewshift, LeiaFaceFeature face, LeiaDisplayParameter dispParam,
		int dimx, int dimy);

#ifdef __cplusplus
}
#endif