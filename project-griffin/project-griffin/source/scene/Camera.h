#pragma once
#ifndef GRIFFIN_CAMERA_H_
#define GRIFFIN_CAMERA_H_

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>


using glm::vec2;
using glm::vec3;
using glm::dvec3;
using glm::mat4;
using glm::dmat4;
using glm::quat;

namespace griffin {
	namespace scene {

		class Camera {
		public:
			Camera() : mModelViewCached(false), mProjectionCached(false), mInverseModelViewCached(false), mWorldUp(0.0f, 1.0f, 0.0f) {}
			virtual ~Camera() {}

			dvec3	getEyePoint() const { return mEyePoint; }
			void	setEyePoint(const dvec3 &aEyePoint);

			double	getCenterOfInterest() const { return mCenterOfInterest; }
			void	setCenterOfInterest(double aCenterOfInterest) { mCenterOfInterest = aCenterOfInterest; }

			dvec3	getCenterOfInterestPoint() const { return mEyePoint + dvec3(mViewDirection) * mCenterOfInterest; }
			void	setCenterOfInterestPoint(const dvec3 &centerOfInterestPoint);

			vec3	getWorldUp() const { return mWorldUp; }
			void	setWorldUp(const vec3 &aWorldUp);

			void	lookAt(const dvec3 &target);
			void	lookAt(const dvec3 &aEyePoint, const dvec3 &target);
			void	lookAt(const dvec3 &aEyePoint, const dvec3 &target, const vec3 &aUp);
			vec3	getViewDirection() const { return mViewDirection; }
			void	setViewDirection(const vec3 &aViewDirection);

			quat	getOrientation() const { return mOrientation; }
			void	setOrientation(const quat &aOrientation);

			float	getFov() const { return mFov; }
			void	setFov(float aFov) { mFov = aFov;  mProjectionCached = false; }
			float	getFovHorizontal() const { return glm::degrees(2.0f * glm::atan(glm::tan(glm::radians(mFov) * 0.5f) * mAspectRatio)); }
			void	setFovHorizontal(float aFov) { mFov = glm::degrees(2.0f * glm::atan(glm::tan(glm::radians(aFov) * 0.5f) / mAspectRatio));  mProjectionCached = false; }

			float	getAspectRatio() const { return mAspectRatio; }
			void	setAspectRatio(float aAspectRatio) { mAspectRatio = aAspectRatio; mProjectionCached = false; }
			float	getNearClip() const { return mNearClip; }
			void	setNearClip(float aNearClip) { mNearClip = aNearClip; mProjectionCached = false; }
			float	getFarClip() const { return mFarClip; }
			void	setFarClip(float aFarClip) { mFarClip = aFarClip; mProjectionCached = false; }

			virtual void	getNearClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight);
			virtual void	getFarClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight);

			//! Returns the coordinates of the camera's frustum, suitable for passing to \c glFrustum
			void	getFrustum(float *left, float *top, float *right, float *bottom, float *near, float *far);
			//! Returns whether the camera represents a perspective projection instead of an orthographic
			virtual bool isPersp() const = 0;

			virtual const mat4&		getProjectionMatrix() const { assert(mProjectionCached); return mProjectionMatrix; }
			virtual const dmat4&	getModelViewMatrix() const { assert(mModelViewCached); return mModelViewMatrix; }
			virtual const dmat4&	getInverseModelViewMatrix() const { assert(mInverseModelViewCached); return mInverseModelViewMatrix; }

			//Ray		generateRay(float u, float v, float imagePlaneAspectRatio) const;
			void	getBillboardVectors(dvec3 *right, dvec3 *up) const;

