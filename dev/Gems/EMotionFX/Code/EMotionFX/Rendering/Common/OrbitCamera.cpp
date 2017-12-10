/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "OrbitCamera.h"
#include <EMotionFX/Source/EMotionFXManager.h>


namespace MCommon
{
    // constructor
    OrbitCamera::OrbitCamera()
        : LookAtCamera()
    {
        Reset();
    }


    // destructor
    OrbitCamera::~OrbitCamera()
    {
    }


    // reset the camera attributes
    void OrbitCamera::Reset(float flightTime)
    {
        // reset the parent class attributes
        LookAtCamera::Reset();

        mMinDistance            = mNearClipDistance;
        mMaxDistance            = mFarClipDistance * 0.5f;
        mPosition               = MCore::Vector3(0.0f, 0.0f, 0.0f);
        mPositionDelta          = AZ::Vector2(0.0f, 0.0f);

        if (flightTime < MCore::Math::epsilon)
        {
            mFlightActive           = false;
            mCurrentDistance        = (float)MCore::Distance::ConvertValue(5.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
            mAlpha                  = GetDefaultAlpha();
            mBeta                   = GetDefaultBeta();
            mTarget                 = MCore::Vector3(0.0f, 0.0f, 0.0f);
        }
        else
        {
            mFlightActive           = true;
            mFlightMaxTime          = flightTime;
            mFlightCurrentTime      = 0.0f;
            mFlightSourceDistance   = mCurrentDistance;
            mFlightTargetDistance   = (float)MCore::Distance::ConvertValue(5.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
            mFlightSourcePosition   = mTarget;
            mFlightTargetPosition   = MCore::Vector3(0.0f, 0.0f, 0.0f);
            mFlightSourceAlpha      = mAlpha;
            mFlightTargetAlpha      = GetDefaultAlpha();
            mFlightSourceBeta       = mBeta;
            mFlightTargetBeta       = GetDefaultBeta();
        }
    }


    // update limits
    void OrbitCamera::AutoUpdateLimits()
    {
        mMinDistance    = mNearClipDistance;
        mMaxDistance    = mFarClipDistance * 0.5f;
    }


    void OrbitCamera::StartFlight(float distance, const MCore::Vector3& position, float alpha, float beta, float flightTime)
    {
        mFlightActive           = true;
        mFlightMaxTime          = flightTime;
        mFlightCurrentTime      = 0.0f;
        mFlightSourceDistance   = mCurrentDistance;
        mFlightSourcePosition   = mTarget;
        mFlightTargetDistance   = distance;
        mFlightTargetPosition   = position;
        mFlightSourceAlpha      = mAlpha;
        mFlightTargetAlpha      = alpha;
        mFlightSourceBeta       = mBeta;
        mFlightTargetBeta       = beta;
    }


    // closeup view of the given bounding box
    void OrbitCamera::ViewCloseup(const MCore::AABB& boundingBox, float flightTime)
    {
        mFlightActive           = true;
        mFlightMaxTime          = flightTime;
        mFlightCurrentTime      = 0.0f;
        mFlightSourceDistance   = mCurrentDistance;
        mFlightSourcePosition   = mTarget;
        const float distanceHorizontalFOV = boundingBox.CalcRadius() / MCore::Math::Tan(0.5f * MCore::Math::DegreesToRadians(mFOV));
        const float distanceVerticalFOV = boundingBox.CalcRadius() / MCore::Math::Tan(0.5f * MCore::Math::DegreesToRadians(mFOV * mAspect));
        mFlightTargetDistance   = MCore::Max(distanceHorizontalFOV, distanceVerticalFOV) * 0.9f;
        mFlightTargetPosition   = boundingBox.CalcMiddle();
        mFlightSourceAlpha      = mAlpha;
        mFlightSourceAlpha      = mAlpha;
        mFlightTargetAlpha      = GetDefaultAlpha();
        mFlightSourceBeta       = mBeta;
        mFlightTargetBeta       = GetDefaultBeta();

        // make sure the target flight distance is in range
        if (mFlightTargetDistance < mMinDistance)
        {
            mFlightTargetDistance = mMinDistance;
        }
        if (mFlightTargetDistance > mMaxDistance)
        {
            mFlightTargetDistance = mMaxDistance;
        }
    }


    // process input and modify the camera attributes
    void OrbitCamera::ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);

        // is left mouse button pressed?
        // rotate camera around target point
        if (leftButtonPressed && rightButtonPressed == false && middleButtonPressed == false)
        {
            // rotate our camera
            mAlpha  += mRotationSpeed * (float)-mouseMovementX;
            mBeta   += mRotationSpeed * (float) mouseMovementY;

            // prevent the camera from looking upside down
            if (mBeta >= 90.0f - 0.01f)
            {
                mBeta =  90.0f - 0.01f;
            }

            if (mBeta <= -90.0f + 0.01f)
            {
                mBeta =  -90.0f + 0.01f;
            }

            // reset the camera to no rotation if we made a whole circle
            if (mAlpha >= 360.0f || mAlpha <= -360.0f)
            {
                mAlpha = 0.0f;
            }
        }

        // is right mouse button pressed?
        // zoom camera in or out
        if (leftButtonPressed == false && rightButtonPressed && middleButtonPressed == false)
        {
            const float distanceScale = mCurrentDistance * 0.002f;
            mCurrentDistance += (float)-mouseMovementY * distanceScale;
        }

        // is middle (or left+right) mouse button pressed?
        // move camera forward, backward, left or right
        if ((leftButtonPressed == false && rightButtonPressed == false && middleButtonPressed) ||
            (leftButtonPressed &&  rightButtonPressed  && middleButtonPressed == false))
        {
            const float distanceScale = mCurrentDistance * 0.002f;

            //if (MCore::GetCoordinateSystem().IsRightHanded())
            //distanceScale *= -1.0f;

            mPositionDelta.SetX((float)mouseMovementX * distanceScale);
            mPositionDelta.SetY((float)mouseMovementY * distanceScale);
        }
    }


