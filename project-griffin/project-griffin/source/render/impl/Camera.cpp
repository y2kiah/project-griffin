#include "../Camera.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>

#include <glm/gtx/euler_angles.hpp> // TEMP

namespace griffin {
	namespace render {
		using namespace glm;

		mat4 alignZAxisWithTarget(vec3 targetDir, vec3 upDir);

		const vec3 c_xAxis(1.0f, 0.0f, 0.0f);
		const vec3 c_yAxis(0.0f, 1.0f, 0.0f);
		const vec3 c_zAxis(0.0f, 0.0f, 1.0f);


		///////////////////////////////////////////////////////////////////////////////////////////
		// CameraPersp2

		const mat4& CameraPersp2::getViewProjection() const
		{
			return mViewProjectionMatrix;
		}

		vec3 CameraPersp2::getEulerAngles() const
		{
			return eulerAngles(mViewRotationQuat);
		}

		void CameraPersp2::setTranslationYawPitchRoll(const vec3& position, float yaw, float pitch, float roll)
		{
			setEyePoint(position);
			mat4 rotation = eulerAngleZ(roll);
			rotation *= eulerAngleXY(pitch, yaw);
			mViewRotationQuat = quat_cast(rotation);
			
			// could be delayed?
			mViewMatrix = translate(rotation, -mEyePoint);
			mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
		}

		void CameraPersp2::lookAt(const vec3 &position, const vec3 &target, const vec3 &worldUp)
		{
			setEyePoint(position);
			mViewMatrix = glm::lookAt(mEyePoint, target, worldUp);
			mViewRotationQuat = quat_cast(mViewMatrix);
			
			// could be delayed?
			mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
		}


		///////////////////////////////////////////////////////////////////////////////////////////
		// Camera

		void Camera::setEyePoint(const vec3 &aEyePoint)
		{
			mEyePoint = aEyePoint;
			mModelViewCached = false;
		}

		void Camera::setCenterOfInterestPoint(const vec3 &centerOfInterestPoint)
		{
			mCenterOfInterest = distance(mEyePoint, centerOfInterestPoint);
			lookAt(centerOfInterestPoint);
		}

		void Camera::setViewDirection(const vec3 &aViewDirection)
		{
			mViewDirection = normalize(aViewDirection);
			mOrientation = quat(vec3(0.0f, 0.0f, -1.0f), mViewDirection);
			mModelViewCached = false;
		}

		void Camera::setOrientation(const quat &aOrientation)
		{
			mOrientation = normalize(aOrientation);
			mViewDirection = mOrientation * vec3(0.0f, 0.0f, -1.0f);
			mModelViewCached = false;
		}

		void Camera::setWorldUp(const vec3 &aWorldUp)
		{
			mWorldUp = normalize(aWorldUp);
			mOrientation = normalize(quat(alignZAxisWithTarget(-mViewDirection, mWorldUp)));
			mModelViewCached = false;
		}

		void Camera::lookAt(const vec3 &target)
		{
			mViewDirection = normalize(target - mEyePoint);
			mOrientation = normalize(quat(alignZAxisWithTarget(-mViewDirection, mWorldUp)));
			mModelViewCached = false;
		}

		void Camera::lookAt(const vec3 &aEyePoint, const vec3 &target)
		{
			mEyePoint = aEyePoint;
			mViewDirection = normalize(target - mEyePoint);
			mOrientation = normalize(quat(alignZAxisWithTarget(-mViewDirection, mWorldUp)));
			mModelViewCached = false;
		}

		void Camera::lookAt(const vec3 &aEyePoint, const vec3 &target, const vec3 &aWorldUp)
		{
			mEyePoint = aEyePoint;
			mWorldUp = normalize(aWorldUp);
			mViewDirection = normalize(target - mEyePoint);
			mOrientation = normalize(quat(alignZAxisWithTarget(-mViewDirection, mWorldUp)));
			mModelViewCached = false;
		}