			//! Converts a world-space coordinate \a worldCoord to screen coordinates as viewed by the camera, based ona s screen which is \a screenWidth x \a screenHeight pixels.
			vec2	worldToScreen(const dvec3 &worldCoord, float screenWidth, float screenHeight) const;
			//! Converts a world-space coordinate \a worldCoord to eye-space, also known as camera-space. -Z is along the view direction.
			vec3	worldToEye(const dvec3 &worldCoord);
			//! Converts a world-space coordinate \a worldCoord to the z axis of eye-space, also known as camera-space. -Z is along the view direction. Suitable for depth sorting.
			float	worldToEyeDepth(const dvec3 &worldCoord) const;
			//! Converts a world-space coordinate \a worldCoord to normalized device coordinates
			vec3	worldToNdc(const dvec3 &worldCoord);

			//float	getScreenRadius(const class Sphere &sphere, float screenWidth, float screenHeight) const;

			inline void		calcMatrices() {
				if (!mModelViewCached) { calcModelView(); }
				if (!mProjectionCached) { calcProjection(); }
				// note: calculation of the inverse modelview matrices is postponed until actually requested
				//if (!mInverseModelViewCached) { calcInverseModelView(); }
			}

			virtual void	calcModelView();
			virtual void	calcInverseModelView();
			virtual void	calcProjection() = 0;

			const vec3&	getRightVector() const { return mU; }
			const vec3&	getUpVector() const	{ return mV; }

		protected:
			dvec3	mEyePoint;
			vec3	mViewDirection;
			quat	mOrientation;
			double	mCenterOfInterest;
			vec3	mWorldUp;

			float	mFov;
			float	mAspectRatio;
			float	mNearClip;
			float	mFarClip;

			vec3	mU;	// Right vector
			vec3	mV;	// Adjust up-vector
			vec3	mW;	// Negative view direction

			mat4	mProjectionMatrix, mInverseProjectionMatrix;
			bool	mProjectionCached;
			dmat4	mModelViewMatrix;
			bool	mModelViewCached;
			dmat4	mInverseModelViewMatrix;
			bool	mInverseModelViewCached;

