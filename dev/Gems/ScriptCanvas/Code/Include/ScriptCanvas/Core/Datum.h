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

#include <AzCore/Math/VectorFloat.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string_view.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Data/BehaviorContextObject.h>
#include <AzCore/Outcome/Outcome.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    using ComparisonOutcome = AZ::Outcome<bool, AZStd::string>;

    /// A Datum is used to provide generic storage for all data types in ScriptCanvas, and provide a common interface to accessing, modifying, and displaying them 
    /// in the editor, regardless of their actual ScriptCanvas or BehaviorContext type.
    class Datum final
    {
    public:
        /// \todo support polymorphism        
        AZ_TYPE_INFO(Datum, "{8B836FC0-98A8-4A81-8651-35C7CA125451}");
        AZ_CLASS_ALLOCATOR(Datum, AZ::SystemAllocator, 0);

        enum class eOriginality : int
        {
            Original,
            Copy
        };
                
        // calls a function and converts the result to a ScriptCanvas type, if necessary
        AZ_INLINE static AZ::Outcome<Datum, AZStd::string> CallBehaviorContextMethodResult(AZ::BehaviorMethod* method, const AZ::BehaviorParameter* resultType, AZ::BehaviorValueParameter* params, unsigned int numExpectedArgs);
        
        template<typename t_Value, bool isPointer=false, bool forceReference=false>
        static Datum CreateInitializedCopy(const t_Value& value);
                
        template<typename t_Value>
        static Datum CreateInitializedCopyFromBehaviorContext(const t_Value& value);

        static Datum CreateBehaviorContextMethodResult(const AZ::BehaviorParameter& resultType);
                
        static Datum CreateFromBehaviorContextValue(const AZ::BehaviorValueParameter& value);

        static Datum CreateUntypedStorage();
        
        static void Reflect(AZ::ReflectContext* reflectContext);

        Datum();
        Datum(const Datum& object);
        Datum(Datum&& object);
        Datum(const Data::Type& type, eOriginality originality);
        Datum(const Data::Type& type, eOriginality originality, const void* source, const AZ::Uuid& sourceTypeID);
        Datum(const AZ::BehaviorParameter& parameterDesc, eOriginality originality, const void* source);
        Datum(const AZStd::string& behaviorClassName, eOriginality originality);
                
        // after used as the destination for a Behavior Context function call, the result must be converted
        void ConvertBehaviorContextMethodResult(const AZ::BehaviorParameter& resultType);
        
        // use RARELY
        void CreateOriginal(const AZStd::string& behaviorClassName);
                
        AZ_INLINE bool Empty() const;
        
        //! use RARELY, this is dangerous. Use ONLY to read the value contained by this Datum, never to modify
        template<typename t_Value>
        AZ_INLINE const t_Value* GetAs() const;
                        
        //! use RARELY, this is dangerous as it circumvents ScriptCanvas execution. Use to initialize values more simply in unit testing, or assist debugging.
        template<typename t_Value>
        AZ_INLINE t_Value* ModAs();
        
        const Data::Type& GetType() const { return m_type; }

        // checks this datum can be converted to the specified type, including checking for required storage
        AZ_INLINE bool IsConvertibleFrom(const AZ::Uuid& typeID) const;
        AZ_INLINE bool IsConvertibleFrom(const Data::Type& type) const;
        
        // checks if the type of this datum can be converted to the specified type, regardless of storage 
        AZ_INLINE bool IsConvertibleTo(const AZ::Uuid& typeID) const;
        AZ_INLINE bool IsConvertibleTo(const Data::Type& type) const;
        bool IsConvertibleTo(const AZ::BehaviorParameter& parameterDesc) const;

        bool IsStorage() const;
                               
        // todo support polymorphism
        // returns true if this type IS_A t_Value type
        template<typename t_Value>
        AZ_INLINE bool IS_A() const;

        // todo support polymorphism
        AZ_INLINE bool IS_A(const Data::Type& type) const;
        
        // can cause de-allocation of original datum, to which other data can point!
        Datum& operator=(const Datum& other);
        Datum& operator=(Datum&& other);

        ComparisonOutcome operator==(const Datum& other) const;
        ComparisonOutcome operator!=(const Datum& other) const;

        ComparisonOutcome operator<(const Datum& other) const;
        ComparisonOutcome operator<=(const Datum& other) const;
        ComparisonOutcome operator>(const Datum& other) const;
        ComparisonOutcome operator>=(const Datum& other) const;

        // use RARELY, this is dangerous
        template<typename t_Value>
        AZ_INLINE bool Set(const t_Value& value);

        template<typename t_Value>
        AZ_INLINE bool SetFromBehaviorContext(const t_Value& value);
        
        void SetNotificationsTarget(AZ::EntityId notificationId);

        // pushes this datum to the void* address in destination
        bool ToBehaviorContext(AZ::BehaviorValueParameter& destination) const;
        
        // creates an AZ::BehaviorValueParameter with a void* that points to this datum, depending on what the parameter needs
        // this is called when the AZ::BehaviorValueParameter needs this value as input to another function
        // so it is appropriate for the value output to be nullptr
        AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> ToBehaviorValueParameter(const AZ::BehaviorParameter& description) const;

        // creates an AZ::BehaviorValueParameter with a void* that points to this datum, depending on what the parameter needs
        // this is called when the AZ::BehaviorValueParameter needs this value as output from another function
        // so it is NOT appropriate for the value output to be nullptr, if the description is for a pointer to an object
        // there needs to be valid memory to write that pointer
        AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> ToBehaviorValueParameterResult(const AZ::BehaviorParameter& description);
                
        AZ_INLINE AZStd::string ToString() const;

        bool ToString(Data::StringType& result) const;

        void SetLabel(AZStd::string_view name);
        void SetVisibility(AZ::Crc32 visibility);
        const AZ::Edit::ElementData* GetEditElementData() const;

        /// \note Directly modifying the data circumvents all of the runtime (and even the edit time) handling of script canvas data.
        /// This is dangerous in that one may not be properly handling values, like converting numeric types to the supported numeric type, 
        /// or modifying objects without notifying systems whose correctness depends on changes in those values.
        /// Use with extreme caution. This will made clearer and easier in a future API change.
        AZ_INLINE const void* GetAsDanger() const;
        AZ_INLINE void* ModAsDanger();

    protected:
        static ComparisonOutcome CallComparisonOperator(AZ::Script::Attributes::OperatorType operatorType, const AZ::BehaviorClass& behaviorClass, const Datum& lhs, const Datum& rhs);

        // Destroys the datum, and the type information
        void Clear();

        bool FromBehaviorContext(const void* source);
                
        bool FromBehaviorContext(const void* source, const AZ::Uuid& typeID);
        
        bool FromBehaviorContextNumber(const void* source, const AZ::Uuid& typeID);

        bool FromBehaviorContextObject(const AZ::BehaviorClass* behaviorClass, const void* source);

        bool FromBehaviorContextVector(const AZ::Uuid& typeId, const void* source);
      
        const void* GetValueAddress() const;
                
        bool Initialize(const Data::Type& type, eOriginality originality, const void* source, const AZ::Uuid& sourceTypeID);

        bool InitializeBehaviorContextParameter(const AZ::BehaviorParameter& parameterDesc, eOriginality originality, const void* source);

        bool InitializeAABB(const void* source);
        
        bool InitializeBehaviorContextObject(eOriginality originality, const void* source);
        
        bool InitializeBehaviorContextMethodResult(const AZ::BehaviorParameter& resultType);
        
        bool InitializeBool(const void* source);

        bool InitializeColor(const void* source);

        bool InitializeCRC(const void* source);

        bool InitializeEntityID(const void* source);
        
        void InitializeLabel();

        bool InitializeMatrix3x3(const void* source);

        bool InitializeMatrix4x4(const void* source);

        bool InitializeNumber(const void* source, const AZ::Uuid& sourceTypeID);
    
        bool InitializeOBB(const void* source);
        
        bool InitializePlane(const void* source);

        bool InitializeRotation(const void* source);
        
        bool InitializeString(const void* source);

        bool InitializeTransform(const void* source);
        
        AZ_INLINE bool InitializeUntypedStorage(const Data::Type& type);

        bool InitializeVector2(const void* source, const AZ::Uuid& sourceTypeID);

        bool InitializeVector3(const void* source, const AZ::Uuid& sourceTypeID);

        bool InitializeVector4(const void* source, const AZ::Uuid& sourceTypeID);

        void* ModResultAddress();

        void* ModValueAddress() const;

        void OnDatumChanged();

        void OnReadBegin();

        void OnWriteEnd();

        AZ_INLINE bool SatisfiesTraits(AZ::u8 behaviorValueTraits) const;

        bool ToBehaviorContextNumber(void* target, const AZ::Uuid& typeID) const;

        AZ::BehaviorValueParameter ToBehaviorValueParameterNumber(const AZ::BehaviorParameter& description);
        
        AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> ToBehaviorValueParameterVector(const AZ::BehaviorParameter& description);

        AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> ToBehaviorValueParameterString(const AZ::BehaviorParameter& description);
        
        AZStd::string ToStringAABB(const Data::AABBType& source) const;

        AZStd::string ToStringColor(const Data::ColorType& source) const;

        AZStd::string ToStringCRC(const Data::CRCType& source) const;

        bool ToStringBehaviorClassObject(Data::StringType& result) const;

        AZStd::string ToStringMatrix3x3(const AZ::Matrix3x3& source) const;

        AZStd::string ToStringMatrix4x4(const AZ::Matrix4x4& source) const;

        AZStd::string ToStringOBB(const Data::OBBType& source) const;

        AZStd::string ToStringPlane(const Data::PlaneType& source) const;

        AZStd::string ToStringRotation(const Data::RotationType& source) const;

        AZStd::string ToStringTransform(const Data::TransformType& source) const;

        AZStd::string ToStringVector2(const AZ::Vector2& source) const;
        
        AZStd::string ToStringVector3(const AZ::Vector3& source) const;
        
        AZStd::string ToStringVector4(const AZ::Vector4& source) const;

    private:        
        class SerializeContextEventHandler : public AZ::SerializeContext::IEventHandler
        {
        public:
            /// Called right before we start reading from the instance pointed by classPtr.
            void OnReadBegin(void* classPtr) override
            {
                Datum* datum = reinterpret_cast<Datum*>(classPtr);
                datum->OnReadBegin();
            }

            /// Called after we are done writing to the instance pointed by classPtr.
            void OnWriteEnd(void* classPtr) override
            {
                Datum* datum = reinterpret_cast<Datum*>(classPtr);
                datum->OnWriteEnd();
            }
        };
                
        friend class SerializeContextEventHandler;
        
        // if is script canvas value type...use value
        // else if passed by value...use value
        // else...pass by reference, translate the value to a void*
        template<typename t_Value, bool isPointer = false, bool forceReference = false>
        struct CreateInitializedCopyHelper
        {
            AZ_FORCE_INLINE static Datum Help(const t_Value& value)
            {
                const bool isValue = Data::Traits<t_Value>::s_isNative || (!(AZStd::is_pointer<typename AZStd::remove_reference<t_Value>::type>::value || AZStd::is_reference<t_Value>::value));
                return Datum(Data::FromAZType(azrtti_typeid<t_Value>()), isValue ? Datum::eOriginality::Original : Datum::eOriginality::Copy, reinterpret_cast<const void*>(&value), azrtti_typeid<t_Value>());
            }
        };
                
        template<typename t_Value>
        struct CreateInitializedCopyHelper<t_Value, true, false>
        {
            AZ_FORCE_INLINE static Datum Help(const t_Value& value)
            {
                return Datum(Data::FromAZType(azrtti_typeid<t_Value>()), Datum::eOriginality::Original, reinterpret_cast<const void*>(value), azrtti_typeid<t_Value>());
            }
        };
        
        template<typename t_Value>
        struct CreateInitializedCopyHelper<t_Value, false, true>
        {
            AZ_FORCE_INLINE static Datum Help(const t_Value& value)
            {
                return Datum(Data::FromAZType(azrtti_typeid<t_Value>()), Datum::eOriginality::Copy, reinterpret_cast<const void*>(&value), azrtti_typeid<t_Value>());
            }
        };

        template<typename t_Value>
        struct CreateInitializedCopyHelper<t_Value, true, true>
        {
            AZ_FORCE_INLINE static Datum Help(const t_Value& value)
            {
                return Datum(Data::FromAZType(azrtti_typeid<t_Value>()), Datum::eOriginality::Copy, reinterpret_cast<const void*>(value), azrtti_typeid<t_Value>());
            }
        };

        template<typename t_Value>
        struct GetAsHelper
        {
            AZ_FORCE_INLINE static const t_Value* Help(Datum& datum)
            {
                AZ_STATIC_ASSERT(!AZStd::is_pointer<t_Value>::value, "no pointer types in the Datum::GetAsHelper<t_Value, false>");
                return (datum.m_type.GetType() == Data::eType::BehaviorContextObject)
                    ? datum.m_type.IS_A(Data::FromBehaviorContextType(azrtti_typeid<t_Value>()))
                        ? (*AZStd::any_cast<BehaviorContextObjectPtr>(&datum.m_datumStorage))->CastConst<t_Value>()
                        : nullptr
                    : datum.m_type.IS_A(Data::FromAZType(azrtti_typeid<t_Value>()))
                        ? AZStd::any_cast<const t_Value>(&datum.m_datumStorage)
                        : nullptr;
            }
        };
                
        template<typename t_Value>
        struct GetAsHelper<t_Value*>
        {
            AZ_FORCE_INLINE static const t_Value** Help(Datum& datum)
            {
                datum.m_pointer = const_cast<void*>(static_cast<const void*>(const_cast<t_Value*>(GetAsHelper<typename std::remove_cv_t<typename AZStd::remove_pointer<t_Value>::type>>::Help(datum))));
                return (const t_Value**)(&datum.m_pointer);
            }
        };
        
        const bool m_isUntypedStorage = false;
        // eOriginality records the graph source of the object
        eOriginality m_originality = eOriginality::Copy;
        // storage for the datum, regardless of ScriptCanvas::Data::Type
        AZStd::any m_datumStorage;
        // This contains the editor label for m_datumStorage.
        AZ::Edit::AttributeData<AZStd::string> m_datumElementDataAttributeLabel;
        // This contains the editor visibility for m_datumStorage.
        AZ::Edit::AttributeData<AZ::Crc32> m_datumElementDataAttributeVisibility;
        AZ::Edit::ElementData m_datumElementData;
        // storage for implicit conversions, when needed
        AZStd::any m_conversionStorage;
        // the most 
        const AZ::BehaviorClass* m_class = nullptr;
        // storage for pointer, if necessary
        mutable void* m_pointer = nullptr;
        // the ScriptCanvas type of the object
        Data::Type m_type;

        // The notificationId to send change notifications to.
        AZ::EntityId m_notificationId;

        explicit Datum(const bool isUntyped);
    }; // class Datum

    AZ::Outcome<Datum, AZStd::string> Datum::CallBehaviorContextMethodResult(AZ::BehaviorMethod* method, const AZ::BehaviorParameter* resultType, AZ::BehaviorValueParameter* params, unsigned int numExpectedArgs)
    {
        AZ_Assert(resultType, "const AZ::BehaviorParameter* resultType == nullptr in Datum");
        AZ_Assert(method, "AZ::BehaviorMethod* method == nullptr in Datum");
        // create storage for the destination of the results in the function call...
        Datum resultDatum = CreateBehaviorContextMethodResult(*resultType);
        //...and initialize a AZ::BehaviorValueParameter to wrap the storage...
        AZ::Outcome<AZ::BehaviorValueParameter, AZStd::string> parameter = resultDatum.ToBehaviorValueParameterResult(*resultType);
        if (parameter.IsSuccess())
        {
            // ...the result of Call here will write back to it...
            if (method->Call(params, numExpectedArgs, &parameter.GetValue()))
            {
                // ...convert the storage, in case the function call result was one of many AZ:::RTTI types mapped to one SC::Type
                resultDatum.ConvertBehaviorContextMethodResult(*resultType);
                return AZ::Success(resultDatum);
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Script Canvas call of %s failed", method->m_name.c_str()));
            }
        }
        else
        {
            // parameter conversion failed
            return AZ::Failure(parameter.GetError());
        }
    }

    template<typename t_Value, bool isPointer, bool forceReference>
    Datum Datum::CreateInitializedCopy(const t_Value& value)
    {
        return CreateInitializedCopyHelper<t_Value, isPointer, forceReference>::Help(value);
    }

    template<typename t_Value>
    Datum Datum::CreateInitializedCopyFromBehaviorContext(const t_Value& value)
    {
		// delete this by the end of memory model II, it is only used for unit testing
        return Datum(Data::FromBehaviorContextType(azrtti_typeid<t_Value>()), Datum::eOriginality::Copy, reinterpret_cast<const void*>(&value), azrtti_typeid<t_Value>());
    }

    bool Datum::Empty() const 
    { 
        return GetValueAddress() == nullptr;
    }

    template<typename t_Value>
    const t_Value* Datum::GetAs() const
    {
        return GetAsHelper<t_Value>::Help(*const_cast<Datum*>(this));
    }