		void Camera::getNearClipCoordinates(vec3 *topLeft, vec3 *topRight, vec3 *bottomLeft, vec3 *bottomRight)
		{
			calcMatrices();

			vec3 viewDirection(normalize(mViewDirection));

			*topLeft = mEyePoint + (mNearClip * viewDirection) + (mFrustumTop * mV) + (mFrustumLeft * mU);
			*topRight = mEyePoint + (mNearClip * viewDirection) + (mFrustumTop * mV) + (mFrustumRight * mU);
			*bottomLeft = mEyePoint + (mNearClip * viewDirection) + (mFrustumBottom * mV) + (mFrustumLeft * mU);
			*bottomRight = mEyePoint + (mNearClip * viewDirection) + (mFrustumBottom * mV) + (mFrustumRight * mU);
		}

		void Camera::getFarClipCoordinates(vec3 *topLeft, vec3 *topRight, vec3 *bottomLeft, vec3 *bottomRight)
		{
			calcMatrices();

			vec3 viewDirection(normalize(mViewDirection));

			float ratio = mFarClip / mNearClip;

			*topLeft = mEyePoint + (mFarClip * viewDirection) + (ratio * mFrustumTop * mV) + (ratio * mFrustumLeft * mU);
			*topRight = mEyePoint + (mFarClip * viewDirection) + (ratio * mFrustumTop * mV) + (ratio * mFrustumRight * mU);
			*bottomLeft = mEyePoint + (mFarClip * viewDirection) + (ratio * mFrustumBottom * mV) + (ratio * mFrustumLeft * mU);
			*bottomRight = mEyePoint + (mFarClip * viewDirection) + (ratio * mFrustumBottom * mV) + (ratio * mFrustumRight * mU);
		}

		void Camera::getFrustum(float *left, float *top, float *right, float *bottom, float *near, float *far)
		{
			calcMatrices();

			*left = mFrustumLeft;
			*top = mFrustumTop;
			*right = mFrustumRight;
			*bottom = mFrustumBottom;
			*near = mNearClip;
			*far = mFarClip;
		}

		//Ray Camera::generateRay(float uPos, float vPos, float imagePlaneApectRatio) const
		//{
		//	calcMatrices();

		//	float s = (uPos - 0.5f) * imagePlaneApectRatio;
		//	float t = (vPos - 0.5f);
		//	float viewDistance = imagePlaneApectRatio / math<float>::abs(mFrustumRight - mFrustumLeft) * mNearClip;
		//	return Ray(mEyePoint, (mU * s + mV * t - (mW * viewDistance)).normalized());
		//}

		void Camera::getBillboardVectors(vec3 *right, vec3 *up) const
		{
			auto& m = getModelViewMatrix();
			right->x = m[0][0];
			right->y = m[1][0];
			right->z = m[2][0];
			up->x    = m[0][1];
			up->y    = m[1][1];
			up->z    = m[2][1];
		}

		vec2 Camera::worldToScreen(const vec3 &worldCoord, float screenWidth, float screenHeight) const
		{
			vec3 eyeCoord = (vec4(worldCoord, 1.0f) * getModelViewMatrix()).xyz;
			vec3 ndc = (vec4(eyeCoord, 1.0f) * getProjectionMatrix()).xyz;

			return vec2((ndc.x + 1.0f) / 2.0f * screenWidth, (1.0f - (ndc.y + 1.0f) / 2.0f) * screenHeight);
		}

		vec3 Camera::worldToEye(const vec3 &worldCoord)
		{
			return vec4(vec4(worldCoord, 1.0f) * getModelViewMatrix()).xyz;
		}

		float Camera::worldToEyeDepth(const vec3 &worldCoord) const
		{
			auto &m = getModelViewMatrix();
			float d = m[0][2] * worldCoord.x + m[1][2] * worldCoord.y + m[2][2] * worldCoord.z + m[3][2];
			return d;
		}

		vec3 Camera::worldToNdc(const vec3 &worldCoord)
		{
			vec4 eye = vec4(worldCoord, 1.0f) * getModelViewMatrix();
			return vec4(eye * getProjectionMatrix()).xyz;
		}

