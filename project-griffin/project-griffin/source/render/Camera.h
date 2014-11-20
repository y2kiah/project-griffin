/**
 * @file	Camera.h
 * @author	Jeff Kiah
**/
#pragma once

#ifndef GRIFFIN_CAMERA_H
#define GRIFFIN_CAMERA_H

#include <cmath>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

#include "cinder/Ray.h"

using glm::vec3;
using glm::vec2;
using glm::mat4x4;
using glm::quat;

namespace griffin {

	class Camera {
	public:
		Camera() : mModelViewCached(false), mProjectionCached(false), mInverseModelViewCached(false), mWorldUp(vec3::yAxis()) {}
		virtual ~Camera() {}

		vec3		getEyePoint() const { return mEyePoint; }
		void		setEyePoint(const vec3 &aEyePoint);

		float		getCenterOfInterest() const { return mCenterOfInterest; }
		void		setCenterOfInterest(float aCenterOfInterest) { mCenterOfInterest = aCenterOfInterest; }

		vec3		getCenterOfInterestPoint() const { return mEyePoint + mViewDirection * mCenterOfInterest; }
		void		setCenterOfInterestPoint(const vec3 &centerOfInterestPoint);

		vec3		getWorldUp() const { return mWorldUp; }
		void		setWorldUp(const vec3 &aWorldUp);

		void		lookAt(const vec3 &target);
		void		lookAt(const vec3 &aEyePoint, const vec3 &target);
		void		lookAt(const vec3 &aEyePoint, const vec3 &target, const vec3 &aUp);
		vec3		getViewDirection() const { return mViewDirection; }
		void		setViewDirection(const vec3 &aViewDirection);

		quat		getOrientation() const { return mOrientation; }
		void		setOrientation(const quat &aOrientation);

		float	getFov() const { return mFov; }
		void	setFov(float aFov) { mFov = aFov;  mProjectionCached = false; }
		float	getFovHorizontal() const { return toDegrees(2.0f * math<float>::atan(math<float>::tan(toRadians(mFov) * 0.5f) * mAspectRatio)); }
		void	setFovHorizontal(float aFov) { mFov = toDegrees(2.0f * math<float>::atan(math<float>::tan(toRadians(aFov) * 0.5f) / mAspectRatio));  mProjectionCached = false; }

		float	getAspectRatio() const { return mAspectRatio; }
		void	setAspectRatio(float aAspectRatio) { mAspectRatio = aAspectRatio; mProjectionCached = false; }
		float	getNearClip() const { return mNearClip; }
		void	setNearClip(float aNearClip) { mNearClip = aNearClip; mProjectionCached = false; }
		float	getFarClip() const { return mFarClip; }
		void	setFarClip(float aFarClip) { mFarClip = aFarClip; mProjectionCached = false; }

		virtual void	getNearClipCoordinates(vec3 *topLeft, vec3 *topRight, vec3 *bottomLeft, vec3 *bottomRight) const;
		virtual void	getFarClipCoordinates(vec3 *topLeft, vec3 *topRight, vec3 *bottomLeft, vec3 *bottomRight) const;

		//! Returns the coordinates of the camera's frustum, suitable for passing to \c glFrustum
		void	getFrustum(float *left, float *top, float *right, float *bottom, float *near, float *far) const;
		//! Returns whether the camera represents a perspective projection instead of an orthographic
		virtual bool isPersp() const = 0;

		virtual const mat4x4&	getProjectionMatrix() const { if (!mProjectionCached) calcProjection(); return mProjectionMatrix; }
		virtual const mat4x4&	getModelViewMatrix() const { if (!mModelViewCached) calcModelView(); return mModelViewMatrix; }
		virtual const mat4x4&	getInverseModelViewMatrix() const { if (!mInverseModelViewCached) calcInverseModelView(); return mInverseModelViewMatrix; }

		Ray		generateRay(float u, float v, float imagePlaneAspectRatio) const;
		void	getBillboardVectors(vec3 *right, vec3 *up) const;

		//! Converts a world-space coordinate \a worldCoord to screen coordinates as viewed by the camera, based ona s screen which is \a screenWidth x \a screenHeight pixels.
		vec3 worldToScreen(const vec3 &worldCoord, float screenWidth, float screenHeight) const;
		//! Converts a world-space coordinate \a worldCoord to eye-space, also known as camera-space. -Z is along the view direction.
		vec3 worldToEye(const vec3 &worldCoord) { return getModelViewMatrix().transformPointAffine(worldCoord); }
		//! Converts a world-space coordinate \a worldCoord to the z axis of eye-space, also known as camera-space. -Z is along the view direction. Suitable for depth sorting.
		float worldToEyeDepth(const vec3 &worldCoord) const { return getModelViewMatrix().m[2] * worldCoord.x + getModelViewMatrix().m[6] * worldCoord.y + getModelViewMatrix().m[10] * worldCoord.z + getModelViewMatrix().m[14]; }
		//! Converts a world-space coordinate \a worldCoord to normalized device coordinates
		vec3 worldToNdc(const vec3 &worldCoord) { vec3 eye = getModelViewMatrix().transformPointAffine(worldCoord); return getProjectionMatrix().transformPoint(eye); }


