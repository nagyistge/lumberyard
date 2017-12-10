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

#pragma once

/**
* A utility file to convert AZ Vectors between each other
*
* Having a separate utility system like this means that Vector
* classes do not need to rely on each other.
*/

namespace AZ
{
    //Forward Declarations
    class Vector2;
    class Vector3;
    class Vector4;

    class VectorFloat;

    /*
        Vector2 conversions
    */

    /**
    * Converts a Vector2 to a Vector3 
    *
    * @param v The Vector2 to convert
    * 
    * @return A Vector3 created from the given Vector2 with 0 for the Z component
    */
    Vector3 Vector2ToVector3(const Vector2& v);
    /**
    * Converts a Vector2 to a Vector3
    *
    * @param v The Vector2 to convert
    * @param z The new value for the resulting Vector3's Z component
    *
    * @return A Vector3 created from the given values
    */
    Vector3 Vector2ToVector3(const Vector2& v, const VectorFloat& z);

    /**
    * Converts a Vector2 to a Vector4
    *
    * @param v The Vector2 to convert
    *
    * @return A Vector4 created from the given Vector2 with 0 for the Z and W components
    */
    Vector4 Vector2ToVector4(const Vector2& v);
    /**
    * Converts a Vector2 to a Vector4
    *
    * @param v The Vector2 to convert
    * @param z The new value for the resulting Vector4's Z component
    *
    * @return A Vector4 created from the given values with a 0 for the W component
    */
    Vector4 Vector2ToVector4(const Vector2& v, const VectorFloat& z);
    /**
    * Converts a Vector2 to a Vector4
    *
    * @param v The Vector2 to convert
    * @param z The new value for the resulting Vector4's Z component
    * @param w The new value for the resulting Vector4's W component
    *
    * @return A Vector4 created from the given values
    */
    Vector4 Vector2ToVector4(const Vector2& v, const VectorFloat& z, const VectorFloat& w);

    /*
        Vector3 conversions
    */

    /**
    * Converts a Vector3 to a Vector2
    *
    * The Z value of the Vector3 will be dropped
    *
    * @param v The Vector3 to convert to a Vector2
    *
    * @return A Vector2 created from the given Vector2
    */
    Vector2 Vector3ToVector2(const Vector3& v);
    /**
    * Converts a Vector3 to a Vector4
    *
    * @param v The Vector3 to convert to a Vector4
    *
    * @return A Vector4 created from the given Vector3 with a 0 for the W component
    */
    Vector4 Vector3ToVector4(const Vector3& v);
    /**
    * Converts a Vector3 to a Vector4
    *
    * @param v The Vector3 to convert to a Vector4
    * @param w The new value for the resulting Vector4's W component
    *
    * @return A Vector4 created from the given values
    */
    Vector4 Vector3ToVector4(const Vector3& v, const VectorFloat& w);

    /*
        Vector4 conversions
    */

    /**
    * Converts a Vector4 to a Vector2
    *
    * The Z and W components of the given Vector4 will be dropped
    *
    * @param v The Vector2 to convert to a Vector4
    *
    * @return A Vector4 created from the given Vector2 with 0s for the Z and W components
    */
    Vector2 Vector4ToVector2(const Vector4& v);
    /**
    * Converts a Vector4 to a Vector3
    *
    * The W component of the given Vector4 will be dropped
    *
    * @param v The Vector3 to convert to a Vector4
    *
    * @return A Vector4 created from the given Vector3 with a 0 for the W component
    */
    Vector3 Vector4ToVector3(const Vector4& v);

} //namespace AZ

#include <AzCore/Math/Internal/VectorConversions.inl>