		//* This only mostly works
		/*float Camera::getScreenRadius(const Sphere &sphere, float screenWidth, float screenHeight) const
		{
			vec2 screenCenter(worldToScreen(sphere.getCenter(), screenWidth, screenHeight));
			vec3 orthog = mViewDirection.getOrthogonal().normalized();
			vec2 screenPerimeter = worldToScreen(sphere.getCenter() + sphere.getRadius() * orthog, screenWidth, screenHeight);
			return distance(screenPerimeter, screenCenter);
		}*/

		void Camera::calcMatrices()
		{
			if (!mModelViewCached) calcModelView();
			if (!mProjectionCached) calcProjection();

			// note: calculation of the inverse modelview matrices is postponed until actually requested
			//if( ! mInverseModelViewCached ) calcInverseModelView();
		}

		void Camera::calcModelView()
		{
			mW = normalize(-mViewDirection);
			mU = mOrientation * c_xAxis;
			mV = mOrientation * c_yAxis;

			vec3 d(dot(-mEyePoint, mU), dot(-mEyePoint, mV), dot(-mEyePoint, mW));
			auto& m = mModelViewMatrix;
			m[0][0] = mU.x; m[1][0] = mU.y; m[2][0] = mU.z; m[3][0] = d.x;
			m[0][1] = mV.x; m[1][1] = mV.y; m[2][1] = mV.z; m[3][1] = d.y;
			m[0][2] = mW.x; m[1][2] = mW.y; m[2][2] = mW.z; m[3][2] = d.z;
			m[0][3] = 0.0f; m[1][3] = 0.0f; m[2][3] = 0.0f; m[3][3] = 1.0f;

			mModelViewCached = true;
			mInverseModelViewCached = false;
		}

		void Camera::calcInverseModelView()
		{
			if (!mModelViewCached) calcModelView();

			mInverseModelViewMatrix = affineInverse(mModelViewMatrix);
			mInverseModelViewCached = true;
		}

		////////////////////////////////////////////////////////////////////////////////////////
		// CameraPersp

		CameraPersp::CameraPersp(int pixelWidth, int pixelHeight, float fovDegrees)
			: Camera(), mLensShift(0.0f)
		{
			float eyeX = pixelWidth / 2.0f;
			float eyeY = pixelHeight / 2.0f;
			float halfFov = 3.14159f * fovDegrees / 360.0f;
			float theTan = tanf(halfFov);
			float dist = eyeY / theTan;
			float nearDist = dist / 10.0f;	// near / far clip plane
			float farDist = dist * 10.0f;
			float aspect = pixelWidth / (float)pixelHeight;

			setPerspective(fovDegrees, aspect, nearDist, farDist);
			lookAt(vec3(eyeX, eyeY, dist), vec3(eyeX, eyeY, 0.0f));
		}

		CameraPersp::CameraPersp(int pixelWidth, int pixelHeight, float fovDegrees, float nearPlane, float farPlane)
			: Camera(), mLensShift(0.0f)
		{
			float halfFov, theTan, aspect;

			float eyeX = pixelWidth / 2.0f;
			float eyeY = pixelHeight / 2.0f;
			halfFov = pi<float>() * fovDegrees / 360.0f;
			theTan = tanf(halfFov);
			float dist = eyeY / theTan;
			aspect = pixelWidth / (float)pixelHeight;

			setPerspective(fovDegrees, aspect, nearPlane, farPlane);
			lookAt(vec3(eyeX, eyeY, dist), vec3(eyeX, eyeY, 0.0f));
		}

		// Creates a default camera resembling Maya Persp
		CameraPersp::CameraPersp()
			: Camera(), mLensShift(0.0f)
		{
			lookAt(vec3(28.0f, 21.0f, 28.0f), vec3(0.0f), c_yAxis);
			setCenterOfInterest(44.822f);
			setPerspective(35.0f, 1.0f, 0.1f, 1000.0f);
		}

