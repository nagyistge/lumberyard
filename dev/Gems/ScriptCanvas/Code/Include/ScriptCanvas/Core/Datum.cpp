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

#include "precompiled.h"
#include "Datum.h"

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Math/MathUtils.h>

#include "DatumBus.h"

namespace
{
    using namespace ScriptCanvas;
    
    template<typename t_Value>
    struct ImplicitConversionHelp
    {
        AZ_FORCE_INLINE bool Help(const Data::Type& sourceType, const void* source, const Data::Type& targetType, AZStd::any& target, const AZ::BehaviorClass* targetClass)
        {
            if (targetType.GetType() == Data::eType::Rotation)
            {
                AZ_Assert(sourceType.GetType() == Data::eType::BehaviorContextObject, "No other types are currently implicitly convertible");
                Data::RotationType& targetRotation = AZStd::any_cast<Data::RotationType&>(target);
                AZ_Assert(sourceType.GetAZType() == azrtti_typeid<Data::RotationType>(), "Rotation type not valid for ScriptCanvas conversion");
                targetRotation = *reinterpret_cast<const Data::RotationType*>(source);
            }
            else
            {
                AZ_Assert(targetType.GetType() == Data::eType::BehaviorContextObject, "No other types are currently implicitly convertible");
                AZ_Assert(targetClass, "Target class unknown, no conversion possible");
                const AZ::BehaviorClass& behaviorClass = *targetClass;
                AZ_Assert(behaviorClass.m_typeId == azrtti_typeid<Data::RotationType>(), "Rotation type not valid for ScriptCanvas conversion");
                const Data::RotationType& sourceRotation = *reinterpret_cast<const Data::RotationType*>(source);
                target = BehaviorContextObject::Create<Data::RotationType>(sourceRotation, behaviorClass);
            }

            return true;
        }
    };

    template<typename t_Value>
    AZ_FORCE_INLINE bool ConvertImplicitlyCheckedGeneric(const Data::Type& sourceType, const void* source, const Data::Type& targetType, AZStd::any& target, const AZ::BehaviorClass* targetClass)
    {
        if (targetType.GetType() == Data::Traits<t_Value>::s_type)
        {
            AZ_Assert(sourceType.GetType() == Data::eType::BehaviorContextObject, "Conversion to %s requires one type to be a BehaviorContextObject", Data::Traits<t_Value>::GetName());
            t_Value& targetValue = AZStd::any_cast<t_Value&>(target);
            AZ_Assert(sourceType.GetAZType() == azrtti_typeid<t_Value>(), "Value type not valid for ScriptCanvas conversion to %s", Data::Traits<t_Value>::GetName());
            targetValue = *reinterpret_cast<const t_Value*>(source);
        }
        else
        {
            AZ_Assert(targetType.GetType() == Data::eType::BehaviorContextObject, "Conversion to %s requires one type to be a BehaviorContextObject", Data::Traits<t_Value>::GetName());
            AZ_Assert(targetClass, "Target class unknown, no conversion possible");
            const AZ::BehaviorClass& behaviorClass = *targetClass;
            AZ_Assert(behaviorClass.m_typeId == azrtti_typeid<t_Value>(), "Value type not valid for ScriptCanvas conversion to %s", Data::Traits<t_Value>::GetName());
            const t_Value& sourceValue = *reinterpret_cast<const t_Value*>(source);
            target = BehaviorContextObject::Create<t_Value>(sourceValue, behaviorClass);
        }

        return true;
    }

    AZ_FORCE_INLINE bool ConvertImplicitlyCheckedVector2(const void* source, const Data::Type& targetType, AZStd::any& target, const AZ::BehaviorClass* targetClass)
    {
        const AZ::Vector2& sourceVector = *reinterpret_cast<const AZ::Vector2*>(source);

        if (Data::IsVectorType(targetType))
        {
            switch (targetType.GetType())
            {
            case Data::eType::Vector2:
                AZStd::any_cast<AZ::Vector2&>(target) = sourceVector;
                break;

            case Data::eType::Vector3:
                AZStd::any_cast<AZ::Vector3&>(target).Set(sourceVector.GetX(), sourceVector.GetY(), AZ::VectorFloat::CreateZero());
                break;

            case Data::eType::Vector4:
                AZStd::any_cast<AZ::Vector4&>(target).Set(sourceVector.GetX(), sourceVector.GetY(), AZ::VectorFloat::CreateZero(), AZ::VectorFloat::CreateZero());
                break;

            default:
                AZ_Assert(false, "Vector type unaccounted for in ScriptCanvas data model");
                return false;
            }
        }
        else
        {
            AZ_Assert(targetType.GetType() == Data::eType::BehaviorContextObject, "No other types are currently implicitly convertible");
            AZ_Assert(targetClass, "Target class unknown, no conversion possible");

            const AZ::BehaviorClass& behaviorClass = *targetClass;
            const AZ::Uuid& typeID = behaviorClass.m_typeId;

            if (typeID == azrtti_typeid<AZ::Vector3>())
            {
                target = BehaviorContextObject::Create<AZ::Vector3>(AZ::Vector3(sourceVector.GetX(), sourceVector.GetY(), AZ::VectorFloat::CreateZero()), behaviorClass);
            }
            else if (typeID == azrtti_typeid<AZ::Vector2>())
            {
                target = BehaviorContextObject::Create<AZ::Vector2>(sourceVector, behaviorClass);
            }
            else if (typeID == azrtti_typeid<AZ::Vector4>())
            {
                target = BehaviorContextObject::Create<AZ::Vector4>(AZ::Vector4(sourceVector.GetX(), sourceVector.GetY(), AZ::VectorFloat::CreateZero(), AZ::VectorFloat::CreateZero()), behaviorClass);
            }
            else
            {
                AZ_Assert(false, "Vector type unaccounted for in ScriptCanvas data model");
                return false;
            }
        }

        return true;
    }
    
    AZ_FORCE_INLINE bool ConvertImplicitlyCheckedVector3(const void* source, const Data::Type& targetType, AZStd::any& target, const AZ::BehaviorClass* targetClass)
    {
        const AZ::Vector3& sourceVector = *reinterpret_cast<const AZ::Vector3*>(source);

        if (Data::IsVectorType(targetType))
        {
            switch (targetType.GetType())
            {
            case Data::eType::Vector2:
                AZStd::any_cast<AZ::Vector2&>(target).Set(sourceVector.GetX(), sourceVector.GetY());
                break;

            case Data::eType::Vector3:
                AZStd::any_cast<AZ::Vector3&>(target) = sourceVector;
                break;

            case Data::eType::Vector4:
                AZStd::any_cast<AZ::Vector4&>(target).Set(sourceVector, AZ::VectorFloat::CreateZero());
                break;

            default:
                AZ_Assert(false, "Vector type unaccounted for in ScriptCanvas data model");
                return false;
            }
        }
        else
        {
            AZ_Assert(targetType.GetType() == Data::eType::BehaviorContextObject, "No other types are currently implicitly convertible");
            AZ_Assert(targetClass, "Target class unknown, no conversion possible");

            const AZ::BehaviorClass& behaviorClass = *targetClass;
            const AZ::Uuid& typeID = behaviorClass.m_typeId;

            if (typeID == azrtti_typeid<AZ::Vector3>())
            {
                target = BehaviorContextObject::Create<AZ::Vector3>(sourceVector, behaviorClass);
            }
            else if (typeID == azrtti_typeid<AZ::Vector2>())
            {
                target = BehaviorContextObject::Create<AZ::Vector2>(AZ::Vector2(static_cast<float>(sourceVector.GetX()), static_cast<float>(sourceVector.GetY())), behaviorClass);
            }
            else if (typeID == azrtti_typeid<AZ::Vector4>())
            {
                target = BehaviorContextObject::Create<AZ::Vector4>(AZ::Vector4::CreateFromVector3(sourceVector), behaviorClass);
            }
            else
            {
                AZ_Assert(false, "Vector type unaccounted for in ScriptCanvas data model");
                return false;
            }
        }

        return true;
    }

    AZ_FORCE_INLINE bool ConvertImplicitlyCheckedVector4(const void* source, const Data::Type& targetType, AZStd::any& target, const AZ::BehaviorClass* targetClass)
    {
        const AZ::Vector4& sourceVector = *reinterpret_cast<const AZ::Vector4*>(source);

        if (Data::IsVectorType(targetType))
        {
            switch (targetType.GetType())
            {
            case Data::eType::Vector2:
                AZStd::any_cast<AZ::Vector2&>(target).Set(sourceVector.GetX(), sourceVector.GetY());
                break;

            case Data::eType::Vector3:
                AZStd::any_cast<AZ::Vector3&>(target) = sourceVector.GetAsVector3();
                break;

            case Data::eType::Vector4:
                AZStd::any_cast<AZ::Vector4&>(target) = sourceVector;
                break;

            default:
                AZ_Assert(false, "Vector type unaccounted for in ScriptCanvas data model");
                return false;
            }
        }
        else
        {
            AZ_Assert(targetType.GetType() == Data::eType::BehaviorContextObject, "No other types are currently implicitly convertible");
            AZ_Assert(targetClass, "Target class unknown, no conversion possible");

            const AZ::BehaviorClass& behaviorClass = *targetClass;
            const AZ::Uuid& typeID = behaviorClass.m_typeId;

            if (typeID == azrtti_typeid<AZ::Vector3>())
            {
                target = BehaviorContextObject::Create<AZ::Vector3>(sourceVector.GetAsVector3(), behaviorClass);
            }
            else if (typeID == azrtti_typeid<AZ::Vector2>())
            {
                target = BehaviorContextObject::Create<AZ::Vector2>(AZ::Vector2(static_cast<float>(sourceVector.GetX()), static_cast<float>(sourceVector.GetY())), behaviorClass);
            }
            else if (typeID == azrtti_typeid<AZ::Vector4>())
            {
                target = BehaviorContextObject::Create<AZ::Vector4>(sourceVector, behaviorClass);
            }
            else
            {
                AZ_Assert(false, "Vector type unaccounted for in ScriptCanvas data model");
                return false;
            }
        }

        return true;
    }

    AZ_FORCE_INLINE bool IsAnyVectorType(const Data::Type& type)
    {
        return type.GetType() == Data::eType::BehaviorContextObject
            ? Data::IsVectorType(type.GetAZType())
            : Data::IsVectorType(type);
    }
    
    AZ_FORCE_INLINE Data::eType GetVectorType(const Data::Type& type)
    {
        return type.GetType() == Data::eType::BehaviorContextObject 
            ? Data::FromAZType(type.GetAZType()).GetType()
            : type.GetType();
    }

    AZ_FORCE_INLINE bool ConvertImplicitlyCheckedVector(const Data::Type& sourceType, const void* source, const Data::Type& targetType, AZStd::any& target, const AZ::BehaviorClass* targetClass)
    {
        const Data::eType sourceVectorType = GetVectorType(sourceType);
        
        switch (sourceVectorType)
        {
        case Data::eType::Vector2:
            return ConvertImplicitlyCheckedVector2(source, targetType, target, targetClass);
        case Data::eType::Vector3:
            return ConvertImplicitlyCheckedVector3(source, targetType, target, targetClass);
        case Data::eType::Vector4:
            return ConvertImplicitlyCheckedVector4(source, targetType, target, targetClass);
        default:
            AZ_Assert(false, "non vector type in conversion");
            return false;
        }
    }

    AZ_FORCE_INLINE Data::eType GetMathConversionType(const Data::Type& a, const Data::Type& b)
    {
        AZ_Assert
        (
            (a.GetType() == Data::eType::BehaviorContextObject && Data::IsAutoBoxedType(b))
            || 
            (b.GetType() == Data::eType::BehaviorContextObject && Data::IsAutoBoxedType(a))
        , "these types are not convertible, or need no conversion.");

        return a.GetType() == Data::eType::BehaviorContextObject ? b.GetType() : a.GetType();
    }