    // update the camera
    void OrbitCamera::Update(float timeDelta)
    {
        if (mFlightActive)
        {
            mFlightCurrentTime += timeDelta;

            const float normalizedTime      = mFlightCurrentTime / mFlightMaxTime;
            const float interpolatedTime    = MCore::CosineInterpolate<float>(0.0f, 1.0f, normalizedTime);

            mTarget             = mFlightSourcePosition + (mFlightTargetPosition - mFlightSourcePosition) * interpolatedTime;
            mCurrentDistance    = mFlightSourceDistance + (mFlightTargetDistance - mFlightSourceDistance) * interpolatedTime;
            mAlpha              = mFlightSourceAlpha + (mFlightTargetAlpha - mFlightSourceAlpha) * interpolatedTime;
            mBeta               = mFlightSourceBeta + (mFlightTargetBeta - mFlightSourceBeta) * interpolatedTime;

            if (mFlightCurrentTime >= mFlightMaxTime)
            {
                mFlightActive       = false;
                mTarget             = mFlightTargetPosition;
                mCurrentDistance    = mFlightTargetDistance;
                mAlpha              = mFlightTargetAlpha;
                mBeta               = mFlightTargetBeta;
            }
        }

        // HACK TODO REMOVEME !!!
        const float scale = 1.0f;

        mCurrentDistance *= scale;

        if (mCurrentDistance <= mMinDistance * scale)
        {
            mCurrentDistance = mMinDistance * scale;
        }

        if (mCurrentDistance >= mMaxDistance * scale)
        {
            mCurrentDistance = mMaxDistance * scale;
        }

        // calculate unit direction vector based on our two angles
        MCore::Vector3 unitSphereVector;
        unitSphereVector.x  = MCore::Math::Cos(MCore::Math::DegreesToRadians(mAlpha)) * MCore::Math::Cos(MCore::Math::DegreesToRadians(mBeta));
        unitSphereVector.y  = MCore::Math::Sin(MCore::Math::DegreesToRadians(mAlpha)) * MCore::Math::Cos(MCore::Math::DegreesToRadians(mBeta));
        unitSphereVector.z  = MCore::Math::Sin(MCore::Math::DegreesToRadians(mBeta));

        // calculate the right and the up vector based on the direction vector
        MCore::Vector3 rightVec = unitSphereVector.Cross(MCore::Vector3(0.0f, 0.0f, 1.0f)).Normalized();
        MCore::Vector3 upVec    = rightVec.Cross(unitSphereVector).Normalized();

        // calculate the lookat target and the camera position using our rotation sphere vectors
        mTarget             += (rightVec * mPositionDelta.GetX()) * mTranslationSpeed + (upVec * mPositionDelta.GetY()) * mTranslationSpeed;
        mPosition           = mTarget + (unitSphereVector * mCurrentDistance);

        // reset the position delta
        mPositionDelta      = AZ::Vector2(0.0f, 0.0f);

        // update our lookat camera at the very end
        LookAtCamera::Update();
    }


    // set all attributes to define a unique camera transformation and update it afterwards
    void OrbitCamera::Set(float alpha, float beta, float currentDistance, const MCore::Vector3& target)
    {
        SetAlpha(alpha);
        SetBeta(beta);
        SetCurrentDistance(currentDistance);
        SetTarget(target);
        Update();
    }
} // namespace MCommon