		void CameraPersp::setPerspective(float verticalFovDegrees, float aspectRatio, float nearPlane, float farPlane)
		{
			mFov = verticalFovDegrees;
			mAspectRatio = aspectRatio;
			mNearClip = nearPlane;
			mFarClip = farPlane;

			mProjectionCached = false;
		}

		void CameraPersp::calcProjection()
		{
			mFrustumTop = mNearClip * tanf(pi<float>() / 180.0f * mFov * 0.5f);
			mFrustumBottom = -mFrustumTop;
			mFrustumRight = mFrustumTop * mAspectRatio;
			mFrustumLeft = -mFrustumRight;

			// perform lens shift
			if (mLensShift.y != 0.0f) {
				mFrustumTop = glm::mix(0.0f, 2.0f * mFrustumTop, 0.5f + 0.5f * mLensShift.y);
				mFrustumBottom = glm::mix(2.0f * mFrustumBottom, 0.0f, 0.5f + 0.5f * mLensShift.y);
			}

			if (mLensShift.x != 0.0f) {
				mFrustumRight = glm::mix(2.0f * mFrustumRight, 0.0f, 0.5f - 0.5f * mLensShift.x);
				mFrustumLeft = glm::mix(0.0f, 2.0f * mFrustumLeft, 0.5f - 0.5f * mLensShift.x);
			}

			mProjectionMatrix = perspective(radians(mFov), mAspectRatio, mNearClip, mFarClip);
			mProjectionCached = true;
		}

		void CameraPersp::setLensShift(float horizontal, float vertical)
		{
			mLensShift.x = horizontal;
			mLensShift.y = vertical;

			mProjectionCached = false;
		}

		/*CameraPersp	CameraPersp::getFrameSphere(const Sphere &worldSpaceSphere, int maxIterations) const
		{
			CameraPersp result = *this;
			result.setEyePoint(worldSpaceSphere.getCenter() - result.mViewDirection * getCenterOfInterest());

			float minDistance = 0.01f, maxDistance = 100000.0f;
			float curDistance = getCenterOfInterest();
			for (int i = 0; i < maxIterations; ++i) {
				float curRadius = result.getScreenRadius(worldSpaceSphere, 2.0f, 2.0f);
				if (curRadius < 1.0f) { // we should get closer
					maxDistance = curDistance;
					curDistance = (curDistance + minDistance) * 0.5f;
				}
				else { // we should get farther
					minDistance = curDistance;
					curDistance = (curDistance + maxDistance) * 0.5f;
				}
				result.setEyePoint(worldSpaceSphere.getCenter() - result.mViewDirection * curDistance);
			}

			result.setCenterOfInterest(result.getEyePoint().distance(worldSpaceSphere.getCenter()));
			return result;
		}*/

		////////////////////////////////////////////////////////////////////////////////////////
		// CameraOrtho
		CameraOrtho::CameraOrtho() :
			Camera()
		{
			lookAt(vec3(0.0f, 0.0f, 0.1f), vec3(0.0f), c_yAxis);
			setCenterOfInterest(0.1f);
			setFov(35.0f);
		}

		CameraOrtho::CameraOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane) :
			Camera()
		{
			mFrustumLeft = left;
			mFrustumRight = right;
			mFrustumTop = top;
			mFrustumBottom = bottom;
			mNearClip = nearPlane;
			mFarClip = farPlane;

			mProjectionCached = false;
			mModelViewCached = true;
			mInverseModelViewCached = true;
		}

		void CameraOrtho::setOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
		{
			mFrustumLeft = left;
			mFrustumRight = right;
			mFrustumTop = top;
			mFrustumBottom = bottom;
			mNearClip = nearPlane;
			mFarClip = farPlane;

			mProjectionCached = false;
		}

		void CameraOrtho::calcProjection()
		{
			mProjectionMatrix = glm::ortho(mFrustumLeft, mFrustumRight, mFrustumBottom, mFrustumTop, mNearClip, mFarClip);
			mProjectionCached = true;
		}