		float	getScreenRadius(const class Sphere &sphere, float screenWidth, float screenHeight) const;

	protected:
		vec3	mEyePoint;
		vec3	mViewDirection;
		quat	mOrientation;
		float	mCenterOfInterest;
		vec3	mWorldUp;

		float	mFov;
		float	mAspectRatio;
		float	mNearClip;
		float	mFarClip;

		mutable vec3		mU;	// Right vector
		mutable vec3		mV;	// Readjust up-vector
		mutable vec3		mW;	// Negative view direction

		mutable mat4x4		mProjectionMatrix, mInverseProjectionMatrix;
		mutable bool		mProjectionCached;
		mutable mat4x4		mModelViewMatrix;
		mutable bool		mModelViewCached;
		mutable mat4x4		mInverseModelViewMatrix;
		mutable bool		mInverseModelViewCached;

		mutable float		mFrustumLeft, mFrustumRight, mFrustumTop, mFrustumBottom;

		inline void		calcMatrices() const;

		virtual void	calcModelView() const;
		virtual void	calcInverseModelView() const;
		virtual void	calcProjection() const = 0;
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

		CameraPersp	getFrameSphere(const class Sphere &worldSpaceSphere, int maxIterations = 20) const;

	protected:
		vec2	mLensShift;

		virtual void	calcProjection() const;
	};

	class CameraOrtho : public Camera {
	public:
		CameraOrtho();
		CameraOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		void setOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		virtual bool	isPersp() const { return false; }

	protected:
		virtual void	calcProjection() const;
	};

	class CameraStereo : public CameraPersp {
	public:
		CameraStereo()
			: mConvergence(1.0f), mEyeSeparation(0.05f), mIsStereo(false), mIsLeft(true) {}
		CameraStereo(int pixelWidth, int pixelHeight, float fov)
			: CameraPersp(pixelWidth, pixelHeight, fov),
			mConvergence(1.0f), mEyeSeparation(0.05f), mIsStereo(false), mIsLeft(true) {} // constructs screen-aligned camera
		CameraStereo(int pixelWidth, int pixelHeight, float fov, float nearPlane, float farPlane)
			: CameraPersp(pixelWidth, pixelHeight, fov, nearPlane, farPlane),
			mConvergence(1.0f), mEyeSeparation(0.05f), mIsStereo(false), mIsLeft(true) {} // constructs screen-aligned camera

		//! Returns the current convergence, which is the distance at which there is no parallax.
		float			getConvergence() const { return mConvergence; }
		//! Sets the convergence of the camera, which is the distance at which there is no parallax.
		void			setConvergence(float distance, bool adjustEyeSeparation = false) {
			mConvergence = distance; mProjectionCached = false;

			if (adjustEyeSeparation)
				mEyeSeparation = mConvergence / 30.0f;
		}
		//! Returns the distance between the camera's for the left and right eyes.
		float			getEyeSeparation() const { return mEyeSeparation; }
		//! Sets the distance between the camera's for the left and right eyes. This affects the parallax effect. 
		void			setEyeSeparation(float distance) { mEyeSeparation = distance; mModelViewCached = false; mProjectionCached = false; }
		//! Returns the location of the currently enabled eye camera.
		vec3			getEyePointShifted() const;

		//! Enables the left eye camera.
		void			enableStereoLeft() { mIsStereo = true; mIsLeft = true; }
		bool			isStereoLeftEnabled() const { return mIsStereo && mIsLeft; }
		//! Enables the right eye camera.
		void			enableStereoRight() { mIsStereo = true; mIsLeft = false; }
		bool			isStereoRightEnabled() const { return mIsStereo && !mIsLeft; }
		//! Disables stereoscopic rendering, converting the camera to a standard CameraPersp.
		void			disableStereo() { mIsStereo = false; }
		bool			isStereoEnabled() const { return mIsStereo; }

		virtual void	getNearClipCoordinates(vec3 *topLeft, vec3 *topRight, vec3 *bottomLeft, vec3 *bottomRight) const;
		virtual void	getFarClipCoordinates(vec3 *topLeft, vec3 *topRight, vec3 *bottomLeft, vec3 *bottomRight) const;

		virtual const mat4x4&	getProjectionMatrix() const;
		virtual const mat4x4&	getModelViewMatrix() const;
		virtual const mat4x4&	getInverseModelViewMatrix() const;

	protected:
		mutable mat4x4	mProjectionMatrixLeft, mInverseProjectionMatrixLeft;
		mutable mat4x4	mProjectionMatrixRight, mInverseProjectionMatrixRight;
		mutable mat4x4	mModelViewMatrixLeft, mInverseModelViewMatrixLeft;
		mutable mat4x4	mModelViewMatrixRight, mInverseModelViewMatrixRight;

		virtual void	calcModelView() const;
		virtual void	calcInverseModelView() const;
		virtual void	calcProjection() const;
	private:
		bool			mIsStereo;
		bool			mIsLeft;

		float			mConvergence;
		float			mEyeSeparation;
	};

}

#endif