#define DATUM_GET_NUMBER_SPECIALIZE(NUMERIC_TYPE)\
        template<>\
        struct Datum::GetAsHelper<NUMERIC_TYPE>\
        {\
            AZ_FORCE_INLINE static const NUMERIC_TYPE* Help(Datum& datum)\
            {\
                AZ_STATIC_ASSERT(!AZStd::is_pointer<NUMERIC_TYPE>::value, "no pointer types in the Datum::GetAsHelper<" #NUMERIC_TYPE ">");\
                void* numberStorage(const_cast<void*>(reinterpret_cast<const void*>(&datum.m_conversionStorage)));\
                return datum.IS_A(Data::Type::Number()) && datum.ToBehaviorContextNumber(numberStorage, AZ::AzTypeInfo<NUMERIC_TYPE>::Uuid())\
                    ? reinterpret_cast<const NUMERIC_TYPE*>(numberStorage)\
                    : nullptr;\
            }\
        };

    DATUM_GET_NUMBER_SPECIALIZE(char);
    DATUM_GET_NUMBER_SPECIALIZE(short);
    DATUM_GET_NUMBER_SPECIALIZE(int);
    DATUM_GET_NUMBER_SPECIALIZE(long);
    DATUM_GET_NUMBER_SPECIALIZE(AZ::s8);
    DATUM_GET_NUMBER_SPECIALIZE(AZ::s64);
    DATUM_GET_NUMBER_SPECIALIZE(unsigned char);
    DATUM_GET_NUMBER_SPECIALIZE(unsigned int);
    DATUM_GET_NUMBER_SPECIALIZE(unsigned long);
    DATUM_GET_NUMBER_SPECIALIZE(unsigned short);
    DATUM_GET_NUMBER_SPECIALIZE(AZ::u64);
    DATUM_GET_NUMBER_SPECIALIZE(float);
    // only requred if the ScriptCanvas::NumberType changes from double, see set specialization below
    // DATUM_GET_NUMBER_SPECIALIZE(double); 
    DATUM_GET_NUMBER_SPECIALIZE(AZ::VectorFloat);

    const void* Datum::GetAsDanger() const
    {
        return GetValueAddress();
    }

    template<typename t_Value>
    t_Value* Datum::ModAs()
    {
        return const_cast<t_Value*>(GetAs<t_Value>());
    }

    void* Datum::ModAsDanger()
    {
        return ModValueAddress();
    }
        
    template<typename t_Value>
    bool Datum::IS_A() const
    {
        AZ_STATIC_ASSERT(!AZStd::is_pointer<t_Value>::value, "no pointer types in the Datum::Is, please");
        return m_type.GetType() == Data::eType::BehaviorContextObject
            ? m_type.IS_A(Data::FromBehaviorContextType(azrtti_typeid<t_Value>()))
            : m_type.IS_A(Data::FromAZType(azrtti_typeid<t_Value>()));
    }

    bool Datum::IS_A(const Data::Type& type) const
    {
        return Data::IS_A(m_type, type);
    }

    bool Datum::IsConvertibleFrom(const AZ::Uuid& typeID) const
    {
        return m_type.IsConvertibleFrom(typeID);
    }

    bool Datum::IsConvertibleFrom(const Data::Type& type) const
    {
        return m_type.IsConvertibleTo(type);
    }

    bool Datum::IsConvertibleTo(const AZ::Uuid& typeID) const
    {
        return m_type.IsConvertibleTo(typeID);
    }

    bool Datum::IsConvertibleTo(const Data::Type& type) const
    {
        return m_type.IsConvertibleTo(type);
    }

    bool Datum::InitializeUntypedStorage(const Data::Type& type)
    {
        return m_isUntypedStorage && type.IsValid() && (m_type.IS_EXACTLY_A(type) || Initialize(type, eOriginality::Copy, nullptr, AZ::Uuid::CreateNull()));
    }

    bool Datum::SatisfiesTraits(AZ::u8 behaviorValueTraits) const
    {
        AZ_Assert(!(behaviorValueTraits & AZ::BehaviorParameter::TR_POINTER && behaviorValueTraits & AZ::BehaviorParameter::TR_REFERENCE), "invalid traits on behavior parameter");
        return GetValueAddress() || (!(behaviorValueTraits & AZ::BehaviorParameter::TR_THIS_PTR) && (behaviorValueTraits & AZ::BehaviorParameter::TR_POINTER));
    }

    template<typename t_Value>
    bool Datum::Set(const t_Value& value)
    {
        AZ_STATIC_ASSERT(!AZStd::is_pointer<t_Value>::value, "no pointer types in the Datum::Set, please");
        InitializeUntypedStorage(Data::FromAZType(azrtti_typeid<t_Value>()));
        AZ_Error("Script Canvas", !IS_A(Data::Type::Number()) || azrtti_typeid<t_Value>() == azrtti_typeid<Data::NumberType>(), "Set on number types must be specialized!");

        if (IS_A<t_Value>())
        {
            if (Data::IsValueType(m_type))
            {
                m_datumStorage = value;
                OnDatumChanged();
                return true;
            }
            else
            {
                return FromBehaviorContext(static_cast<const void*>(&value));
            }
        }

        return false;
    }

    template<typename t_Value>
    bool Datum::SetFromBehaviorContext(const t_Value& value)
    {
        AZ_STATIC_ASSERT(!AZStd::is_pointer<t_Value>::value, "no pointer types in the Datum::Set, please");
        const Data::Type valueType = Data::FromBehaviorContextType(azrtti_typeid<t_Value>());
        InitializeUntypedStorage(valueType);
        AZ_Error("Script Canvas", !IS_A(Data::Type::Number()) || azrtti_typeid<t_Value>() == azrtti_typeid<Data::NumberType>(), "SetFromBehaviorContext on number types must be specialized!");

        if (IS_A(valueType))
        {
            if (Data::IsValueType(m_type))
            {
                m_datumStorage = value;
                OnDatumChanged();
                return true;
            }
            else
            {
                return FromBehaviorContext(static_cast<const void*>(&value), azrtti_typeid<t_Value>());
            }
        }

        return false;
    }

#define DATUM_SET_NUMBER_SPECIALIZE(NUMERIC_TYPE)\
    template<>\
    AZ_INLINE bool Datum::Set(const NUMERIC_TYPE& value)\
    {\
        if (FromBehaviorContextNumber(&value, azrtti_typeid<NUMERIC_TYPE>()))\
        {\
            OnDatumChanged();\
            return true;\
        }\
        return false;\
    }\
    template<>\
    AZ_INLINE bool Datum::SetFromBehaviorContext(const NUMERIC_TYPE& value)\
    {\
        return Set<NUMERIC_TYPE>(value);\
    }

    DATUM_SET_NUMBER_SPECIALIZE(char);
    DATUM_SET_NUMBER_SPECIALIZE(short);
    DATUM_SET_NUMBER_SPECIALIZE(int);
    DATUM_SET_NUMBER_SPECIALIZE(long);
    DATUM_SET_NUMBER_SPECIALIZE(AZ::s8);
    DATUM_SET_NUMBER_SPECIALIZE(AZ::s64);
    DATUM_SET_NUMBER_SPECIALIZE(unsigned char);
    DATUM_SET_NUMBER_SPECIALIZE(unsigned int);
    DATUM_SET_NUMBER_SPECIALIZE(unsigned long);
    DATUM_SET_NUMBER_SPECIALIZE(unsigned short);
    DATUM_SET_NUMBER_SPECIALIZE(AZ::u64);
    DATUM_SET_NUMBER_SPECIALIZE(float);
    // only requried if the ScriptCanvas::NumberType changes from double, see get specialization above
    // DATUM_SET_NUMBER_SPECIALIZE(double);
    DATUM_SET_NUMBER_SPECIALIZE(AZ::VectorFloat);
    
    // vectors are the most convertible objects, so more get/set specialization is necessary
#define DATUM_SET_VECTOR_SPECIALIZE(VECTOR_TYPE)\
    template<>\
    AZ_INLINE bool Datum::Set(const VECTOR_TYPE& value)\
    {\
        if (FromBehaviorContext(&value, azrtti_typeid<VECTOR_TYPE>()))\
        {\
            OnDatumChanged();\
            return true;\
        }\
        return false;\
    }\
    template<>\
    AZ_INLINE bool Datum::SetFromBehaviorContext(const VECTOR_TYPE& value)\
    {\
        return Set<VECTOR_TYPE>(value);\
    }

    DATUM_SET_VECTOR_SPECIALIZE(AZ::Vector2);
    DATUM_SET_VECTOR_SPECIALIZE(AZ::Vector3); 
    DATUM_SET_VECTOR_SPECIALIZE(AZ::Vector4);

    AZStd::string Datum::ToString() const
    {
        AZStd::string result;
        ToString(result);
        return result;
    }

} // namespace ScriptCanvas
