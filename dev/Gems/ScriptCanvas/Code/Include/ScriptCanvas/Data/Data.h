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

#include <AZCore/Math/Aabb.h>
#include <AZCore/Math/Color.h>
#include <AZCore/Math/Crc.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string.h>
#include <ScriptCanvas/Core/Core.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace Data
    {
        //////////////////////////////////////////////////////////////////////////
        // type interface
        //////////////////////////////////////////////////////////////////////////
                
        /// \note CHANGING THE ORDER OR NUMBER OF VALUES IN THIS LIST ALMOST CERTAINLY INVALIDATES PREVIOUS DATA
        enum class eType : AZ::u32
        {
            Boolean,
            EntityID,
            Invalid,
            Number,
            BehaviorContextObject,
            String,
            Rotation,
            Transform,
            Vector3,
            Vector2,
            Vector4,
            AABB,
            Color,
            CRC,
            Matrix3x3,
            Matrix4x4,
            OBB,
            Plane,            
            // Function, 
            // List,
        };
        
        using AABBType = AZ::Aabb;
        using BooleanType = bool;
        using CRCType = AZ::Crc32;
        using ColorType = AZ::Color;
        using EntityIDType = AZ::EntityId;
        using Matrix3x3Type = AZ::Matrix3x3;
        using Matrix4x4Type = AZ::Matrix4x4;
        using NumberType = double;
        using OBBType = AZ::Obb;
        using PlaneType = AZ::Plane;
        using RotationType = AZ::Quaternion;
        using StringType = AZStd::string;
        using TransformType = AZ::Transform;
        using Vector2Type = AZ::Vector2;
        using Vector3Type = AZ::Vector3;
        using Vector4Type = AZ::Vector4;

        class Type
        {
        public:
            AZ_TYPE_INFO(Type, "{0EADF8F5-8AB8-42E9-9C50-F5C78255C817}");
            
            static void Reflect(AZ::ReflectContext* reflection);
                        
            static Type AABB();
            static Type BehaviorContextObject(const AZ::Uuid& aztype);
            static Type Boolean();
            static Type Color();
            static Type CRC();
            static Type EntityID();
            static Type Invalid();
            static Type Matrix3x3();
            static Type Matrix4x4();
            static Type Number();
            static Type OBB();
            static Type Plane();
            static Type Rotation();
            static Type String();
            static Type Transform();
            static Type Vector2();
            static Type Vector3();
            static Type Vector4();
            
            // the default ctr produces the invalid type, and is only here to help the compiler
            Type();
            
            const AZ::Uuid& GetAZType() const;
            eType GetType() const;
            bool IsValid() const;
            explicit operator bool() const;
            bool operator!() const;
            
            // raw equality checks are never desired, use IS_A and IS_EXACTLY_A
            bool operator==(const Type& other) const = delete;
            bool operator!=(const Type& other) const = delete;

            // returns true if this type is, or is derived from the other type
            // \todo support polymorphism
            AZ_FORCE_INLINE bool IS_A(const Type& other) const;
            AZ_FORCE_INLINE bool IS_EXACTLY_A(const Type& other) const;

            AZ_FORCE_INLINE bool IsConvertibleFrom(const AZ::Uuid& target) const;
            AZ_FORCE_INLINE bool IsConvertibleFrom(const Type& target) const;
            
            AZ_FORCE_INLINE bool IsConvertibleTo(const AZ::Uuid& target) const;
            AZ_FORCE_INLINE bool IsConvertibleTo(const Type& target) const;
            
            Type& operator=(const Type& source) = default;

        private:
            eType m_type;
            AZ::Uuid m_azType;
            Type* m_independentType = nullptr;
            
            explicit Type(eType type);
            // for BehaviorContextObjects specifically
            explicit Type(const AZ::Uuid& aztype);
        }; // class Type

        /**
         * assumes that azType is a valid script canvas type of some kind, asserts if not
         * favors native ScriptCanvas types over BehaviorContext class types with the corresponding AZ type id
         */
        Type FromAZType(const AZ::Uuid& aztype);
        
        // if azType is not a valid script canvas type of some kind, returns invalid, NOT for use at run-time
        Type FromAZTypeChecked(const AZ::Uuid& aztype);

        /**
        * assumes that azType is a valid script canvas type of some kind, asserts if not
        * favors BehaviorContext class types with the corresponding AZ type id over native ScriptCanvas types
        */
        Type FromBehaviorContextType(const AZ::Uuid& aztype);
        
        // if azType is not a valid script canvas type of some kind, returns invalid, NOT for use at run-time
        Type FromBehaviorContextTypeChecked(const AZ::Uuid& aztype);

        template<typename T>
        Type FromAZType();

        const char* GetBehaviorContextName(const AZ::Uuid& type);
        const char* GetName(const Type& type);

        // returns true if candidate is, or is derived from reference
        // todo support polymorphism
        bool IS_A(const Type& candidate, const Type& reference);
        bool IS_EXACTLY_A(const Type& candidate, const Type& reference);

        bool IsConvertible(const Type& source, const AZ::Uuid& target);
        bool IsConvertible(const Type& source, const Type& target);

        template<typename t_Type>
        struct Traits
        {
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = false;
            static const eType s_type = eType::Invalid;
            
            static AZ::Uuid GetAZType() { return azrtti_typeid<t_Type>(); }
            static const char* GetName() { return "invalid"; }
        };

        // a compile time map of eType back to underlying AZ type and traits
        template<eType>
        struct eTraits
        {
            
        };
                
        bool IsAutoBoxedType(const Type& type);
        bool IsValueType(const Type& type);
        bool IsVectorType(const AZ::Uuid& type);
        bool IsVectorType(const Type& type);
        AZ::Uuid ToAZType(const Type& type);

        bool IsAABB(const AZ::Uuid& type);
        bool IsAABB(const Type& type);
        bool IsBoolean(const AZ::Uuid& type);
        bool IsBoolean(const Type& type);
        bool IsColor(const AZ::Uuid& type);
        bool IsColor(const Type& type);
        bool IsCRC(const AZ::Uuid& type);
        bool IsCRC(const Type& type);
        bool IsEntityID(const AZ::Uuid& type);
        bool IsEntityID(const Type& type);
        bool IsNumber(const AZ::Uuid& type);
        bool IsNumber(const Type& type);
        bool IsMatrix3x3(const AZ::Uuid& type);
        bool IsMatrix3x3(const Type& type);
        bool IsMatrix4x4(const AZ::Uuid& type);
        bool IsMatrix4x4(const Type& type);
        bool IsOBB(const AZ::Uuid& type);
        bool IsOBB(const Type& type);
        bool IsPlane(const AZ::Uuid& type);
        bool IsPlane(const Type& type);
        bool IsRotation(const AZ::Uuid& type);
        bool IsRotation(const Type& type);
        bool IsString(const AZ::Uuid& type);
        bool IsString(const Type& type);
        bool IsTransform(const AZ::Uuid& type);
        bool IsTransform(const Type& type);
        bool IsVector2(const AZ::Uuid& type);
        bool IsVector2(const Type& type);
        bool IsVector3(const AZ::Uuid& type);
        bool IsVector3(const Type& type);
        bool IsVector4(const AZ::Uuid& type);
        bool IsVector4(const Type& type);                
        
        //////////////////////////////////////////////////////////////////////////
        // type implementation
        //////////////////////////////////////////////////////////////////////////
        template<typename T>
        AZ_INLINE Type FromAZType()
        {
            return FromAZType(azrtti_typeid<T>());
        }

        AZ_INLINE bool IsAABB(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AABBType>();
        }

        AZ_INLINE bool IsAABB(const Type& type)
        {
            return type.GetType() == eType::AABB;
        }

        AZ_INLINE bool IsBoolean(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<bool>();
        }

        AZ_INLINE bool IsBoolean(const Type& type)
        {
            return type.GetType() == eType::Boolean;
        }

        AZ_INLINE bool IsColor(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<ColorType>();
        }

        AZ_INLINE bool IsColor(const Type& type)
        {
            return type.GetType() == eType::Color;
        }

        AZ_INLINE bool IsCRC(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<CRCType>();
        }

        AZ_INLINE bool IsCRC(const Type& type)
        {
            return type.GetType() == eType::CRC;
        }

        AZ_INLINE bool IsEntityID(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::EntityId>();
        }

        AZ_INLINE bool IsEntityID(const Type& type)
        {
            return type.GetType() == eType::EntityID;
        }

        AZ_INLINE bool IsMatrix3x3(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Matrix3x3>();
        }

        AZ_INLINE bool IsMatrix3x3(const Type& type)
        {
            return type.GetType() == eType::Matrix3x3;
        }

        AZ_INLINE bool IsMatrix4x4(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Matrix4x4>();
        }

        AZ_INLINE bool IsMatrix4x4(const Type& type)
        {
            return type.GetType() == eType::Matrix4x4;
        }

        AZ_INLINE bool IsNumber(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<char>()
                || type == azrtti_typeid<short>()
                || type == azrtti_typeid<int>()
                || type == azrtti_typeid<long>()
                || type == azrtti_typeid<AZ::s8>()
                || type == azrtti_typeid<AZ::s64>()
                || type == azrtti_typeid<unsigned char>()
                || type == azrtti_typeid<unsigned int>()
                || type == azrtti_typeid<unsigned long>()
                || type == azrtti_typeid<unsigned short>()
                || type == azrtti_typeid<AZ::u64>()
                || type == azrtti_typeid<float>()
                || type == azrtti_typeid<double>()
                || type == azrtti_typeid<AZ::VectorFloat>();
        }

        AZ_INLINE bool IsNumber(const Type& type)
        {
            return type.GetType() == eType::Number;
        }

        AZ_INLINE bool IsOBB(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<OBBType>();
        }

        AZ_INLINE bool IsOBB(const Type& type)
        {
            return type.GetType() == eType::OBB;
        }

        AZ_INLINE bool IsPlane(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<PlaneType>();
        }

        AZ_INLINE bool IsPlane(const Type& type)
        {
            return type.GetType() == eType::Plane;
        }

        AZ_INLINE bool IsRotation(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<RotationType>();
        }

        AZ_INLINE bool IsRotation(const Type& type)
        {
            return type.GetType() == eType::Rotation;
        }

        AZ_INLINE bool IsString(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<StringType>();
        }

        AZ_INLINE bool IsString(const Type& type)
        {
            return type.GetType() == eType::String;
        }

        AZ_INLINE bool IsTransform(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<TransformType>();
        }

        AZ_INLINE bool IsTransform(const Type& type)
        {
            return type.GetType() == eType::Transform;
        }

        AZ_INLINE bool IsVector3(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector3>();
        }

        AZ_INLINE bool IsVector2(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector2>();
        }

        AZ_INLINE bool IsVector4(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector4>();
        }

        AZ_INLINE bool IsVector3(const Type& type)
        {
            return type.GetType() == eType::Vector3;
        }

        AZ_INLINE AZ::Uuid ToAZType(const Type& type)
        {
            switch (type.GetType())
            {
            case eType::AABB:
                return azrtti_typeid<AABBType>();

            case eType::BehaviorContextObject:
                return type.GetAZType();

            case eType::Boolean:
                return azrtti_typeid<bool>();

            case eType::Color:
                return azrtti_typeid<ColorType>();

            case eType::CRC:
                return azrtti_typeid<CRCType>();

            case eType::EntityID:
                return azrtti_typeid<AZ::EntityId>();

            case eType::Invalid:
                return AZ::Uuid::CreateNull();

            case eType::Matrix3x3:
                return azrtti_typeid<AZ::Matrix3x3>();

            case eType::Matrix4x4:
                return azrtti_typeid<AZ::Matrix4x4>();

            case eType::Number:
                return azrtti_typeid<NumberType>();

            case eType::OBB:
                return azrtti_typeid<OBBType>();

            case eType::Plane:
                return azrtti_typeid<PlaneType>();

            case eType::Rotation:
                return azrtti_typeid<RotationType>();

            case eType::String:
                return azrtti_typeid<StringType>();

            case eType::Transform:
                return azrtti_typeid<TransformType>();

            case eType::Vector2:
                return azrtti_typeid<AZ::Vector2>();

            case eType::Vector3:
                return azrtti_typeid<AZ::Vector3>();

            case eType::Vector4:
                return azrtti_typeid<AZ::Vector4>();

            default:
                AZ_Assert(false, "Invalid type!");
                // check for behavior context support
                return AZ::Uuid::CreateNull();
            }
        }

        AZ_INLINE bool IsVectorType(const AZ::Uuid& type)
        {
            return type == azrtti_typeid<AZ::Vector3>()
                || type == azrtti_typeid<AZ::Vector2>()
                || type == azrtti_typeid<AZ::Vector4>();
        }

        AZ_INLINE bool IsVectorType(const Type& type)
        {
            static const AZ::u32 s_vectorTypes =
            {
                1 << static_cast<AZ::u32>(eType::Vector3)
                | 1 << static_cast<AZ::u32>(eType::Vector2)
                | 1 << static_cast<AZ::u32>(eType::Vector4)
            };

            return ((1 << static_cast<AZ::u32>(type.GetType())) & s_vectorTypes) != 0;
        }
                
        template<>
        struct Traits<AABBType>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::AABB;
            static const char* GetName() { return "AABB"; }
        };

        template<>
        struct Traits<BooleanType>
        {
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::Boolean;
            static const char* GetName() { return "Boolean"; }
        };

        template<>
        struct Traits<ColorType>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Color;
            static const char* GetName() { return "Color"; }
        };

        template<>
        struct Traits<CRCType>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::CRC;
            static const char* GetName() { return "CRC"; }
        };

        template<>
        struct Traits<AZ::EntityId>
        {
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::EntityID;
            static const char* GetName() { return "EntityID"; }
        };

        template<>
        struct Traits<AZ::Matrix3x3>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Matrix3x3;
            static const char* GetName() { return "Matrix3x3"; }
        };

        template<>
        struct Traits<AZ::Matrix4x4>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Matrix4x4;
            static const char* GetName() { return "Matrix4x4"; }
        };

        template<>
        struct Traits<NumberType>
        {
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::Number;
            static const char* GetName() { return "Number"; }
        };

        template<>
        struct Traits<OBBType>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::OBB;
            static const char* GetName() { return "OBB"; }
        };

        template<>
        struct Traits<PlaneType>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Plane;
            static const char* GetName() { return "Plane"; }
        };

        template<>
        struct Traits<RotationType>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Rotation;
            static const char* GetName() { return "Rotation"; }
        };

        template<>
        struct Traits<StringType>
        {
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::String;
            static const char* GetName() { return "String"; }
        };

        template<>
        struct Traits<TransformType>
        {
            static const bool s_isAutoBoxed = false;
            static const bool s_isNative = true;
            static const eType s_type = eType::Transform;
            static const char* GetName() { return "Transform"; }
        };

        template<>
        struct Traits<AZ::Vector2>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Vector2;
            static const char* GetName() { return "Vector2"; }
        };

        template<>
        struct Traits<AZ::Vector3>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Vector3;
            static const char* GetName() { return "Vector3"; }
        };

        template<>
        struct Traits<AZ::Vector4>
        {
            static const bool s_isAutoBoxed = true;
            static const bool s_isNative = true;
            static const eType s_type = eType::Vector4;
            static const char* GetName() { return "Vector4"; }
        };

        template<>
        struct eTraits<eType::AABB>
        {
            static const Traits<AABBType> s_traits;
        };

        template<>
        struct eTraits<eType::Boolean>
        {
            static const Traits<BooleanType> s_traits;
        };

        template<>
        struct eTraits<eType::Color>
        {
            static const Traits<ColorType> s_traits;
        };

        template<>
        struct eTraits<eType::CRC>
        {
            static const Traits<CRCType> s_traits;
        };

        template<>
        struct eTraits<eType::EntityID>
        {
            static const Traits<EntityIDType> s_traits;
        };

        template<>
        struct eTraits<eType::Matrix3x3>
        {
            static const Traits<Matrix3x3Type> s_traits;
        };

        template<>
        struct eTraits<eType::Matrix4x4>
        {
            static const Traits<Matrix4x4Type> s_traits;
        };

        template<>
        struct eTraits<eType::Number>
        {
            static const Traits<NumberType> s_traits;
        };

        template<>
        struct eTraits<eType::OBB>
        {
            static const Traits<OBBType> s_traits;
        };

        template<>
        struct eTraits<eType::Plane>
        {
            static const Traits<PlaneType> s_traits;
        };

        template<>
        struct eTraits<eType::Rotation>
        {
            static const Traits<RotationType> s_traits;
        };

        template<>
        struct eTraits<eType::String>
        {
            static const Traits<StringType> s_traits;
        };

        template<>
        struct eTraits<eType::Transform>
        {
            static const Traits<TransformType> s_traits;
        };

        template<>
        struct eTraits<eType::Vector2>
        {
            static const Traits<Vector2Type> s_traits;
        };

        template<>
        struct eTraits<eType::Vector3>
        {
            static const Traits<Vector3Type> s_traits;
        };

        template<>
        struct eTraits<eType::Vector4>
        {
            static const Traits<Vector4Type> s_traits;
        };

        AZ_INLINE bool IsAutoBoxedType(const Type& type)
        {
            static const AZ::u32 s_autoBoxedTypes =
            {
                1 << static_cast<AZ::u32>(eType::AABB)
                | 1 << static_cast<AZ::u32>(eType::Color)
                | 1 << static_cast<AZ::u32>(eType::CRC)
                | 1 << static_cast<AZ::u32>(eType::Matrix3x3)
                | 1 << static_cast<AZ::u32>(eType::Matrix4x4)
                | 1 << static_cast<AZ::u32>(eType::OBB)
                | 1 << static_cast<AZ::u32>(eType::Rotation)
                | 1 << static_cast<AZ::u32>(eType::Transform)
                | 1 << static_cast<AZ::u32>(eType::Vector3)
                | 1 << static_cast<AZ::u32>(eType::Vector2)
                | 1 << static_cast<AZ::u32>(eType::Vector4)
            };

            return ((1 << static_cast<AZ::u32>(type.GetType())) & s_autoBoxedTypes) != 0;
        }

        AZ_INLINE bool IsValueType(const Type& type)
        {
            static const AZ::u32 s_valueTypes =
            { 
                  1 << static_cast<AZ::u32>(eType::AABB)
                | 1 << static_cast<AZ::u32>(eType::Boolean)
                | 1 << static_cast<AZ::u32>(eType::Color)
                | 1 << static_cast<AZ::u32>(eType::CRC)
                | 1 << static_cast<AZ::u32>(eType::EntityID)
                | 1 << static_cast<AZ::u32>(eType::Matrix3x3)
                | 1 << static_cast<AZ::u32>(eType::Matrix4x4)
                | 1 << static_cast<AZ::u32>(eType::Number)
                | 1 << static_cast<AZ::u32>(eType::OBB)
                | 1 << static_cast<AZ::u32>(eType::Rotation)
                | 1 << static_cast<AZ::u32>(eType::String)
                | 1 << static_cast<AZ::u32>(eType::Transform)
                | 1 << static_cast<AZ::u32>(eType::Vector3)
                | 1 << static_cast<AZ::u32>(eType::Vector2)
                | 1 << static_cast<AZ::u32>(eType::Vector4)
            };
            
            return ((1 << static_cast<AZ::u32>(type.GetType())) & s_valueTypes) != 0;
        }
               
        AZ_FORCE_INLINE Type::Type()
            : m_type(eType::Invalid)
            , m_azType(AZ::Uuid::CreateNull())
        {}

        AZ_FORCE_INLINE Type::Type(eType type)
            : m_type(type)
            , m_azType(AZ::Uuid::CreateNull())
        {}
        
        AZ_FORCE_INLINE Type::Type(const AZ::Uuid& aztype)
            : m_type(eType::BehaviorContextObject)
            , m_azType(aztype)
        {
            AZ_Error("ScriptCanvas", !aztype.IsNull(), "no invalid aztypes allowed");
        }

        AZ_FORCE_INLINE Type Type::AABB()
        {
            return Type(eType::AABB);
        }
        
        AZ_FORCE_INLINE Type Type::BehaviorContextObject(const AZ::Uuid& aztype)
        {
            return Type(aztype);
        }

        AZ_FORCE_INLINE Type Type::Boolean()
        {
            return Type(eType::Boolean);
        }

        AZ_FORCE_INLINE Type Type::Color()
        {
            return Type(eType::Color);
        }

        AZ_FORCE_INLINE Type Type::CRC()
        {
            return Type(eType::CRC);
        }
        
        AZ_FORCE_INLINE Type Type::EntityID()
        {
            return Type(eType::EntityID);
        }        

        AZ_FORCE_INLINE const AZ::Uuid& Type::GetAZType() const
        {
            AZ_Assert(m_type == eType::BehaviorContextObject, "this type doesn't expose an AZ type");
            return m_azType;
        }

        AZ_FORCE_INLINE eType Type::GetType() const
        {
            return m_type;
        }

        AZ_FORCE_INLINE Type Type::Invalid()
        {
            return Type();
        }

        AZ_FORCE_INLINE bool IS_A(const Type& candidate, const Type& reference)
        { 
            return candidate.IS_A(reference); 
        }

        AZ_FORCE_INLINE bool IS_EXACTLY_A(const Type& candidate, const Type& reference)
        {
            return candidate.IS_EXACTLY_A(reference);
        }

        AZ_FORCE_INLINE bool IsConvertible(const Type& source, const AZ::Uuid& target)
        {
            return source.IsConvertibleTo(target);
        }
        
        AZ_FORCE_INLINE bool IsConvertible(const Type& source, const Type& target)
        {
            return source.IsConvertibleTo(target);
        }
        
        AZ_FORCE_INLINE bool Type::IsConvertibleFrom(const AZ::Uuid& target) const
        {
            return FromAZType(target).IsConvertibleTo(*this);
        }

        AZ_FORCE_INLINE bool Type::IsConvertibleFrom(const Type& target) const
        {
            return target.IsConvertibleTo(*this);
        }

        AZ_FORCE_INLINE bool Type::IsConvertibleTo(const AZ::Uuid& target) const
        {
            return IsConvertibleTo(FromAZType(target));
        }

        AZ_FORCE_INLINE bool Type::IsConvertibleTo(const Type& target) const
        {
            AZ_Assert(!IS_A(target), "Don't mix concepts, it is too dangerous.");
            
            if (GetType() == eType::BehaviorContextObject)
            {
                return target.GetType() != eType::BehaviorContextObject && target.IsConvertibleTo(*this);
            }

            if (target.GetType() == eType::BehaviorContextObject)
            {
                return IS_A(FromAZType(target.GetAZType()));
            }

            switch (GetType())
            {
            case eType::Vector3:
            case eType::Vector2:
            case eType::Vector4:
                return IsVectorType(target) || (target.GetType() == eType::BehaviorContextObject && IsVectorType(target.GetAZType()));
            default:
                return false;
            }
        }
        
        AZ_FORCE_INLINE bool Type::IS_A(const Type& other) const
        {
            // \todo support polymorphism
            return IS_EXACTLY_A(other);
        }

        AZ_FORCE_INLINE bool Type::IS_EXACTLY_A(const Type& other) const
        {
            return m_type == other.m_type && m_azType == other.m_azType;
        }

        AZ_FORCE_INLINE bool Type::IsValid() const
        {
            return m_type != eType::Invalid;
        }

        AZ_FORCE_INLINE Type Type::Matrix3x3()
        {
            return Type(eType::Matrix3x3);
        }

        AZ_FORCE_INLINE Type Type::Matrix4x4()
        {
            return Type(eType::Matrix4x4);
        }

        AZ_FORCE_INLINE Type Type::Number()
        {
            return Type(eType::Number);
        }

        AZ_FORCE_INLINE Type Type::OBB()
        {
            return Type(eType::OBB);
        }

        AZ_FORCE_INLINE Type::operator bool() const
        {
            return m_type != eType::Invalid;
        }

        AZ_FORCE_INLINE bool Type::operator!() const
        {
            return m_type == eType::Invalid;
        }

        AZ_FORCE_INLINE Type Type::Plane()
        {
            return Type(eType::Plane);
        }

        AZ_FORCE_INLINE Type Type::Rotation()
        {
            return Type(eType::Rotation);
        }

        AZ_FORCE_INLINE Type Type::String()
        {
            return Type(eType::String);
        }

        AZ_FORCE_INLINE Type Type::Transform()
        {
            return Type(eType::Transform);
        }
 
        AZ_FORCE_INLINE Type Type::Vector3()
        {
            return Type(eType::Vector3);
        }

        AZ_FORCE_INLINE Type Type::Vector2()
        {
            return Type(eType::Vector2);
        }

        AZ_FORCE_INLINE Type Type::Vector4()
        {
            return Type(eType::Vector4);
        }

    } // namespace Data

} // namespace ScriptCanvas