    AZ_FORCE_INLINE bool ConvertImplicitlyChecked(const Data::Type& sourceType, const void* source, const Data::Type& targetType, AZStd::any& target, const AZ::BehaviorClass* targetClass)
    {
        AZ_Assert(!targetType.IS_A(sourceType), "Bad use of conversion, target type IS-A source type");
        
        if (IsAnyVectorType(sourceType) && IsAnyVectorType(targetType))
        {
            return ConvertImplicitlyCheckedVector(sourceType, source, targetType, target, targetClass);
        } 
        else if (Data::IsConvertible(sourceType, targetType))
        {
            auto conversionType = GetMathConversionType(targetType, sourceType);
            
            switch (conversionType)
            {
            case Data::eType::AABB:
                return ConvertImplicitlyCheckedGeneric<Data::AABBType>(sourceType, source, targetType, target, targetClass);
            case Data::eType::Color:
                return ConvertImplicitlyCheckedGeneric<Data::ColorType>(sourceType, source, targetType, target, targetClass);
            case Data::eType::CRC:
                return ConvertImplicitlyCheckedGeneric<Data::CRCType>(sourceType, source, targetType, target, targetClass);
            case Data::eType::Matrix3x3:
                return ConvertImplicitlyCheckedGeneric<Data::Matrix3x3Type>(sourceType, source, targetType, target, targetClass);
            case Data::eType::Matrix4x4:
                return ConvertImplicitlyCheckedGeneric<Data::Matrix4x4Type>(sourceType, source, targetType, target, targetClass);
            case Data::eType::OBB:
                return ConvertImplicitlyCheckedGeneric<Data::OBBType>(sourceType, source, targetType, target, targetClass);
            case Data::eType::Plane:
                return ConvertImplicitlyCheckedGeneric<Data::AABBType>(sourceType, source, targetType, target, targetClass);
            case Data::eType::Transform:
                return ConvertImplicitlyCheckedGeneric<Data::TransformType>(sourceType, source, targetType, target, targetClass);
            case Data::eType::Rotation:
                return ConvertImplicitlyCheckedGeneric<Data::RotationType>(sourceType, source, targetType, target, targetClass);
            default:
                AZ_Assert(false, "unsupported convertible type added");
            }
        }

        return false;
    }
            
    template<typename t_Value>
    AZ_INLINE bool FromBehaviorContext(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        if (typeID == azrtti_typeid<t_Value>())
        {
            destination = *reinterpret_cast<const t_Value*>(source);
            return true;
        }
        else
        {
            AZ_Error("Script Canvas", false, "FromBehaviorContext generic failed on type match");
        }

        return false;
    }
    