		////////////////////////////////////////////////////////////////////////////////////////
		// CameraStereo

		vec3 CameraStereo::getEyePointShifted() const
		{
			if (!mIsStereo)
				return mEyePoint;

			if (mIsLeft)
				return mEyePoint - mOrientation * c_xAxis * (0.5f * mEyeSeparation);
			else
				return mEyePoint + mOrientation * c_xAxis * (0.5f * mEyeSeparation);
		}

		void CameraStereo::getNearClipCoordinates(vec3 *topLeft, vec3 *topRight, vec3 *bottomLeft, vec3 *bottomRight)
		{
			calcMatrices();

			vec3 viewDirection(normalize(mViewDirection));

			vec3 eye(getEyePointShifted());

			float shift = 0.5f * mEyeSeparation * (mNearClip / mConvergence);
			shift *= (mIsStereo ? (mIsLeft ? 1.0f : -1.0f) : 0.0f);

			float left = mFrustumLeft + shift;
			float right = mFrustumRight + shift;

			*topLeft = eye + (mNearClip * viewDirection) + (mFrustumTop * mV) + (left * mU);
			*topRight = eye + (mNearClip * viewDirection) + (mFrustumTop * mV) + (right * mU);
			*bottomLeft = eye + (mNearClip * viewDirection) + (mFrustumBottom * mV) + (left * mU);
			*bottomRight = eye + (mNearClip * viewDirection) + (mFrustumBottom * mV) + (right * mU);
		}

		void CameraStereo::getFarClipCoordinates(vec3 *topLeft, vec3 *topRight, vec3 *bottomLeft, vec3 *bottomRight)
		{
			calcMatrices();

			vec3 viewDirection(normalize(mViewDirection));

			float ratio = mFarClip / mNearClip;

			vec3 eye(getEyePointShifted());

			float shift = 0.5f * mEyeSeparation * (mNearClip / mConvergence);
			shift *= (mIsStereo ? (mIsLeft ? 1.0f : -1.0f) : 0.0f);

			float left = mFrustumLeft + shift;
			float right = mFrustumRight + shift;

			*topLeft = eye + (mFarClip * viewDirection) + (ratio * mFrustumTop * mV) + (ratio * left * mU);
			*topRight = eye + (mFarClip * viewDirection) + (ratio * mFrustumTop * mV) + (ratio * right * mU);
			*bottomLeft = eye + (mFarClip * viewDirection) + (ratio * mFrustumBottom * mV) + (ratio * left * mU);
			*bottomRight = eye + (mFarClip * viewDirection) + (ratio * mFrustumBottom * mV) + (ratio * right * mU);
		}

		const mat4& CameraStereo::getProjectionMatrix() const
		{
			assert(mProjectionCached);

			if (!mIsStereo)
				return mProjectionMatrix;
			else if (mIsLeft)
				return mProjectionMatrixLeft;
			else
				return mProjectionMatrixRight;
		}

		const mat4& CameraStereo::getModelViewMatrix() const
		{
			assert(mModelViewCached);

			if (!mIsStereo)
				return mModelViewMatrix;
			else if (mIsLeft)
				return mModelViewMatrixLeft;
			else
				return mModelViewMatrixRight;
		}

		const mat4& CameraStereo::getInverseModelViewMatrix() const
		{
			assert(mInverseModelViewCached);

			if (!mIsStereo)
				return mInverseModelViewMatrix;
			else if (mIsLeft)
				return mInverseModelViewMatrixLeft;
			else
				return mInverseModelViewMatrixRight;
		}