			float	mFrustumLeft, mFrustumRight, mFrustumTop, mFrustumBottom;
		};


		class CameraPersp : public Camera {
		public:
			CameraPersp();
			CameraPersp(int pixelWidth, int pixelHeight, float fov); // constructs screen-aligned camera
			CameraPersp(int pixelWidth, int pixelHeight, float fov, float nearPlane, float farPlane); // constructs screen-aligned camera

			void	setPerspective(float verticalFovDegrees, float aspectRatio, float nearPlane, float farPlane);

			/** Returns both the horizontal and vertical lens shift.
			A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
			A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
			void	getLensShift(float *horizontal, float *vertical) const { *horizontal = mLensShift.x; *vertical = mLensShift.y; }
			/** Returns both the horizontal and vertical lens shift.
			A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
			A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
			vec2	getLensShift() const { return vec2(mLensShift.x, mLensShift.y); }
			/** Sets both the horizontal and vertical lens shift.
			A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
			A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
			void	setLensShift(float horizontal, float vertical);
			/** Sets both the horizontal and vertical lens shift.
			A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
			A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
			void	setLensShift(const vec2 &shift) { setLensShift(shift.x, shift.y); }
			//! Returns the horizontal lens shift. A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
			float	getLensShiftHorizontal() const { return mLensShift.x; }
			/** Sets the horizontal lens shift.
			A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport. */
			void	setLensShiftHorizontal(float horizontal) { setLensShift(horizontal, mLensShift.y); }
			//! Returns the vertical lens shift. A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport.
			float	getLensShiftVertical() const { return mLensShift.y; }
			/** Sets the vertical lens shift.
			A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
			void	setLensShiftVertical(float vertical) { setLensShift(mLensShift.x, vertical); }

			virtual bool	isPersp() const { return true; }

			//CameraPersp	getFrameSphere(const class Sphere &worldSpaceSphere, int maxIterations = 20) const;

			virtual void	calcProjection();

		protected:
			vec2	mLensShift;
		};


		class CameraPersp2 : public CameraPersp {
		public:
			CameraPersp2(int pixelWidth, int pixelHeight, float fov, float nearPlane, float farPlane) :
				CameraPersp(pixelWidth, pixelHeight, fov, nearPlane, farPlane)
			{}

			const mat4&	getViewProjection() const;
			vec3		getEulerAngles() const;

			void	setTranslationYawPitchRoll(const dvec3& position, double yaw, double pitch, double roll);
			void	lookAt(const dvec3 &position, const dvec3 &target, const vec3 &worldUp);
			void	extractFrustumPlanes();

		protected:
			quat	mViewRotationQuat;
			mat4	mViewMatrix;
			mat4	mViewProjectionMatrix;
		};


		class CameraOrtho : public Camera {
		public:
			CameraOrtho();
			CameraOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane);

			void setOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane);

			virtual bool	isPersp() const { return false; }

		protected:
			virtual void	calcProjection();
		};


		class CameraStereo : public CameraPersp {
		public:
			CameraStereo() :
				mConvergence(1.0f), mEyeSeparation(0.05f), mIsStereo(false), mIsLeft(true)
			{}
			CameraStereo(int pixelWidth, int pixelHeight, float fov) :
				CameraPersp(pixelWidth, pixelHeight, fov),
				mConvergence(1.0f), mEyeSeparation(0.05f), mIsStereo(false), mIsLeft(true) // constructs screen-aligned camera
			{}
			CameraStereo(int pixelWidth, int pixelHeight, float fov, float nearPlane, float farPlane) :
				CameraPersp(pixelWidth, pixelHeight, fov, nearPlane, farPlane),
				mConvergence(1.0f), mEyeSeparation(0.05f), mIsStereo(false), mIsLeft(true) // constructs screen-aligned camera
			{}

			//! Returns the current convergence, which is the distance at which there is no parallax.
			float			getConvergence() const { return mConvergence; }
			//! Sets the convergence of the camera, which is the distance at which there is no parallax.
			void			setConvergence(float distance, bool adjustEyeSeparation = false) {
								mConvergence = distance; mProjectionCached = false;
								if (adjustEyeSeparation) { mEyeSeparation = mConvergence / 30.0f; }
							}
			//! Returns the distance between the camera's for the left and right eyes.
			float			getEyeSeparation() const { return mEyeSeparation; }
			//! Sets the distance between the camera's for the left and right eyes. This affects the parallax effect. 
			void			setEyeSeparation(float distance) { mEyeSeparation = distance; mModelViewCached = false; mProjectionCached = false; }
			//! Returns the location of the currently enabled eye camera.
			dvec3			getEyePointShifted() const;

			//! Enables the left eye camera.
			void			enableStereoLeft() { mIsStereo = true; mIsLeft = true; }
			bool			isStereoLeftEnabled() const { return mIsStereo && mIsLeft; }
			//! Enables the right eye camera.
			void			enableStereoRight() { mIsStereo = true; mIsLeft = false; }
			bool			isStereoRightEnabled() const { return mIsStereo && !mIsLeft; }
			//! Disables stereoscopic rendering, converting the camera to a standard CameraPersp.
			void			disableStereo() { mIsStereo = false; }
			bool			isStereoEnabled() const { return mIsStereo; }

			virtual void	getNearClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight);
			virtual void	getFarClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight);

			virtual const mat4&		getProjectionMatrix() const;
			virtual const dmat4&	getModelViewMatrix() const;
			virtual const dmat4&	getInverseModelViewMatrix() const;

			virtual void	calcModelView();
			virtual void	calcInverseModelView();
			virtual void	calcProjection();

		protected:
			mat4			mProjectionMatrixLeft, mInverseProjectionMatrixLeft;
			mat4			mProjectionMatrixRight, mInverseProjectionMatrixRight;
			dmat4			mModelViewMatrixLeft, mInverseModelViewMatrixLeft;
			dmat4			mModelViewMatrixRight, mInverseModelViewMatrixRight;

		private:
			bool			mIsStereo;
			bool			mIsLeft;

			float			mConvergence;
			float			mEyeSeparation;
		};

	}
}

#endif