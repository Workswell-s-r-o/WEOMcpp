#ifndef CORE_PROPERTYADAPTERVALUEDEVICECOMPOSITE_H
#define CORE_PROPERTYADAPTERVALUEDEVICECOMPOSITE_H

#include "core/properties/propertyadaptervaluedevice.h"


namespace core
{

template<class ValueType, template <class> class BaseAdapter>
class PropertyAdapterValueDeviceComposite : public BaseAdapter<ValueType>
{
    using BaseClass = BaseAdapter<ValueType>;
    static_assert(std::is_base_of_v<PropertyAdapterValueDevice<ValueType>, BaseClass>, "Invalid base class!");

public:
    using CompositeType = ValueType;

    explicit PropertyAdapterValueDeviceComposite(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                 const Properties::AdapterTaskCreator& taskCreator,
                                                 const connection::AddressRanges& addressRanges,
                                                 const typename BaseClass::ValueReader& valueReader,
                                                 const typename BaseClass::ValueWriter& valueWriter);

    using ComponentTransformFunction = std::function<ValueType (const ValueType&, const PropertyValues::Transaction&)>;
    void addComponentTransformFunction(const ComponentTransformFunction& componentTransformFunction);

    using ComponentValidationForWriteFunction = std::function<RankedValidationResult (const ValueType&, const PropertyValues::Transaction&)>;
    void addComponentValidationForWriteFunction(const ComponentValidationForWriteFunction& componentValidationForWriteFunction);

    ValueResult<ValueType> getCurrentCompositeValueForWrite(const PropertyValues::Transaction& transaction) const;

    const ValueType& getCompositeValueDefault() const;
    void setCompositeValueDefault(const ValueType& compositeValueDefault);

    virtual RankedValidationResult validateValueForWrite(const ValueType& value, const PropertyValues::Transaction& transaction) const override;

protected:
    virtual void addWriteTask(const ValueType& value, const PropertyValues::Transaction& transaction) override;
    virtual void beforeValueUpdate(const OptionalResult<ValueType>& newValue, const PropertyValues::Transaction& transaction) override;

private:
    ValueType getTransformedValue(const ValueType& value, const PropertyValues::Transaction& transaction);

    std::vector<ComponentTransformFunction> m_componentTransformFunctions;
    std::vector<ComponentValidationForWriteFunction> m_componentValidationForWriteFunctions;