    AZ_INLINE bool FromBehaviorContextAABB(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<Data::AABBType>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextBool(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<bool>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextColor(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<Data::ColorType>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextCRC(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<Data::CRCType>(typeID, source, destination);
    }
    
    AZ_INLINE bool FromBehaviorContextEntityID(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<AZ::EntityId>(typeID, source, destination);
    }

    template<typename t_Value>
    AZ_INLINE bool FromBehaviorContextNumeric(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        if (typeID == azrtti_typeid<t_Value>())
        {
            Data::NumberType number = aznumeric_caster(*reinterpret_cast<const t_Value*>(source));
            destination = number;
            return true;
        }

        return false;
    }

    // special cases NumberType, to prevent compiler errors on unecessary casting
    template<>
    AZ_INLINE bool FromBehaviorContextNumeric<Data::NumberType>(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        if (typeID == azrtti_typeid<Data::NumberType>())
        {
            destination = *reinterpret_cast<const Data::NumberType*>(source);
            return true;
        }

        return false;
    }

    // special cases conversion from vector float, which is non-trivial
    template<>
    AZ_INLINE bool FromBehaviorContextNumeric<AZ::VectorFloat>(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        if (typeID == azrtti_typeid<AZ::VectorFloat>())
        {
            float firstConversion;
            reinterpret_cast<const AZ::VectorFloat*>(source)->StoreToFloat(&firstConversion);
            Data::NumberType secondConversion = aznumeric_caster(firstConversion);
            destination = secondConversion;
            return true;
        }

        return false;
    }

    AZ_INLINE bool FromBehaviorContextNumber(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        AZ_Assert(source, "bad source in FromBehaviorContextNumber");
        return FromBehaviorContextNumeric<char>(typeID, source, destination)
            || FromBehaviorContextNumeric<short>(typeID, source, destination)
            || FromBehaviorContextNumeric<int>(typeID, source, destination)
            || FromBehaviorContextNumeric<long>(typeID, source, destination)
            || FromBehaviorContextNumeric<AZ::s8>(typeID, source, destination)
            || FromBehaviorContextNumeric<AZ::s64>(typeID, source, destination)
            || FromBehaviorContextNumeric<unsigned char>(typeID, source, destination)
            || FromBehaviorContextNumeric<unsigned int>(typeID, source, destination)
            || FromBehaviorContextNumeric<unsigned long>(typeID, source, destination)
            || FromBehaviorContextNumeric<unsigned short>(typeID, source, destination)
            || FromBehaviorContextNumeric<AZ::u64>(typeID, source, destination)
            || FromBehaviorContextNumeric<float>(typeID, source, destination)
            || FromBehaviorContextNumeric<double>(typeID, source, destination)
            || FromBehaviorContextNumeric<AZ::VectorFloat>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextMatrix3x3(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<AZ::Matrix3x3>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextMatrix4x4(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<AZ::Matrix4x4>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextOBB(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<Data::OBBType>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextPlane(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<Data::PlaneType>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextRotation(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<ScriptCanvas::Data::RotationType>(typeID, source, destination);
    }

    AZ_INLINE bool FromBehaviorContextTransform(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<ScriptCanvas::Data::TransformType>(typeID, source, destination);
    }
    
    AZ_INLINE bool FromBehaviorContextVector2(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        AZ::Vector2* target = AZStd::any_cast<AZ::Vector2>(&destination);
        AZ_Assert(source, "bad source in FromBehaviorContextVector");

        if (typeID == azrtti_typeid<AZ::Vector3>())
        {
            AZ::Vector3 sourceVector3(*reinterpret_cast<const AZ::Vector3*>(source));
            target->SetX(sourceVector3.GetX());
            target->SetY(sourceVector3.GetY());
            return true;
        }
        else if (typeID == azrtti_typeid<AZ::Vector2>())
        {
            *target = *reinterpret_cast<const AZ::Vector2*>(source);
            return true;
        }
        else if (typeID == azrtti_typeid<AZ::Vector4>())
        {
            AZ::Vector4 sourceVector4(*reinterpret_cast<const AZ::Vector4*>(source));
            target->SetX(sourceVector4.GetX());
            target->SetY(sourceVector4.GetY());
            return true;
        }
        else
        {
            return false;
        }
    }

    AZ_INLINE bool FromBehaviorContextVector3(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        AZ::Vector3* target = AZStd::any_cast<AZ::Vector3>(&destination);
        AZ_Assert(source, "bad source in FromBehaviorContextVector");

        if (typeID == azrtti_typeid<AZ::Vector3>())
        {
            *target = *reinterpret_cast<const AZ::Vector3*>(source);
            return true;
        }
        else if (typeID == azrtti_typeid<AZ::Vector2>())
        {
            auto vector2 = reinterpret_cast<const AZ::Vector2*>(source);
            target->Set(vector2->GetX(), vector2->GetY(), AZ::VectorFloat::CreateZero());
            return true;
        }
        else if (typeID == azrtti_typeid<AZ::Vector4>())
        {
            *target = reinterpret_cast<const AZ::Vector4*>(source)->GetAsVector3();
            return true;
        }
        else
        {
            return false;
        }
    }

    AZ_INLINE bool FromBehaviorContextVector4(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        AZ::Vector4* target = AZStd::any_cast<AZ::Vector4>(&destination);
        AZ_Assert(source, "bad source in FromBehaviorContextVector");

        if (typeID == azrtti_typeid<AZ::Vector3>())
        {
            *target = AZ::Vector4::CreateFromVector3(*reinterpret_cast<const AZ::Vector3*>(source));
            return true;
        }
        else if (typeID == azrtti_typeid<AZ::Vector2>())
        {
            auto vector2 = reinterpret_cast<const AZ::Vector2*>(source);
            target->Set(vector2->GetX(), vector2->GetY(), AZ::VectorFloat::CreateZero(), AZ::VectorFloat::CreateZero());
            return true;
        }
        else if (typeID == azrtti_typeid<AZ::Vector4>())
        {
            *target = *reinterpret_cast<const AZ::Vector4*>(source);
            return true;
        }
        else
        {
            return false;
        }
    }

    AZ_INLINE bool FromBehaviorContextString(const AZ::Uuid& typeID, const void* source, AZStd::any& destination)
    {
        return FromBehaviorContext<ScriptCanvas::Data::StringType>(typeID, source, destination);
    }
    
    template<typename t_Value>
    AZ_INLINE bool IsDataEqual(const void* lhs, const void* rhs)
    {
        return *reinterpret_cast<const t_Value*>(lhs) == *reinterpret_cast<const t_Value*>(rhs);
    }

    AZ_INLINE bool IsDataEqual(const Data::Type& type, const void* lhs, const void* rhs)
    {
        switch (type.GetType())
        {
        case Data::eType::AABB:
            return IsDataEqual<Data::AABBType>(lhs, rhs);

        case Data::eType::BehaviorContextObject:
            AZ_Error("ScriptCanvas", false, "BehaviorContextObject passed into IsDataEqual, which is invalid, an attempt must be made to call the behavior method");
            return false;

        case Data::eType::Boolean:
            return IsDataEqual<Data::BooleanType>(lhs, rhs);

        case Data::eType::Color:
            return IsDataEqual<Data::ColorType>(lhs, rhs);

        case Data::eType::CRC:
            return IsDataEqual<Data::CRCType>(lhs, rhs);

        case Data::eType::EntityID:
            return IsDataEqual<Data::EntityIDType>(lhs, rhs);

        case Data::eType::Invalid:
            return false;

        case Data::eType::Matrix3x3:
            return IsDataEqual<Data::Matrix3x3Type>(lhs, rhs);

        case Data::eType::Matrix4x4:
            return IsDataEqual<Data::Matrix4x4Type>(lhs, rhs);

        case Data::eType::Number:
            return IsDataEqual<Data::NumberType>(lhs, rhs);

        case Data::eType::OBB:
            return IsDataEqual<Data::OBBType>(lhs, rhs);

        case Data::eType::Plane:
            return IsDataEqual<Data::PlaneType>(lhs, rhs);

        case Data::eType::Rotation:
            return IsDataEqual<Data::RotationType>(lhs, rhs);

        case Data::eType::String:
            return IsDataEqual<Data::StringType>(lhs, rhs);

        case Data::eType::Transform:
            return IsDataEqual<Data::TransformType>(lhs, rhs);

        case Data::eType::Vector2:
            return IsDataEqual<Data::Vector2Type>(lhs, rhs);

        case Data::eType::Vector3:
            return IsDataEqual<Data::Vector3Type>(lhs, rhs);

        case Data::eType::Vector4:
            return IsDataEqual<Data::Vector4Type>(lhs, rhs);

        default:
            AZ_Assert(false, "unsupported type found in IsDataEqual");
            return false;
        }
    }

    template<typename t_Value>
    AZ_INLINE bool IsDataLess(const void* lhs, const void* rhs)
    {
        return *reinterpret_cast<const t_Value*>(lhs) < *reinterpret_cast<const t_Value*>(rhs);
    }

    AZ_INLINE bool IsDataLess(const Data::Type& type, const void* lhs, const void* rhs)
    {
        switch (type.GetType())
        {
        case Data::eType::BehaviorContextObject:
            AZ_Error("ScriptCanvas", false, "BehaviorContextObject passed into IsDataLess, which is invalid, an attempt must be made to call the behavior method");
            return false;

        case Data::eType::Number:
            return IsDataLess<Data::NumberType>(lhs, rhs);

        case Data::eType::Vector2:
            return reinterpret_cast<const Data::Vector2Type*>(lhs)->IsLessThan(*reinterpret_cast<const Data::Vector2Type*>(rhs));

        case Data::eType::Vector3:
            return reinterpret_cast<const Data::Vector3Type*>(lhs)->IsLessThan(*reinterpret_cast<const Data::Vector3Type*>(rhs));

        case Data::eType::Vector4:
            return reinterpret_cast<const Data::Vector4Type*>(lhs)->IsLessThan(*reinterpret_cast<const Data::Vector4Type*>(rhs));

        case Data::eType::Boolean:
            return IsDataLess<Data::BooleanType>(lhs, rhs);

        case Data::eType::AABB:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::AABBType>::GetName());
            return false;

        case Data::eType::OBB:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::OBBType>::GetName());
            return false;

        case Data::eType::Plane:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::PlaneType>::GetName());
            return false;

        case Data::eType::Rotation:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::RotationType>::GetName());
            return false;

        case Data::eType::String:
            return IsDataLess<Data::StringType>(lhs, rhs);

        case Data::eType::Transform:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::TransformType>::GetName());
            return false;

        case Data::eType::Color:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::ColorType>::GetName());
            return false;

        case Data::eType::CRC:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::CRCType>::GetName());
            return false;

        case Data::eType::EntityID:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::EntityIDType>::GetName());
            return false;

        case Data::eType::Matrix3x3:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::Matrix3x3Type>::GetName());
            return false;

        case Data::eType::Matrix4x4:
            AZ_Error("ScriptCanvas", false, "No Less operator exists for type: %s", Data::Traits<Data::Matrix4x4Type>::GetName());
            return false;

        case Data::eType::Invalid:
            return false;

        default:
            AZ_Assert(false, "unsupported type found in IsDataLess");
            return false;
        }
    }

    template<typename t_Value>
    AZ_INLINE bool IsDataLessEqual(const void* lhs, const void* rhs)
    {
        return *reinterpret_cast<const t_Value*>(lhs) <= *reinterpret_cast<const t_Value*>(rhs);
    }

    AZ_INLINE bool IsDataLessEqual(const Data::Type& type, const void* lhs, const void* rhs)
    {
        switch (type.GetType())
        {
        case Data::eType::BehaviorContextObject:
            AZ_Error("ScriptCanvas", false, "BehaviorContextObject passed into IsDataLessEqual, which is invalid, an attempt must be made to call the behavior method");
            return false;

        case Data::eType::Number:
            return IsDataLessEqual<Data::NumberType>(lhs, rhs);

        case Data::eType::Vector2:
            return reinterpret_cast<const Data::Vector2Type*>(lhs)->IsLessEqualThan(*reinterpret_cast<const Data::Vector2Type*>(rhs));

        case Data::eType::Vector3:
            return reinterpret_cast<const Data::Vector3Type*>(lhs)->IsLessEqualThan(*reinterpret_cast<const Data::Vector3Type*>(rhs));

        case Data::eType::Vector4:
            return reinterpret_cast<const Data::Vector4Type*>(lhs)->IsLessEqualThan(*reinterpret_cast<const Data::Vector4Type*>(rhs));

        case Data::eType::Boolean:
            return IsDataLessEqual<Data::BooleanType>(lhs, rhs);

        case Data::eType::AABB:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::AABBType>::GetName());
            return false;

        case Data::eType::OBB:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::OBBType>::GetName());
            return false;

        case Data::eType::Plane:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::PlaneType>::GetName());
            return false;

        case Data::eType::Rotation:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::RotationType>::GetName());
            return false;

        case Data::eType::String:
            return IsDataLessEqual<Data::StringType>(lhs, rhs);

        case Data::eType::Transform:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::TransformType>::GetName());
            return false;

        case Data::eType::Color:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::ColorType>::GetName());
            return false;

        case Data::eType::CRC:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::CRCType>::GetName());
            return false;

        case Data::eType::EntityID:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::EntityIDType>::GetName());
            return false;

        case Data::eType::Matrix3x3:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::Matrix3x3Type>::GetName());
            return false;

        case Data::eType::Matrix4x4:
            AZ_Error("ScriptCanvas", false, "No LessEqual operator exists for type: %s", Data::Traits<Data::Matrix4x4Type>::GetName());
            return false;

        case Data::eType::Invalid:
            return false;

        default:
            AZ_Assert(false, "unsupported type found in IsDataLessEqual");
            return false;
        }
    }

    template<typename t_Value>
    AZ_INLINE bool IsDataGreater(const void* lhs, const void* rhs)
    {
        return *reinterpret_cast<const t_Value*>(lhs) > *reinterpret_cast<const t_Value*>(rhs);
    }

    AZ_INLINE bool IsDataGreater(const Data::Type& type, const void* lhs, const void* rhs)
    {
        switch (type.GetType())
        {
        case Data::eType::BehaviorContextObject:
            AZ_Error("ScriptCanvas", false, "BehaviorContextObject passed into IsDataGreater, which is invalid, an attempt must be made to call the behavior method");
            return false;

        case Data::eType::Number:
            return IsDataGreater<Data::NumberType>(lhs, rhs);

        case Data::eType::Vector2:
            return reinterpret_cast<const Data::Vector2Type*>(lhs)->IsGreaterThan(*reinterpret_cast<const Data::Vector2Type*>(rhs));

        case Data::eType::Vector3:
            return reinterpret_cast<const Data::Vector3Type*>(lhs)->IsGreaterThan(*reinterpret_cast<const Data::Vector3Type*>(rhs));

        case Data::eType::Vector4:
            return reinterpret_cast<const Data::Vector4Type*>(lhs)->IsGreaterThan(*reinterpret_cast<const Data::Vector4Type*>(rhs));

        case Data::eType::Boolean:
            return IsDataGreater<Data::BooleanType>(lhs, rhs);

        case Data::eType::AABB:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::AABBType>::GetName());
            return false;

        case Data::eType::OBB:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::OBBType>::GetName());
            return false;

        case Data::eType::Plane:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::PlaneType>::GetName());
            return false;

        case Data::eType::Rotation:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::RotationType>::GetName());
            return false;

        case Data::eType::String:
            return IsDataGreater<Data::StringType>(lhs, rhs);

        case Data::eType::Transform:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::TransformType>::GetName());
            return false;

        case Data::eType::Color:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::ColorType>::GetName());
            return false;

        case Data::eType::CRC:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::CRCType>::GetName());
            return false;

        case Data::eType::EntityID:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::EntityIDType>::GetName());
            return false;

        case Data::eType::Matrix3x3:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::Matrix3x3Type>::GetName());
            return false;

        case Data::eType::Matrix4x4:
            AZ_Error("ScriptCanvas", false, "No Greater operator exists for type: %s", Data::Traits<Data::Matrix4x4Type>::GetName());
            return false;

        case Data::eType::Invalid:
            return false;

        default:
            AZ_Assert(false, "unsupported type found in IsDataGreater");
            return false;
        }
    }

    template<typename t_Value>
    AZ_INLINE bool IsDataGreaterEqual(const void* lhs, const void* rhs)
    {
        return *reinterpret_cast<const t_Value*>(lhs) >= *reinterpret_cast<const t_Value*>(rhs);
    }

    AZ_INLINE bool IsDataGreaterEqual(const Data::Type& type, const void* lhs, const void* rhs)
    {
        switch (type.GetType())
        {
            AZ_Error("ScriptCanvas", false, "BehaviorContextObject passed into IsDataGreater, which is invalid, an attempt must be made to call the behavior method");
            return false;

        case Data::eType::Number:
            return IsDataGreaterEqual<Data::NumberType>(lhs, rhs);

        case Data::eType::Vector2:
            return reinterpret_cast<const Data::Vector2Type*>(lhs)->IsGreaterEqualThan(*reinterpret_cast<const Data::Vector2Type*>(rhs));

        case Data::eType::Vector3:
            return reinterpret_cast<const Data::Vector3Type*>(lhs)->IsGreaterEqualThan(*reinterpret_cast<const Data::Vector3Type*>(rhs));

        case Data::eType::Vector4:
            return reinterpret_cast<const Data::Vector4Type*>(lhs)->IsGreaterEqualThan(*reinterpret_cast<const Data::Vector4Type*>(rhs));

        case Data::eType::Boolean:
            return IsDataGreaterEqual<Data::BooleanType>(lhs, rhs);

        case Data::eType::AABB:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::AABBType>::GetName());
            return false;

        case Data::eType::OBB:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::OBBType>::GetName());
            return false;

        case Data::eType::Plane:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::PlaneType>::GetName());
            return false;

        case Data::eType::Rotation:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::RotationType>::GetName());
            return false;

        case Data::eType::String:
            return IsDataGreaterEqual<Data::StringType>(lhs, rhs);

        case Data::eType::Transform:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::TransformType>::GetName());
            return false;

        case Data::eType::Color:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::ColorType>::GetName());
            return false;

        case Data::eType::CRC:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::CRCType>::GetName());
            return false;

        case Data::eType::EntityID:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::EntityIDType>::GetName());
            return false;

        case Data::eType::Matrix3x3:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::Matrix3x3Type>::GetName());
            return false;

        case Data::eType::Matrix4x4:
            AZ_Error("ScriptCanvas", false, "No GreaterEqual operator exists for type: %s", Data::Traits<Data::Matrix4x4Type>::GetName());
            return false;

        case Data::eType::Invalid:
            return false;

        default:
            AZ_Assert(false, "unsupported type found in IsDataGreaterEqual");
            return false;
        }
    }

    template<typename t_Value>
    AZ_INLINE bool IsDataNotEqual(const void* lhs, const void* rhs)
    {
        return *reinterpret_cast<const t_Value*>(lhs) != *reinterpret_cast<const t_Value*>(rhs);
    }

    AZ_INLINE bool IsDataNotEqual(const Data::Type& type, const void* lhs, const void* rhs)
    {
        switch (type.GetType())
        {
        case Data::eType::AABB:
            return IsDataNotEqual<Data::AABBType>(lhs, rhs);

        case Data::eType::BehaviorContextObject:
            return lhs != rhs;

        case Data::eType::Boolean:
            return IsDataNotEqual<Data::BooleanType>(lhs, rhs);

        case Data::eType::Color:
            return IsDataNotEqual<Data::ColorType>(lhs, rhs);

        case Data::eType::CRC:
            return IsDataNotEqual<Data::CRCType>(lhs, rhs);

        case Data::eType::EntityID:
            return IsDataNotEqual<Data::EntityIDType>(lhs, rhs);

        case Data::eType::Invalid:
            return false;

        case Data::eType::Matrix3x3:
            return IsDataNotEqual<Data::Matrix3x3Type>(lhs, rhs);

        case Data::eType::Matrix4x4:
            return IsDataNotEqual<Data::Matrix4x4Type>(lhs, rhs);

        case Data::eType::Number:
            return IsDataNotEqual<Data::NumberType>(lhs, rhs);

        case Data::eType::OBB:
            return IsDataNotEqual<Data::OBBType>(lhs, rhs);

        case Data::eType::Plane:
            return IsDataNotEqual<Data::PlaneType>(lhs, rhs);

        case Data::eType::Rotation:
            return IsDataNotEqual<Data::RotationType>(lhs, rhs);

        case Data::eType::String:
            return IsDataNotEqual<Data::StringType>(lhs, rhs);

        case Data::eType::Transform:
            return IsDataNotEqual<Data::TransformType>(lhs, rhs);

        case Data::eType::Vector2:
            return IsDataNotEqual<Data::Vector2Type>(lhs, rhs);

        case Data::eType::Vector3:
            return IsDataNotEqual<Data::Vector3Type>(lhs, rhs);

        case Data::eType::Vector4:
            return IsDataNotEqual<Data::Vector4Type>(lhs, rhs);

        default:
            AZ_Assert(false, "unsupported type found in IsDataNotEqual");
            return false;
        }
    }
    template<typename t_Value>
    AZ_INLINE bool ToBehaviorContext(AZStd::any& valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        if (typeIDOut == azrtti_typeid<t_Value>())
        {
            valueOut = *reinterpret_cast<const t_Value*>(valueIn);
            return true;
        }

        return false;
    }

    template<typename t_Value>
    AZ_INLINE bool ToBehaviorContextNumeric(AZStd::any& valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        if (typeIDOut == azrtti_typeid<t_Value>())
        {
            const t_Value value = aznumeric_caster(*reinterpret_cast<const Data::NumberType*>(valueIn));
            valueOut = value;
            return true;
        }

        return false;
    }

    template<>
    AZ_INLINE bool ToBehaviorContextNumeric<Data::NumberType>(AZStd::any& valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<Data::NumberType>(valueOut, typeIDOut, valueIn);
    }

    template<>
    AZ_INLINE bool ToBehaviorContextNumeric<AZ::VectorFloat>(AZStd::any& valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        if (typeIDOut == azrtti_typeid<AZ::VectorFloat>())
        {
            float firstConversion = aznumeric_caster(*reinterpret_cast<const Data::NumberType*>(valueIn));
            valueOut = AZ::VectorFloat::CreateFromFloat(&firstConversion);
            return true;
        }

        return false;
    }

    AZ_INLINE bool ToBehaviorContextNumber(AZStd::any& valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return valueIn && (ToBehaviorContextNumeric<char>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<short>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<int>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<long>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<AZ::s8>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<AZ::s64>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<unsigned char>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<unsigned int>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<unsigned long>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<unsigned short>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<AZ::u64>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<float>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<double>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<AZ::VectorFloat>(valueOut, typeIDOut, valueIn));
    }

    template<typename t_Value>
    AZ_INLINE bool ToBehaviorContext(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        if (typeIDOut == azrtti_typeid<t_Value>())
        {
            *reinterpret_cast<t_Value*>(valueOut) = *reinterpret_cast<const t_Value*>(valueIn);
            return true;
        }

        return false;
    }

    AZ_INLINE bool ToBehaviorContextAABB(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<Data::AABBType>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextBool(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<bool>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextColor(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<Data::ColorType>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextCRC(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<Data::CRCType>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextEntityID(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<AZ::EntityId>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextMatrix3x3(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<AZ::Matrix3x3>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextMatrix4x4(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<AZ::Matrix4x4>(valueOut, typeIDOut, valueIn);
    }

    template<typename t_Value>
    AZ_INLINE bool ToBehaviorContextNumeric(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        if (typeIDOut == azrtti_typeid<t_Value>())
        {
            *reinterpret_cast<t_Value*>(valueOut) = aznumeric_caster(*reinterpret_cast<const Data::NumberType*>(valueIn));
            return true;
        }

        return false;
    }

    template<>
    AZ_INLINE bool ToBehaviorContextNumeric<Data::NumberType>(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<Data::NumberType>(valueOut, typeIDOut, valueIn);
    }

    template<>
    AZ_INLINE bool ToBehaviorContextNumeric<AZ::VectorFloat>(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        if (typeIDOut == azrtti_typeid<AZ::VectorFloat>())
        {
            float firstConversion = aznumeric_caster(*reinterpret_cast<const Data::NumberType*>(valueIn));
            *reinterpret_cast<AZ::VectorFloat*>(valueOut) = AZ::VectorFloat::CreateFromFloat(&firstConversion);
            return true;;
        }

        return false;
    }

    AZ_INLINE bool ToBehaviorContextNumber(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return valueIn && (ToBehaviorContextNumeric<char>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<short>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<int>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<long>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<AZ::s8>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<AZ::s64>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<unsigned char>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<unsigned int>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<unsigned long>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<unsigned short>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<AZ::u64>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<float>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<double>(valueOut, typeIDOut, valueIn)
            || ToBehaviorContextNumeric<AZ::VectorFloat>(valueOut, typeIDOut, valueIn));
    }

    AZ_INLINE bool ToBehaviorContextOBB(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<Data::OBBType>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextObject(const AZ::BehaviorClass* behaviorClass, void* valueOut, const void* valueIn)
    {
        if (behaviorClass && behaviorClass->m_cloner)
        {
            behaviorClass->m_cloner(valueOut, valueIn, nullptr);
            return true;
        }

        return false;
    }

    AZ_INLINE bool ToBehaviorContextPlane(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<Data::PlaneType>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextRotation(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<ScriptCanvas::Data::RotationType>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextString(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<ScriptCanvas::Data::StringType>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextTransform(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        return ToBehaviorContext<ScriptCanvas::Data::TransformType>(valueOut, typeIDOut, valueIn);
    }

    AZ_INLINE bool ToBehaviorContextVector2(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        const AZ::Vector2* vector2in(reinterpret_cast<const AZ::Vector2*>(valueIn));

        if (typeIDOut == azrtti_typeid<AZ::Vector3>())
        {
            AZ::Vector3* vector3out = reinterpret_cast<AZ::Vector3*>(valueOut);
            vector3out->SetX(vector2in->GetX());
            vector3out->SetY(vector2in->GetY());
            return true;
        }
        else if (typeIDOut == azrtti_typeid<AZ::Vector2>())
        {
            *reinterpret_cast<AZ::Vector2*>(valueOut) = *vector2in;
            return true;
        }
        else if (typeIDOut == azrtti_typeid<AZ::Vector4>())
        {
            AZ::Vector4* vector4out = reinterpret_cast<AZ::Vector4*>(valueOut);
            vector4out->SetX(vector2in->GetX());
            vector4out->SetY(vector2in->GetY());
            return true;
        }
        
        return false;
    }

    AZ_INLINE bool ToBehaviorContextVector3(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        const AZ::Vector3* vector3in(reinterpret_cast<const AZ::Vector3*>(valueIn));

        if (typeIDOut == azrtti_typeid<AZ::Vector3>())
        {
            *reinterpret_cast<AZ::Vector3*>(valueOut) = *vector3in;
            return true;
        }
        else if (typeIDOut == azrtti_typeid<AZ::Vector2>())
        {
            reinterpret_cast<AZ::Vector2*>(valueOut)->Set(vector3in->GetX(), vector3in->GetY());
            return true;
        }
        else if (typeIDOut == azrtti_typeid<AZ::Vector4>())
        {
            *reinterpret_cast<AZ::Vector4*>(valueOut) = AZ::Vector4::CreateFromVector3(*vector3in);
            return true;
        }
        
        return false;
    }

    AZ_INLINE bool ToBehaviorContextVector4(void* valueOut, const AZ::Uuid& typeIDOut, const void* valueIn)
    {
        const AZ::Vector4* vector4in(reinterpret_cast<const AZ::Vector4*>(valueIn));

        if (typeIDOut == azrtti_typeid<AZ::Vector3>())
        {
            *reinterpret_cast<AZ::Vector3*>(valueOut) = vector4in->GetAsVector3();
            return true;
        }
        else if (typeIDOut == azrtti_typeid<AZ::Vector2>())
        {
            reinterpret_cast<AZ::Vector2*>(valueOut)->Set(vector4in->GetX(), vector4in->GetY());
            return true;
        }
        else if (typeIDOut == azrtti_typeid<AZ::Vector4>())
        {
            *reinterpret_cast<AZ::Vector4*>(valueOut) = *vector4in;
            return true;
        }
        
        return false;
    }

    bool ToBehaviorContext(const ScriptCanvas::Data::Type& typeIn, const void* valueIn, const AZ::Uuid& typeIDOut, void* valueOut, AZ::BehaviorClass* behaviorClassOut)
    {
        if (valueIn)
        {
            switch (typeIn.GetType())
            {
            case ScriptCanvas::Data::eType::AABB:
                return ToBehaviorContextAABB(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::BehaviorContextObject:
                return ToBehaviorContextObject(behaviorClassOut, valueOut, valueIn);

            case ScriptCanvas::Data::eType::Boolean:
                return ToBehaviorContextBool(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Color:
                return ToBehaviorContextColor(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::CRC:
                return ToBehaviorContextCRC(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::EntityID:
                return ToBehaviorContextEntityID(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Matrix3x3:
                return ToBehaviorContextMatrix3x3(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Matrix4x4:
                return ToBehaviorContextMatrix4x4(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Number:
                return ToBehaviorContextNumber(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::OBB:
                return ToBehaviorContextOBB(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Plane:
                return ToBehaviorContextPlane(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Rotation:
                return ToBehaviorContextRotation(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::String:
                return ToBehaviorContextString(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Transform:
                return ToBehaviorContextTransform(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Vector2:
                return ToBehaviorContextVector2(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Vector3:
                return ToBehaviorContextVector3(valueOut, typeIDOut, valueIn);

            case ScriptCanvas::Data::eType::Vector4:
                return ToBehaviorContextVector4(valueOut, typeIDOut, valueIn);
            }
        }

        AZ_Error("Script Canvas", false, "invalid object going from Script Canvas!");
        return false;
    }

    AZ::BehaviorValueParameter ConvertibleToBehaviorValueParameter(const AZ::BehaviorParameter& description, const AZ::Uuid& azType, AZ::BehaviorClass* behaviorClass, void* value, void*& pointer, AZ::IRttiHelper* azRtti = nullptr)
    {
        AZ_Assert(value, "value must be valid");
        AZ::BehaviorValueParameter parameter;
        parameter.m_typeId = description.m_typeId;
        parameter.m_name = behaviorClass ? behaviorClass->m_name.c_str() : Data::GetBehaviorContextName(azType);
        parameter.m_azRtti = behaviorClass ? behaviorClass->m_azRtti : azRtti;

        if (description.m_traits & AZ::BehaviorParameter::TR_POINTER)
        {
            pointer = value;
            parameter.m_value = &pointer;
            parameter.m_traits = AZ::BehaviorParameter::TR_POINTER;
        }
        else
        {
            parameter.m_value = value;
            parameter.m_traits = 0;
        }

        return parameter;
    }

    AZ::Outcome<Data::StringType, AZStd::string> ConvertBehaviorContextString(const AZ::BehaviorParameter& parameterDesc, const void* source)
    {
        if (!source)
        {
            return AZ::Success(AZStd::string());
        }

        if (parameterDesc.m_typeId == azrtti_typeid<char>() && (parameterDesc.m_traits | (AZ::BehaviorParameter::TR_POINTER & AZ::BehaviorParameter::TR_CONST)))
        {
            AZStd::string_view parameterString = *reinterpret_cast<const char* const *>(source);
            return AZ::Success(AZStd::string(parameterString));
        }
        else if (parameterDesc.m_typeId == azrtti_typeid<AZStd::string_view>())
        {
            const AZStd::string_view* parameterString = nullptr;
            if (parameterDesc.m_traits & AZ::BehaviorParameter::TR_POINTER)
            {
                parameterString = *reinterpret_cast<const AZStd::string_view* const *>(source);
            }
            else
            {
                parameterString = reinterpret_cast<const AZStd::string_view*>(source);
            }

            if (parameterString)
            {
                return AZ::Success(AZStd::string(*parameterString));
            }
        }
        return AZ::Failure(AZStd::string("Cannot convert BehaviorContext String type to Script Canvas String"));
    }
} // namespace anonymous

namespace ScriptCanvas
{
    void Datum::InitializeLabel()
    {
        m_datumElementData.m_name = "Datum"; // This field is mandatory.
        m_datumElementData.m_attributes.resize(2);
        m_datumElementData.m_attributes[0] = { AZ::Edit::Attributes::NameLabelOverride, &m_datumElementDataAttributeLabel };
        m_datumElementData.m_attributes[1] = { AZ::Edit::Attributes::Visibility, &m_datumElementDataAttributeVisibility };
    }

    Datum::Datum()
    : m_datumElementDataAttributeLabel("")
    , m_datumElementDataAttributeVisibility(AZ::Edit::PropertyVisibility::Show)
    {
        InitializeLabel();
    }

    Datum::Datum(const bool isUntyped)
        : m_isUntypedStorage(true)
        , m_originality(eOriginality::Copy)
        , m_datumElementDataAttributeLabel("")
        , m_datumElementDataAttributeVisibility(AZ::Edit::PropertyVisibility::Show)
    {
        InitializeLabel();
    }

    Datum::Datum(const Datum& datum)
        : m_isUntypedStorage(true)
        , m_datumElementDataAttributeLabel("")
        , m_datumElementDataAttributeVisibility(AZ::Edit::PropertyVisibility::Show)
    {
        *this = datum;
        const_cast<bool&>(m_isUntypedStorage) = datum.m_isUntypedStorage;

        InitializeLabel();
        m_datumElementDataAttributeLabel = datum.m_datumElementDataAttributeLabel.Get(nullptr);
        m_datumElementDataAttributeVisibility = datum.m_datumElementDataAttributeVisibility.Get(nullptr);
    }

    Datum::Datum(Datum&& datum)
        : m_isUntypedStorage(true)
        , m_datumElementDataAttributeLabel("")
        , m_datumElementDataAttributeVisibility(AZ::Edit::PropertyVisibility::Show)
    {
        *this = AZStd::move(datum);
        const_cast<bool&>(m_isUntypedStorage) = AZStd::move(datum.m_isUntypedStorage);

        InitializeLabel();
        m_datumElementDataAttributeLabel = datum.m_datumElementDataAttributeLabel.Get(nullptr);
        m_datumElementDataAttributeVisibility = datum.m_datumElementDataAttributeVisibility.Get(nullptr);
    }

    Datum::Datum(const Data::Type& type, eOriginality originality)
        : Datum(type, originality, nullptr, AZ::Uuid::CreateNull())
    {
    }

    Datum::Datum(const Data::Type& type, eOriginality originality, const void* source, const AZ::Uuid& sourceTypeID)
        : m_datumElementDataAttributeLabel("")
        , m_datumElementDataAttributeVisibility(AZ::Edit::PropertyVisibility::Show)
    {
        InitializeLabel();
        Initialize(type, originality, source, sourceTypeID);
    }
        
    Datum::Datum(const AZStd::string& behaviorClassName, eOriginality originality)
        : Datum(Data::FromAZType(AZ::BehaviorContextHelper::GetClassType(behaviorClassName)), originality, nullptr, AZ::Uuid::CreateNull())
    {
    }

    Datum::Datum(const AZ::BehaviorParameter& parameterDesc, eOriginality originality, const void* source)
        : m_datumElementDataAttributeLabel("")
        , m_datumElementDataAttributeVisibility(AZ::Edit::PropertyVisibility::Show)
    {
        InitializeLabel();
        InitializeBehaviorContextParameter(parameterDesc, originality, source);
    }

    ComparisonOutcome Datum::CallComparisonOperator(AZ::Script::Attributes::OperatorType operatorType, const AZ::BehaviorClass& behaviorClass, const Datum& lhs, const Datum& rhs)
    {
        // depending on when this gets called, check for null operands, they could be possible
        for (auto& methodIt : behaviorClass.m_methods)
        {
            auto* method = methodIt.second;

            if (AZ::Attribute* operatorAttr = AZ::FindAttribute(AZ::Script::Attributes::Operator, method->m_attributes))
            {
                AZ::AttributeReader operatorAttrReader(nullptr, operatorAttr);
                AZ::Script::Attributes::OperatorType methodAttribute{};
                
                if (operatorAttrReader.Read<AZ::Script::Attributes::OperatorType>(methodAttribute)
                    && methodAttribute == operatorType
                    && method->HasResult()
                    && method->GetResult()->m_typeId == azrtti_typeid<bool>()
                    && method->GetNumArguments() == 2)
                {
                    bool comparisonResult{};
                    AZ::BehaviorValueParameter result(&comparisonResult);
                    AZStd::array<AZ::BehaviorValueParameter, 2> params;
                    AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> lhsArgument = lhs.ToBehaviorValueParameter(*method->GetArgument(0));
                    
                    if (lhsArgument.IsSuccess() && lhsArgument.GetValue().m_value)
                    {
                        params[0].Set(lhsArgument.GetValue());
                        AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> rhsArgument = rhs.ToBehaviorValueParameter(*method->GetArgument(1));
                        
                        if (rhsArgument.IsSuccess() && rhsArgument.GetValue().m_value)
                        {
                            params[1].Set(rhsArgument.GetValue());
                            
                            if (method->Call(params.data(), aznumeric_caster(params.size()), &result))
                            {
                                return AZ::Success(comparisonResult);
                            }
                        }
                    }
                }
            }
        }

        return AZ::Failure(AZStd::string("Invalid Comparison Operator Method"));
    }

    void Datum::Clear()
    {
        m_datumStorage.clear();
        m_class = nullptr;
        m_type = Data::Type::Invalid();
    }

    void Datum::ConvertBehaviorContextMethodResult(const AZ::BehaviorParameter& resultType)
    {
        if (IS_A(Data::Type::Number()))
        {
            if (resultType.m_traits & AZ::BehaviorParameter::TR_POINTER)
            {
                if (m_pointer)
                {
                    ::FromBehaviorContextNumber(resultType.m_typeId, m_pointer, m_datumStorage);
                }
            }
            else
            {
                ::FromBehaviorContextNumber(resultType.m_typeId, reinterpret_cast<const void*>(&m_conversionStorage), m_datumStorage);
            }
        }
        else if (Data::IsVectorType(m_type))
        {
            // convert to exact type if necessary
            if (resultType.m_traits & AZ::BehaviorParameter::TR_POINTER)
            {
                if (m_pointer)
                {
                    FromBehaviorContextVector(resultType.m_typeId, m_pointer);
                }
            }
            else
            {
                FromBehaviorContextVector(resultType.m_typeId, reinterpret_cast<const void*>(&m_conversionStorage));
            }
        }
        else if (IS_A(Data::Type::String()) && !Data::IsString(resultType.m_typeId) && AZ::BehaviorContextHelper::IsStringParameter(resultType))
        {
            void* storageAddress = (resultType.m_traits & AZ::BehaviorParameter::TR_POINTER) ? reinterpret_cast<void*>(&m_pointer) : AZStd::any_cast<void>(&m_conversionStorage);
            if (auto stringOutcome = ConvertBehaviorContextString(resultType, storageAddress))
            {
                m_datumStorage = stringOutcome.GetValue();
            }
        }
        else if (m_type.GetType() == Data::eType::BehaviorContextObject
            && (resultType.m_traits & (AZ::BehaviorParameter::TR_POINTER | AZ::BehaviorParameter::TR_REFERENCE)))
        {
            if (m_pointer)
            {
                m_datumStorage = BehaviorContextObject::CreateReference(resultType.m_typeId, m_pointer);
            }
        }
    }
    
    Datum Datum::CreateBehaviorContextMethodResult(const AZ::BehaviorParameter& resultType)
    {
        Datum result;
        result.InitializeBehaviorContextMethodResult(resultType);
        return result;
    }

    Datum Datum::CreateFromBehaviorContextValue(const AZ::BehaviorValueParameter& value)
    {
        const eOriginality originality
            = !(value.m_traits & (AZ::BehaviorParameter::TR_POINTER | AZ::BehaviorParameter::TR_REFERENCE))
            ? eOriginality::Original
            : eOriginality::Copy;

        return Datum(value, originality, value.m_value);
    }

    void Datum::CreateOriginal(const AZStd::string& behaviorClassName)
    {
        AZ_Assert(Empty(), "This datum node is already initialized");
        Initialize(Data::FromAZType(AZ::BehaviorContextHelper::GetClassType(behaviorClassName)), eOriginality::Original, nullptr, AZ::Uuid::CreateNull());
    }

    Datum Datum::CreateUntypedStorage()
    {
        const bool isUntypedStorage(true);
        return Datum(isUntypedStorage);
    }
    
    bool Datum::FromBehaviorContext(const void* source, const AZ::Uuid& typeID)
    {
        // \todo support polymorphism
        const auto& type = Data::FromBehaviorContextType(typeID);
        InitializeUntypedStorage(type);
        
        if (IS_A(type))
        {
            switch (m_type.GetType())
            {
            case ScriptCanvas::Data::eType::AABB:
                return FromBehaviorContextAABB(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::BehaviorContextObject:
                return FromBehaviorContextObject(m_class, source);
            
            case ScriptCanvas::Data::eType::Boolean:
                return FromBehaviorContextBool(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::Color:
                return FromBehaviorContextColor(typeID, source, m_datumStorage);
            
            case ScriptCanvas::Data::eType::CRC:
                return FromBehaviorContextCRC(typeID, source, m_datumStorage);
                
            case ScriptCanvas::Data::eType::EntityID:
                return FromBehaviorContextEntityID(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::Matrix3x3:
                return FromBehaviorContextMatrix3x3(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::Matrix4x4:
                return FromBehaviorContextMatrix4x4(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::Number:
                return ::FromBehaviorContextNumber(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::OBB:
                return FromBehaviorContextOBB(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::Plane:
                return FromBehaviorContextPlane(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::Rotation:
                return FromBehaviorContextRotation(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::String:
                return FromBehaviorContextString(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::Transform:
                return FromBehaviorContextTransform(typeID, source, m_datumStorage);

            case ScriptCanvas::Data::eType::Vector2:
                return ::FromBehaviorContextVector2(typeID, source, m_datumStorage);
            
            case ScriptCanvas::Data::eType::Vector3:
                return ::FromBehaviorContextVector3(typeID, source, m_datumStorage);
            
            case ScriptCanvas::Data::eType::Vector4:
                return ::FromBehaviorContextVector4(typeID, source, m_datumStorage);
            }
        }
        else if (ConvertImplicitlyChecked(type, source, m_type, m_datumStorage, m_class))
        {
            return true;
        }

        AZ_Error("Script Canvas", false, "Invalid type has come into a Script Canvas node");
        return false;
    }

    bool Datum::FromBehaviorContext(const void* source)
    {
        return FromBehaviorContextObject(m_class, source);
    }

    bool Datum::FromBehaviorContextNumber(const void* source, const AZ::Uuid& typeID)
    {
        return ::FromBehaviorContextNumber(typeID, source, m_datumStorage);
    }

    bool Datum::FromBehaviorContextObject(const AZ::BehaviorClass* behaviorClass, const void* source)
    {
        if (behaviorClass)
        {
            m_datumStorage = BehaviorContextObject::CreateReference(behaviorClass->m_typeId, const_cast<void*>(source));
            return true;
        }

        return false;
    }
    
    bool Datum::FromBehaviorContextVector(const AZ::Uuid& typeId, const void* source)
    {   
        switch (m_type.GetType())
        {
        case Data::eType::Vector2:
            return FromBehaviorContextVector2(typeId, source, m_datumStorage);
        
        case Data::eType::Vector3:
            return FromBehaviorContextVector3(typeId, source, m_datumStorage);
        
        case Data::eType::Vector4:
            return FromBehaviorContextVector4(typeId, source, m_datumStorage);
        
        default:
            AZ_Assert(false, "Datum::FromBehaviorContextVector is for vector types only");
            return false;
        }
    }

    const AZ::Edit::ElementData* Datum::GetEditElementData() const
    {
        return &m_datumElementData;
    }

    const void* Datum::GetValueAddress() const
    {
        return m_type.GetType() != Data::eType::BehaviorContextObject 
             ? AZStd::any_cast<void>(&m_datumStorage) 
             : (*AZStd::any_cast<BehaviorContextObjectPtr>(&m_datumStorage))->Get();
    }

    bool Datum::Initialize(const Data::Type& type, eOriginality originality, const void* source, const AZ::Uuid& sourceTypeID)
    {
        if (m_isUntypedStorage)
        {
            Clear();
        }

        AZ_Error("ScriptCanvas", Empty(), "double initialized datum");

        m_type = type;
       
        switch (type.GetType())
        {
        case ScriptCanvas::Data::eType::AABB:
            return InitializeAABB(source);

        case ScriptCanvas::Data::eType::BehaviorContextObject:
            return InitializeBehaviorContextObject(originality, source);

        case ScriptCanvas::Data::eType::Boolean:
            return InitializeBool(source);

        case ScriptCanvas::Data::eType::Color:
            return InitializeColor(source);

        case ScriptCanvas::Data::eType::CRC:
            return InitializeCRC(source);

        case ScriptCanvas::Data::eType::EntityID:
            return InitializeEntityID(source);

        case ScriptCanvas::Data::eType::Matrix3x3:
            return InitializeMatrix3x3(source);

        case ScriptCanvas::Data::eType::Matrix4x4:
            return InitializeMatrix4x4(source);

        case ScriptCanvas::Data::eType::Number:
            return InitializeNumber(source, sourceTypeID);

        case ScriptCanvas::Data::eType::OBB:
            return InitializeOBB(source);

        case ScriptCanvas::Data::eType::Plane:
            return InitializePlane(source);

        case ScriptCanvas::Data::eType::Rotation:
            return InitializeRotation(source);
        
        case ScriptCanvas::Data::eType::String:
            return InitializeString(source);
        
        case ScriptCanvas::Data::eType::Transform:
            return InitializeTransform(source);

        case ScriptCanvas::Data::eType::Vector2:
            return InitializeVector2(source, sourceTypeID);

        case ScriptCanvas::Data::eType::Vector3:
            return InitializeVector3(source, sourceTypeID);

        case ScriptCanvas::Data::eType::Vector4:
            return InitializeVector4(source, sourceTypeID);

        default:
            AZ_Error("Script Canvas", false, "Invalid datum type found datum initialization");
        }

        return false;
    }

    bool Datum::InitializeAABB(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const Data::AABBType*>(source) : AZ::Aabb::CreateNull();
        return true;
    }


    bool Datum::InitializeBehaviorContextParameter(const AZ::BehaviorParameter& parameterDesc, eOriginality originality, const void* source)
    {
        if (AZ::BehaviorContextHelper::IsStringParameter(parameterDesc))
        {
            auto convertOutcome = ConvertBehaviorContextString(parameterDesc, source);
            if (convertOutcome)
            {
                m_type = Data::Type::String();
                return InitializeString(&convertOutcome.GetValue());
            }
        }

        const auto& type = Data::FromBehaviorContextType(parameterDesc.m_typeId);

        return Initialize(type, originality, source, parameterDesc.m_typeId);
    }

    bool Datum::InitializeBehaviorContextMethodResult(const AZ::BehaviorParameter& description)
    {
        if (AZ::BehaviorContextHelper::IsStringParameter(description))
        {
            auto convertOutcome = ConvertBehaviorContextString(description, nullptr);
            if (convertOutcome)
            {
                m_type = Data::Type::String();
                return InitializeString(&convertOutcome.GetValue());
            }
        }

        const auto& type = Data::FromBehaviorContextType(description.m_typeId);
        const eOriginality originality 
            = !(description.m_traits & (AZ::BehaviorParameter::TR_POINTER | AZ::BehaviorParameter::TR_REFERENCE))
            ? eOriginality::Original
            : eOriginality::Copy;

        AZ_VerifyError("ScriptCavas", Initialize(type, originality, nullptr, AZ::Uuid::CreateNull()), "Initialization of BehaviorContext Method result failed");
        return true;
    }

    bool Datum::InitializeBehaviorContextObject(eOriginality originality, const void* source)
    {
        AZ_STATIC_ASSERT(sizeof(BehaviorContextObjectPtr) <= AZStd::Internal::ANY_SBO_BUF_SIZE, "BehaviorContextObjectPtr doesn't fit in generic Datum storage");
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Script Canvas can't do anything without a behavior context!");
        AZ_Assert(!IsValueType(m_type), "Can't initialize value types as objects!");
        const auto azType = m_type.GetAZType();

        auto classIter = behaviorContext->m_typeToClassMap.find(azType);
        if (classIter != behaviorContext->m_typeToClassMap.end())
        {
            const AZ::BehaviorClass& behaviorClass = *classIter->second;
            m_class = &behaviorClass;
            m_originality = originality;

            if (m_originality == eOriginality::Original)
            {
                m_datumStorage = BehaviorContextObject::Create(behaviorClass, source);
            }
            else
            {
                m_datumStorage = BehaviorContextObject::CreateReference(behaviorClass.m_typeId, const_cast<void*>(source));
            }

            return true;
        }

        return false;
    }

    bool Datum::InitializeBool(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const Data::BooleanType*>(source) : false;
        return true;
    }

    bool Datum::InitializeColor(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const Data::ColorType*>(source) : Data::ColorType::CreateZero();
        return true;
    }

    bool Datum::InitializeCRC(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const Data::CRCType*>(source) : Data::CRCType();
        return true;
    }

    bool Datum::InitializeEntityID(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const AZ::EntityId*>(source) : AZ::EntityId{};
        return true;
    }
    
    bool Datum::InitializeMatrix3x3(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const AZ::Matrix3x3*>(source) : AZ::Matrix3x3::CreateIdentity();
        return true;
    }

    bool Datum::InitializeMatrix4x4(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const AZ::Matrix4x4*>(source) : AZ::Matrix4x4::CreateIdentity();
        return true;
    }

    bool Datum::InitializeNumber(const void* source, const AZ::Uuid& sourceTypeID)
    {
        m_datumStorage = Data::NumberType(0);
        return (source && ::FromBehaviorContextNumber(sourceTypeID, source, m_datumStorage)) || true;
    }

    bool Datum::InitializeOBB(const void* source)
    {
        m_datumStorage = source 
            ? *reinterpret_cast<const Data::OBBType*>(source) 
            : AZ::Obb::CreateFromPositionAndAxes
                ( AZ::Vector3::CreateZero()
                , AZ::Vector3(1,0,0), 0.5
                , AZ::Vector3(0,1,0), 0.5
                , AZ::Vector3(0,0,1), 0.5);
        return true;
    }

    bool Datum::InitializePlane(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const Data::PlaneType*>(source) : AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0,0,1), AZ::Vector3::CreateZero());
        return true;
    }
    bool Datum::InitializeRotation(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const Data::RotationType*>(source) : Data::RotationType::CreateIdentity();
        return true;
    }

    bool Datum::InitializeString(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const Data::StringType*>(source) : Data::StringType();
        return true;
    }

    bool Datum::InitializeTransform(const void* source)
    {
        m_datumStorage = source ? *reinterpret_cast<const Data::TransformType*>(source) : Data::TransformType::CreateIdentity();
        return true;
    }

    bool Datum::InitializeVector2(const void* source, const AZ::Uuid& sourceTypeID)
    {
        m_datumStorage = AZ::Vector2::CreateZero();
        // return a success regardless, but do the initialization first if source is not null
        return (source && FromBehaviorContextVector2(sourceTypeID, source, m_datumStorage)) || true;
    }

    bool Datum::InitializeVector3(const void* source, const AZ::Uuid& sourceTypeID)
    {
        m_datumStorage = AZ::Vector3::CreateZero();
        // return a success regardless, but do the initialization first if source is not null
        return (source && FromBehaviorContextVector3(sourceTypeID, source, m_datumStorage)) || true;
    }

    bool Datum::InitializeVector4(const void* source, const AZ::Uuid& sourceTypeID)
    {
        m_datumStorage = AZ::Vector4::CreateZero();
        // return a success regardless, but do the initialization first if source is not null
        return (source && FromBehaviorContextVector4(sourceTypeID, source, m_datumStorage)) || true;
    }

    bool Datum::IsConvertibleTo(const AZ::BehaviorParameter& parameterDesc) const
    {
        if (AZ::BehaviorContextHelper::IsStringParameter(parameterDesc) && Data::IsString(GetType()))
        {
            return true;
        }

        return IsConvertibleTo(Data::FromAZType(parameterDesc.m_typeId));
    }

    bool Datum::IsStorage() const
    {
        return m_originality == eOriginality::Original || Data::IsValueType(GetType());
    }    
     
    void* Datum::ModResultAddress()
    {
        return m_type.GetType() != Data::eType::BehaviorContextObject
            ? AZStd::any_cast<void>(&m_datumStorage)
            : (*AZStd::any_cast<BehaviorContextObjectPtr>(&m_datumStorage))->Mod();
    }

    void* Datum::ModValueAddress() const
    {
        return const_cast<void*>(GetValueAddress());
    }

    Datum& Datum::operator=(Datum&& source)
    {
        if (this != &source)
        {
            if (m_isUntypedStorage || source.IS_A(m_type))
            {
                InitializeUntypedStorage(source.m_type);
                m_originality = AZStd::move(source.m_originality);
                m_class = AZStd::move(source.m_class);
                m_type = AZStd::move(source.m_type);
                m_datumStorage = AZStd::move(source.m_datumStorage);
                OnDatumChanged();
            }
            else if (ConvertImplicitlyChecked(source.GetType(), source.GetValueAddress(), m_type, m_datumStorage, m_class))
            {
                OnDatumChanged();
            }
            else
            {
                AZ_Error("Script Canvas", false, "Script Canvas data is type safe!");
            }

            m_notificationId = source.m_notificationId;

            m_conversionStorage = AZStd::move(source.m_conversionStorage);

            m_datumElementDataAttributeLabel = source.m_datumElementDataAttributeLabel.Get(nullptr);
            m_datumElementDataAttributeVisibility = source.m_datumElementDataAttributeVisibility.Get(nullptr);
        }

        return *this;
    }

    Datum& Datum::operator=(const Datum& source)
    {
        if (this != &source)
        {
            if (m_isUntypedStorage || source.IS_A(m_type))
            {
                InitializeUntypedStorage(source.m_type);
                m_class = source.m_class;
                m_type = source.m_type;
                m_datumStorage = source.m_datumStorage;
                OnDatumChanged();
            }
            else if (ConvertImplicitlyChecked(source.GetType(), source.GetValueAddress(), m_type, m_datumStorage, m_class))
            {
                OnDatumChanged();
            }
            else
            {
                AZ_Error("Script Canvas", false, "Script Canvas data is type safe!");
            }

            m_notificationId = source.m_notificationId;

            m_conversionStorage = source.m_conversionStorage;

            m_datumElementDataAttributeLabel = source.m_datumElementDataAttributeLabel.Get(nullptr);
            m_datumElementDataAttributeVisibility = source.m_datumElementDataAttributeVisibility.Get(nullptr);
        }

        return *this;
    }

    ComparisonOutcome Datum::operator==(const Datum& other) const
    {
        if (this == &other)
        {
            return AZ::Success(true);
        }
        else if (m_type.IS_EXACTLY_A(other.GetType()))
        {
            if (m_type.GetType() == Data::eType::BehaviorContextObject)
            {
                return CallComparisonOperator(AZ::Script::Attributes::OperatorType::Equal, *m_class, *this, other);
            }
            else
            {
                return AZ::Success(IsDataEqual(m_type, GetValueAddress(), other.GetValueAddress()));
            }
        }

        return AZ::Failure(AZStd::string("Invalid call of Datum::operator=="));
    }

    ComparisonOutcome Datum::operator!=(const Datum& other) const
    {
        if (this == &other)
        {
            return AZ::Success(false);
        }

        ComparisonOutcome isEqualResult = (*this == other);
        if (isEqualResult.IsSuccess())
        {
            return AZ::Success(!isEqualResult.GetValue());
        }
        else
        {
            return AZ::Failure(AZStd::string("Invalid call of Datum::operator!="));
        }
    }

    ComparisonOutcome Datum::operator<(const Datum& other) const
    {
        if (this == &other)
        {
            return AZ::Success(false);
        }
        else if (m_type.IS_EXACTLY_A(other.GetType()))
        {
            if (m_type.GetType() == Data::eType::BehaviorContextObject)
            {
                return CallComparisonOperator(AZ::Script::Attributes::OperatorType::LessThan, *m_class, *this, other);
            }
            else 
            {
                return AZ::Success(IsDataLess(m_type, GetValueAddress(), other.GetValueAddress()));
            }
        }

        return AZ::Failure(AZStd::string("Invalid call of Datum::operator<"));
    }

    ComparisonOutcome Datum::operator<=(const Datum& other) const
    {
        if (this == &other)
        {
            return AZ::Success(true);
        }
        else if (m_type.IS_EXACTLY_A(other.GetType()))
        {
            if (m_type.GetType() == Data::eType::BehaviorContextObject)
            {
                return CallComparisonOperator(AZ::Script::Attributes::OperatorType::LessEqualThan, *m_class, *this, other);
            }
            else
            {
                return AZ::Success(IsDataLessEqual(m_type, GetValueAddress(), other.GetValueAddress()));
            }
        }

        return AZ::Failure(AZStd::string("Invalid call of Datum::operator<"));
    }

    ComparisonOutcome Datum::operator>(const Datum& other) const
    {
        if (this == &other)
        {
            return AZ::Success(false);
        }

        ComparisonOutcome isLessEqualResult = (*this <= other);
        
        if (isLessEqualResult.IsSuccess())
        {
            return AZ::Success(!isLessEqualResult.GetValue());
        }
        else
        {
            return AZ::Failure(AZStd::string("Invalid call of Datum::Datum::operator>"));
        }
    }

    ComparisonOutcome Datum::operator>=(const Datum& other) const
    {
        if (this == &other)
        {
            return AZ::Success(true);
        }

        ComparisonOutcome isLessResult = (*this < other);
        
        if (isLessResult.IsSuccess())
        {
            return AZ::Success(!isLessResult.GetValue());
        }
        else
        {
            return AZ::Failure(AZStd::string("Invalid call of Datum::Datum::operator>="));
        }
    }

    void Datum::OnDatumChanged()
    {
        DatumNotificationBus::Event(m_notificationId, &DatumNotifications::OnDatumChanged, this);
    }

    void Datum::OnReadBegin()
    {
        // right now, edit-time graphs are cloned for execution, if that ever changes, this will have to 
        // respect originality and value type as well, to detect possibly saveable changes at run-time
        if (m_isUntypedStorage)
        {
            Clear();
        }
    }

    void Datum::OnWriteEnd()
    {
        if (m_type.GetType() == Data::eType::BehaviorContextObject)
        {
            /*
            BehaviorContextObject types require that their behavior context classes are updated, and their type infos are updated
            */
            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            AZ_Assert(behaviorContext, "Script Canvas can't do anything without a behavior context!");

            auto classIter = behaviorContext->m_typeToClassMap.find(m_type.GetAZType());
            if (classIter != behaviorContext->m_typeToClassMap.end() && classIter->second)
            {
                m_class = classIter->second;
            }
            else
            {
                AZ_Error("Script Canvas", false, "Datum type de-serialized, but no such class found in the behavior context");
            }
        }
    }

    void Datum::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<Datum>()
                ->Version(5)
                ->EventHandler<SerializeContextEventHandler>()
                ->Field("m_isUntypedStorage", &Datum::m_isUntypedStorage)
                ->Field("m_type", &Datum::m_type)
                ->Field("m_originality", &Datum::m_originality)
                ->Field("m_datumStorage", &Datum::m_datumStorage)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Datum>("Datum", "Datum")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Datum::m_datumStorage, "Datum", "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Datum::OnDatumChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }
    
    void Datum::SetLabel(AZStd::string_view name)
    {
        m_datumElementDataAttributeLabel = name;
    }

    void Datum::SetVisibility(AZ::Crc32 visibility)
    {
        m_datumElementDataAttributeVisibility = visibility;
    }
    
    void Datum::SetNotificationsTarget(AZ::EntityId notificationId)
    {
        m_notificationId = notificationId;
    }

    // pushes this datum to the void* address in destination
    bool Datum::ToBehaviorContext(AZ::BehaviorValueParameter& destination) const
    {
        /// \todo support polymorphism
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Script Canvas can't do anything without a behavior context!");
        AZ::BehaviorClass* destinationBehaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, destination.m_typeId);               
        const auto targetType = Data::FromBehaviorContextType(destination.m_typeId);
        
        const bool success = 
            (IS_A(targetType) || IsConvertibleTo(targetType))
            && ::ToBehaviorContext(m_type, GetValueAddress(), destination.m_typeId, destination.GetValueAddress(), destinationBehaviorClass);

        AZ_Error("Script Canvas", success, "invalid datum going from Script Canvas!");
        return success;
    }

    bool Datum::ToBehaviorContextNumber(void* target, const AZ::Uuid& typeID) const
    {
        return ::ToBehaviorContextNumber(target, typeID, GetValueAddress());
    }

    AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> Datum::ToBehaviorValueParameter(const AZ::BehaviorParameter& description) const
    {
        AZ_Assert(m_isUntypedStorage || IS_A(Data::FromAZType(description.m_typeId)) || IsConvertibleTo(description), "Mismatched type going to behavior value parameter");
        
        const_cast<Datum*>(this)->InitializeUntypedStorage(Data::FromBehaviorContextType(description.m_typeId));
        
        if (!Data::IsValueType(m_type) && !SatisfiesTraits(description.m_traits))
        {
            return AZ::Failure(AZStd::string("Attempting to convert null value to BehaviorValueParameter that expects reference or value"));
        }

        if (IS_A(Data::Type::Number()))
        {
            return AZ::Success(const_cast<Datum*>(this)->ToBehaviorValueParameterNumber(description));
        }
        else if (Data::IsVectorType(m_type)) 
        {
            return const_cast<Datum*>(this)->ToBehaviorValueParameterVector(description);
        }
        else if (IS_A(Data::Type::String()) && AZ::BehaviorContextHelper::IsStringParameter(description))
        {
            return const_cast<Datum*>(this)->ToBehaviorValueParameterString(description);
        }
        else
        {
            AZ::BehaviorValueParameter parameter;
            parameter.m_typeId = description.m_typeId; /// \todo verify there is no polymorphic danger here
            parameter.m_name = m_class ? m_class->m_name.c_str() : Data::GetName(m_type);
            parameter.m_azRtti = m_class ? m_class->m_azRtti : nullptr;

            if (description.m_traits & AZ::BehaviorParameter::TR_POINTER)
            {
                m_pointer = ModValueAddress();
                if (description.m_traits & AZ::BehaviorParameter::TR_THIS_PTR && !m_pointer)
                {
                    return AZ::Failure(AZStd::string(R"(Cannot invoke behavior context method on nullptr "this" parameter)"));
                }
                parameter.m_value = &m_pointer;
                parameter.m_traits = AZ::BehaviorParameter::TR_POINTER;
            }
            else
            {
                parameter.m_value = ModValueAddress();
                parameter.m_traits = 0;
            }

            return AZ::Success(parameter);
        }
    }
    
    AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> Datum::ToBehaviorValueParameterResult(const AZ::BehaviorParameter& description)
    {
        AZ_Assert(m_isUntypedStorage || IS_A(Data::FromAZType(description.m_typeId)) || IsConvertibleTo(description), "Mismatched type going to behavior value parameter");

        InitializeUntypedStorage(Data::FromBehaviorContextType(description.m_typeId));

        if (IS_A(Data::Type::Number()))
        {
            return AZ::Success(const_cast<Datum*>(this)->ToBehaviorValueParameterNumber(description));
        }
        else if (Data::IsVectorType(m_type))
        {
            return const_cast<Datum*>(this)->ToBehaviorValueParameterVector(description);
        }
        else if (IS_A(Data::Type::String()) && AZ::BehaviorContextHelper::IsStringParameter(description))
        {
            return const_cast<Datum*>(this)->ToBehaviorValueParameterString(description);
        }

        AZ::BehaviorValueParameter parameter;

        if (IsValueType(m_type))
        {
            parameter.m_typeId = description.m_typeId; /// \todo verify there is no polymorphic danger here
            parameter.m_name = m_class ? m_class->m_name.c_str() : Data::GetName(m_type);
            parameter.m_azRtti = m_class ? m_class->m_azRtti : nullptr;

            if (description.m_traits & AZ::BehaviorParameter::TR_POINTER)
            {
                m_pointer = ModResultAddress();
                
                if (!m_pointer)
                {
                    return AZ::Failure(AZStd::string("nowhere to go for the for behavior context result"));
                }

                parameter.m_value = &m_pointer;
                parameter.m_traits = AZ::BehaviorParameter::TR_POINTER;
            }
            else
            {
                parameter.m_value = ModResultAddress();
                
                if (!parameter.m_value)
                {
                    return AZ::Failure(AZStd::string("nowhere to go for the for behavior context result"));
                }
                
                parameter.m_traits = 0;
            }
        }
        else
        {
            parameter.m_typeId = description.m_typeId; /// \todo verify there is no polymorphic danger here
            parameter.m_name = m_class ? m_class->m_name.c_str() : Data::GetName(m_type);
            parameter.m_azRtti = m_class ? m_class->m_azRtti : nullptr;
            
            if (description.m_traits & (AZ::BehaviorParameter::TR_POINTER | AZ::BehaviorParameter::TR_REFERENCE))
            {
                parameter.m_value = &m_pointer;
                parameter.m_traits = AZ::BehaviorParameter::TR_POINTER;
            }
            else
            {
                parameter.m_value = ModResultAddress();
                
                if (!parameter.m_value)
                {
                    return AZ::Failure(AZStd::string("nowhere to go for the for behavior context result"));
                }
            }
        }

        return AZ::Success(parameter);
    }

    AZ::BehaviorValueParameter Datum::ToBehaviorValueParameterNumber(const AZ::BehaviorParameter& description)
    {
        AZ_Assert(IS_A(Data::Type::Number()), "ToBehaviorValueParameterNumber is only for numbers");
        // m_conversion isn't a number yet
        // make it a number by initializing it to the proper type
        ::ToBehaviorContextNumber(m_conversionStorage, description.m_typeId, GetValueAddress());
        return ConvertibleToBehaviorValueParameter(description, description.m_typeId, nullptr, reinterpret_cast<void*>(&m_conversionStorage), m_pointer);
    }

    AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> Datum::ToBehaviorValueParameterVector(const AZ::BehaviorParameter& description)
    {
        AZ_Assert(Data::IsVectorType(m_type), "ToBehaviorValueParameterVector is only for Vectors");
        
        if (description.m_typeId == azrtti_typeid<AZ::Vector3>())
        {
            m_conversionStorage = AZ::Vector3::CreateZero();
        }
        else if (description.m_typeId == azrtti_typeid<AZ::Vector2>())
        {
            m_conversionStorage = AZ::Vector2::CreateZero();
        }
        else if (description.m_typeId == azrtti_typeid<AZ::Vector4>())
        {
            m_conversionStorage = AZ::Vector4::CreateZero();
        }
        else
        {
            return AZ::Failure(AZStd::string("bad vector type in ToBehaviorValueParameterVector"));
        }

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Script Canvas can't do anything without a behavior context!");

        auto classIter = behaviorContext->m_typeToClassMap.find(description.m_typeId);
        if (classIter != behaviorContext->m_typeToClassMap.end() && classIter->second)
        {
            const AZ::BehaviorClass* behaviorClass = classIter->second;
            // first convert the vector, and store the result in m_conversionStorage, ...
            if (ConvertImplicitlyChecked(m_type, AZStd::any_cast<void>(&m_datumStorage), Data::FromBehaviorContextType(description.m_typeId), m_conversionStorage, behaviorClass))
            {
                // ...then send the auxiliary storage value out as the parameter
                return AZ::Success(ConvertibleToBehaviorValueParameter(description, description.m_typeId, nullptr, reinterpret_cast<void*>(&m_conversionStorage), m_pointer, description.m_azRtti));
            }
            else
            {
                return AZ::Failure(AZStd::string("Failed to convert script canvas vector to behavior context vector"));
            }
        }

        return AZ::Failure(AZStd::string("Vector behavior class not found in behavior context"));
    }

    AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> Datum::ToBehaviorValueParameterString(const AZ::BehaviorParameter& description)
    {
        AZ_Assert(IS_A(Data::Type::String()), "Cannot created BehaviorValueParameter that contains a string. Datum type must be a string");

        if (!AZ::BehaviorContextHelper::IsStringParameter(description))
        {
            return AZ::Failure(AZStd::string("BehaviorParameter is not a string parameter, a BehaviorValueParameter that references a Script Canvas string cannot be made"));
        }

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Script Canvas can't do anything without a behavior context!");

        if (Data::IsString(description.m_typeId))
        {
            return AZ::Success(ConvertibleToBehaviorValueParameter(description, description.m_typeId, nullptr, ModValueAddress(), m_pointer, description.m_azRtti));
        }
        else
        {
            auto stringValue = GetAs<Data::StringType>();
            if (description.m_typeId == azrtti_typeid<char>() && (description.m_traits | (AZ::BehaviorParameter::TR_POINTER & AZ::BehaviorParameter::TR_CONST)))
            {
                return AZ::Success(ConvertibleToBehaviorValueParameter(description, description.m_typeId, nullptr, const_cast<char*>(stringValue->data()), m_pointer, description.m_azRtti));
            }
            else if (description.m_typeId == azrtti_typeid<AZStd::string_view>())
            {
                m_conversionStorage = AZStd::make_any<AZStd::string_view>(*stringValue);
                return AZ::Success(ConvertibleToBehaviorValueParameter(description, description.m_typeId, nullptr, AZStd::any_cast<void>(&m_conversionStorage), m_pointer, description.m_azRtti));
            }
        }

        return AZ::Failure(AZStd::string::format("Cannot create a BehaviorValueParameter of type %s", description.m_name));
    }
       
    bool Datum::ToString(Data::StringType& result) const
    {
        switch (GetType().GetType())
        {
        case Data::eType::AABB:
            result = ToStringAABB(*GetAs<Data::AABBType>());
            return true;

        case Data::eType::BehaviorContextObject:
            ToStringBehaviorClassObject(result);
            return true;

        case Data::eType::Boolean:
            result = *GetAs<bool>() ? "true" : "false";
            return true;

        case Data::eType::Color:
            result = ToStringColor(*GetAs<Data::ColorType>());
            return true;

        case Data::eType::CRC:
            result = ToStringCRC(*GetAs<Data::CRCType>());
            return true;

        case Data::eType::EntityID:
            result = GetAs<AZ::EntityId>()->ToString();
            return true;

        case Data::eType::Invalid:
            result = "Invalid";
            return true;

        case Data::eType::Matrix3x3:
            result = ToStringMatrix3x3(*GetAs<AZ::Matrix3x3>());
            return true;

        case Data::eType::Matrix4x4:
            result = ToStringMatrix4x4(*GetAs<AZ::Matrix4x4>());
            return true;

        case Data::eType::Number:
            result = AZStd::string::format("%f", *GetAs<Data::NumberType>());
            return true;

        case Data::eType::OBB:
            result = ToStringOBB(*GetAs<Data::OBBType>());
            return true;

        case Data::eType::Plane:
            result = ToStringPlane(*GetAs<Data::PlaneType>());
            return true;

        case Data::eType::Rotation:
            result = ToStringRotation(*GetAs<Data::RotationType>());
            return true;

        case Data::eType::String:
            result = *GetAs<Data::StringType>();
            return true;

        case Data::eType::Transform:
            result = ToStringTransform(*GetAs<Data::TransformType>());
            return true;

        case Data::eType::Vector2:
            result = ToStringVector2(*GetAs<AZ::Vector2>());
            return true;

        case Data::eType::Vector3:
            result = ToStringVector3(*GetAs<AZ::Vector3>());
            return true;

        case Data::eType::Vector4:
            result = ToStringVector4(*GetAs<AZ::Vector4>());
            return true;

        default:
            AZ_Error("ScriptCanvas", false, "Unsupported type in Datum::ToString()");
            break;
        }

        result = AZStd::string::format("<Datum.ToString() failed for this type: %s >", Data::GetName(m_type));
        return false;
    }
    
    AZStd::string Datum::ToStringAABB(const Data::AABBType& aabb) const
    {
        return AZStd::string::format
            ( "(Min: %s, Max: %s)"
            , ToStringVector3(aabb.GetMin()).c_str()
            , ToStringVector3(aabb.GetMax()).c_str());
    }

    AZStd::string Datum::ToStringCRC(const Data::CRCType& source) const
    {
        return AZStd::string::format("0x%08x", static_cast<AZ::u32>(source));
    }

    AZStd::string Datum::ToStringColor(const Data::ColorType& c) const
    {
        return AZStd::string::format("(r=%.7f,g=%.7f,b=%.7f,a=%.7f)", static_cast<float>(c.GetR()), static_cast<float>(c.GetG()), static_cast<float>(c.GetB()), static_cast<float>(c.GetA()));
    }

    bool Datum::ToStringBehaviorClassObject(Data::StringType& stringOut) const
    {
        if (m_class)
        {
            for (auto& methodIt : m_class->m_methods)
            {
                auto* method = methodIt.second;

                if (AZ::Attribute* operatorAttr = AZ::FindAttribute(AZ::Script::Attributes::Operator, method->m_attributes))
                {
                    AZ::AttributeReader operatorAttrReader(nullptr, operatorAttr);
                    AZ::Script::Attributes::OperatorType operatorType{};
                    if (operatorAttrReader.Read<AZ::Script::Attributes::OperatorType>(operatorType) && operatorType == AZ::Script::Attributes::OperatorType::ToString &&
                        method->HasResult() && (method->GetResult()->m_typeId == azrtti_typeid<const char*>() || method->GetResult()->m_typeId == azrtti_typeid<AZStd::string>()))
                    {
                        if (method->GetNumArguments() > 0)
                        {
                            AZ::BehaviorValueParameter result(&stringOut);
                            AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> argument = ToBehaviorValueParameter(*method->GetArgument(0));
                            return argument.IsSuccess() && argument.GetValue().m_value && method->Call(&argument.GetValue(), 1, &result);
                        }
                    }
                }
            }
        }
        
        stringOut = "<Invalid ToString Method>";
        return false;
    }
    
    AZStd::string Datum::ToStringMatrix3x3(const AZ::Matrix3x3& m) const
    {
        return AZStd::string::format
            ( "(%s, %s, %s)"
            , ToStringVector3(m.GetColumn(0)).c_str()
            , ToStringVector3(m.GetColumn(1)).c_str()
            , ToStringVector3(m.GetColumn(2)).c_str());

    }

    AZStd::string Datum::ToStringMatrix4x4(const AZ::Matrix4x4& m) const
    {
        return AZStd::string::format
            ( "(%s, %s, %s, %s)"
            , ToStringVector4(m.GetColumn(0)).c_str()
            , ToStringVector4(m.GetColumn(1)).c_str()
            , ToStringVector4(m.GetColumn(2)).c_str()
            , ToStringVector4(m.GetColumn(3)).c_str());

    }

    AZStd::string Datum::ToStringOBB(const Data::OBBType& obb) const
    {
        return AZStd::string::format
            ( "(Position: %s, AxisX: %s, AxisY: %s, AxisZ: %s, halfLengthX: %.7f, halfLengthY: %.7f, halfLengthZ: %.7f)"
            , ToStringVector3(obb.GetPosition()).c_str()
            , ToStringVector3(obb.GetAxisX()).c_str()
            , ToStringVector3(obb.GetAxisY()).c_str()
            , ToStringVector3(obb.GetAxisZ()).c_str()
            , obb.GetHalfLengthX()
            , obb.GetHalfLengthY()
            , obb.GetHalfLengthZ());
    }

    AZStd::string Datum::ToStringPlane(const Data::PlaneType& source) const
    {
        return ToStringVector4(source.GetPlaneEquationCoefficients());
    }

    AZStd::string Datum::ToStringRotation(const Data::RotationType& source) const
    {
        AZ::Vector3 eulerRotation = AzFramework::ConvertTransformToEulerDegrees(AZ::Transform::CreateFromQuaternion(source));
        return AZStd::string::format
            ( "(Pitch: %5.2f, Roll: %5.2f, Yaw: %5.2f)"
            , static_cast<float>(eulerRotation.GetX())
            , static_cast<float>(eulerRotation.GetY())
            , static_cast<float>(eulerRotation.GetZ()));
    }

    AZStd::string Datum::ToStringTransform(const Data::TransformType& source) const
    {
        Data::TransformType copy(source);
        AZ::Vector3 pos = copy.GetPosition();
        AZ::Vector3 scale = copy.ExtractScale();
        AZ::Vector3 rotation = AzFramework::ConvertTransformToEulerDegrees(copy);
        return AZStd::string::format
             ( "(Position: X: %f, Y: %f, Z: %f,"
               " Rotation: X: %f, Y: %f, Z: %f,"
               " Scale: X: %f, Y: %f, Z: %f)"
             , static_cast<float>(pos.GetX()), static_cast<float>(pos.GetY()), static_cast<float>(pos.GetZ())
             , static_cast<float>(rotation.GetX()), static_cast<float>(rotation.GetY()), static_cast<float>(rotation.GetZ())
             , static_cast<float>(scale.GetX()), static_cast<float>(scale.GetY()), static_cast<float>(scale.GetZ()));
    }

    AZStd::string Datum::ToStringVector2(const AZ::Vector2& source) const
    {
        return AZStd::string::format
            ( "(X: %f, Y: %f)"
            , source.GetX()
            , source.GetY());
    }
    
    AZStd::string Datum::ToStringVector3(const AZ::Vector3& source) const
    {
        return AZStd::string::format
            ( "(X: %f, Y: %f, Z: %f)"
            , static_cast<float>(source.GetX())
            , static_cast<float>(source.GetY())
            , static_cast<float>(source.GetZ()));
    }
    
    AZStd::string Datum::ToStringVector4(const AZ::Vector4& source) const
    {
        return AZStd::string::format
            ("(X: %f, Y: %f, Z: %f, W: %f)"
            , static_cast<float>(source.GetX())
            , static_cast<float>(source.GetY())
            , static_cast<float>(source.GetZ())
            , static_cast<float>(source.GetW()));
    }
} // namespace ScriptCanvas