		void CameraStereo::calcModelView()
		{
			// calculate default matrix first
			CameraPersp::calcModelView();

			mModelViewMatrixLeft = mModelViewMatrix;
			mModelViewMatrixRight = mModelViewMatrix;

			// calculate left matrix
			vec3 eye = mEyePoint - mOrientation * c_xAxis * (0.5f * mEyeSeparation);
			vec3 d = vec3(dot(-eye, mU), dot(-eye, mV), dot(-eye, mW));

			auto& m = mModelViewMatrixLeft;
			m[3][0] = d.x; m[3][1] = d.y; m[3][2] = d.z;

			// calculate right matrix
			eye = mEyePoint + mOrientation * c_xAxis * (0.5f * mEyeSeparation);
			d = vec3(dot(-eye, mU), dot(-eye, mV), dot(-eye, mW));

			m = mModelViewMatrixRight;
			m[3][0] = d.x; m[3][1] = d.y; m[3][2] = d.z;

			mModelViewCached = true;
			mInverseModelViewCached = false;
		}

		void CameraStereo::calcInverseModelView()
		{
			if (!mModelViewCached) calcModelView();

			mInverseModelViewMatrix = affineInverse(mModelViewMatrix);
			mInverseModelViewMatrixLeft = affineInverse(mModelViewMatrixLeft);
			mInverseModelViewMatrixRight = affineInverse(mModelViewMatrixRight);
			mInverseModelViewCached = true;
		}

		void CameraStereo::calcProjection()
		{
			// calculate default matrices first
			CameraPersp::calcProjection();

			mProjectionMatrixLeft = mProjectionMatrix;
			mInverseProjectionMatrixLeft = mInverseProjectionMatrix;

			mProjectionMatrixRight = mProjectionMatrix;
			mInverseProjectionMatrixRight = mInverseProjectionMatrix;

			// calculate left matrices
			auto& m = mProjectionMatrixLeft;
			m[2][0] = (mFrustumRight + mFrustumLeft + mEyeSeparation * (mNearClip / mConvergence)) / (mFrustumRight - mFrustumLeft);

			m = mInverseProjectionMatrixLeft;
			m[3][0] = (mFrustumRight + mFrustumLeft + mEyeSeparation * (mNearClip / mConvergence)) / (2.0f * mNearClip);

			// calculate right matrices
			m = mProjectionMatrixRight;
			m[2][0] = (mFrustumRight + mFrustumLeft - mEyeSeparation * (mNearClip / mConvergence)) / (mFrustumRight - mFrustumLeft);

			m = mInverseProjectionMatrixRight;
			m[3][0] = (mFrustumRight + mFrustumLeft - mEyeSeparation * (mNearClip / mConvergence)) / (2.0f * mNearClip);
			
			mProjectionCached = true;
		}


		mat4 alignZAxisWithTarget(vec3 targetDir, vec3 upDir)
		{
			// Ensure that the target direction is non-zero.
			if (length2(targetDir) == 0) {
				targetDir = c_zAxis;
			}

			// Ensure that the up direction is non-zero.
			if (length2(upDir) == 0) {
				upDir = c_yAxis;
			}

			// Check for degeneracies.  If the upDir and targetDir are parallel 
			// or opposite, then compute a new, arbitrary up direction that is
			// not parallel or opposite to the targetDir.
			if (length2(cross(upDir, targetDir)) == 0) {
				upDir = cross(targetDir, c_xAxis);
				if (length2(upDir) == 0) {
					upDir = cross(targetDir, c_zAxis);
				}
			}

			// Compute the x-, y-, and z-axis vectors of the new coordinate system.
			vec3 targetPerpDir = cross(upDir, targetDir);
			vec3 targetUpDir = cross(targetDir, targetPerpDir);

			// Rotate the x-axis into targetPerpDir (row 0),
			// rotate the y-axis into targetUpDir   (row 1),
			// rotate the z-axis into targetDir     (row 2).
			vec3 row[3];
			row[0] = normalize(targetPerpDir);
			row[1] = normalize(targetUpDir);
			row[2] = normalize(targetDir);

			mat4 mat(row[0].x, row[0].y, row[0].z, 0,
					 row[1].x, row[1].y, row[1].z, 0,
					 row[2].x, row[2].y, row[2].z, 0,
					 0, 0, 0, 1);

			return mat;
		}
	}
}
