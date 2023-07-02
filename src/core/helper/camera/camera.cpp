#include "camera.h"

Camera::Camera(glm::vec3 pPosition) {
	mPosition = pPosition;

	mWorldUp = LAKOT_CAMERA_WORLD_UP;
	mFrontVector = LAKOT_CAMERA_FRONT;

	mYaw = LAKOT_DEFAULT_CAMERA_YAW;
	mPitch = LAKOT_DEFAULT_CAMERA_PITCH;

	mZoom = LAKOT_DEFAULT_CAMERA_ZOOM;

	updateCameraVectors();
}

void Camera::updateCameraVectors() {
	mFrontVector = glm::normalize(glm::vec3(cos(glm::radians(mYaw)) * cos(glm::radians(mPitch)),
											sin(glm::radians(mPitch)),
											sin(glm::radians(mYaw)) * cos(glm::radians(mPitch))));
	mRightVector = glm::normalize(glm::cross(mFrontVector, mWorldUp));
	mUpVector = glm::normalize(glm::cross(mRightVector, mFrontVector));
}

glm::vec3 Camera::getPosition() {
	return mPosition;
}

glm::vec3 Camera::getFrontVector() {
	return mFrontVector;
}

glm::vec3 Camera::getUpVector() {
	return mUpVector;
}

glm::vec3 Camera::getRightVector() {
	return mRightVector;
}

double Camera::getZoom() {
	return mZoom;
}

void Camera::changePosition(glm::vec3 pAmount) {
	mPosition += pAmount;
}

void Camera::changeZoom(double pAmount) {
	mZoom -= pAmount;

	if (mZoom < LAKOT_CAMERA_MIN_ZOOM) {
		mZoom = LAKOT_CAMERA_MIN_ZOOM;
	}

	if (mZoom > LAKOT_CAMERA_MAX_ZOOM) {
		mZoom = LAKOT_CAMERA_MAX_ZOOM;
	}
}

void Camera::changeYaw(double pAmount) {
	mYaw += pAmount;
}

void Camera::changePitch(double pAmount) {
	mPitch += pAmount;

	if (mPitch > LAKOT_CAMERA_MAX_PITCH) {
		mPitch = LAKOT_CAMERA_MAX_PITCH;
	}

	if (mPitch < LAKOT_CAMERA_MIN_PITCH) {
		mPitch = LAKOT_CAMERA_MIN_PITCH;
	}
}