    std::optional<ValueType> m_lastWrittenValue;
    OptionalResult<ValueType> m_firstReadValue;
    ValueType m_compositeValueDefault;
};

// Impl

template<class ValueType, template <class> class BaseAdapter>
PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::PropertyAdapterValueDeviceComposite(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                                                 const Properties::AdapterTaskCreator& taskCreator,
                                                                                                 const connection::AddressRanges& addressRanges,
                                                                                                 const typename BaseClass::ValueReader& valueReader,
                                                                                                 const typename BaseClass::ValueWriter& valueWriter) :
    BaseClass(propertyId, getStatusForDeviceFunction,
              taskCreator,
              addressRanges,
              valueReader,
              valueWriter,
              [this](const ValueType& value, const PropertyValues::Transaction& transaction) { return getTransformedValue(value, transaction); })
{
    PropertyAdapterBase::statusChanged.connect([this](size_t propertyInternalId, PropertyAdapterBase::Status newStatus)
    {
        assert(propertyInternalId == this->getPropertyId().getInternalId());
        if (!this->isWritableStatus(newStatus))
        {
            m_lastWrittenValue = std::nullopt;
        }

        if (!this->isReadableStatus(newStatus))
        {
            m_firstReadValue = std::nullopt;
        }
    });
}

template<class ValueType, template <class> class BaseAdapter>
void PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::addComponentTransformFunction(const ComponentTransformFunction& componentTransformFunction)
{
    m_componentTransformFunctions.push_back(componentTransformFunction);
}

template<class ValueType, template <class> class BaseAdapter>
void PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::addComponentValidationForWriteFunction(const ComponentValidationForWriteFunction& componentValidationForWriteFunction)
{
    m_componentValidationForWriteFunctions.push_back(componentValidationForWriteFunction);
}

template<class ValueType, template <class> class BaseAdapter>
ValueResult<ValueType> PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::getCurrentCompositeValueForWrite(const PropertyValues::Transaction& transaction) const
{
    using ResultType = ValueResult<ValueType>;

    if (m_lastWrittenValue.has_value())
    {
        return m_lastWrittenValue.value();
    }

    if (!m_firstReadValue.containsValue())
    {
        if (m_firstReadValue.containsError() || !this->isReadable(transaction))
        {
            return m_compositeValueDefault;
        }

        return ResultType::createError("Unable to write", "composite value not ready");
    }

    const auto validationResult = this->validateValueForWrite(m_firstReadValue.getValue(), transaction);
    if (validationResult.isAcceptable())
    {
        return m_firstReadValue.getValue();
    }

    using ErrorRank = core::RankedValidationResult::ErrorRank;

    switch (validationResult.getErrorRank().value())
    {
        case ErrorRank::DATA_FOR_VALIDATION_NOT_READY:
            return ResultType::createError("Unable to write", "not ready for validation");

        case ErrorRank::FATAL_ERROR:
            return m_compositeValueDefault;

        default:
            assert(false);
    }

    return m_compositeValueDefault;
}

template<class ValueType, template <class> class BaseAdapter>
const ValueType& PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::getCompositeValueDefault() const
{
    return m_compositeValueDefault;
}

template<class ValueType, template <class> class BaseAdapter>
void PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::setCompositeValueDefault(const ValueType& compositeValueDefault)
{
    m_compositeValueDefault = compositeValueDefault;
}

template<class ValueType, template <class> class BaseAdapter>
RankedValidationResult PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::validateValueForWrite(const ValueType& value, const PropertyValues::Transaction& transaction) const
{
    std::optional<RankedValidationResult> warning;
    for (const auto& componentValidationForWriteFunction : m_componentValidationForWriteFunctions)
    {
        const auto result  = componentValidationForWriteFunction(value, transaction);
        if (!result.isAcceptable())
        {
            return result;
        }

        if (!warning.has_value() && !result.getResult().isOk())
        {
            assert(result.getErrorRank() == RankedValidationResult::ErrorRank::WARNING);
            warning = result;
        }
    }

    const auto baseClassResult = BaseClass::validateValueForWrite(value, transaction);
    if (!warning.has_value() || !baseClassResult.isAcceptable())
    {
        return baseClassResult;
    }

    return warning.value();
}

template<class ValueType, template <class> class BaseAdapter>
void PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::addWriteTask(const ValueType& value, const PropertyValues::Transaction& transaction)
{
    assert(this->validateValueForWrite(value, transaction).isAcceptable());

    m_lastWrittenValue = value;

    BaseClass::addWriteTask(value, transaction);
}

template<class ValueType, template <class> class BaseAdapter>
void PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::beforeValueUpdate(const OptionalResult<ValueType>& newValue, const PropertyValues::Transaction& transaction)
{
    BaseClass::beforeValueUpdate(newValue, transaction);

    if (!m_firstReadValue.hasResult())
    {
        m_firstReadValue = newValue;
    }
}

template<class ValueType, template <class> class BaseAdapter>
ValueType PropertyAdapterValueDeviceComposite<ValueType, BaseAdapter>::getTransformedValue(const ValueType& value, const PropertyValues::Transaction& transaction)
{
    auto transformedValue = value;

    for (const auto& componentTransformFunction : m_componentTransformFunctions)
    {
        transformedValue = componentTransformFunction(transformedValue, transaction);
    }

    return transformedValue;
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUEDEVICECOMPOSITE_H
