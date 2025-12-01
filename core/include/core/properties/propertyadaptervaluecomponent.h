#ifndef CORE_PROPERTYADAPTERVALUECOMPONENT_H
#define CORE_PROPERTYADAPTERVALUECOMPONENT_H

#include "core/properties/propertyadaptervalue.h"


namespace core
{

template<class ValueType, class CompositeAdapterType>
class PropertyAdapterValueComponent : public PropertyAdapterValue<ValueType>
{
    using BaseClass = PropertyAdapterValue<ValueType>;

public:
    using CompositeType = typename CompositeAdapterType::CompositeType;
    using ComponentPointerType = ValueResult<ValueType> CompositeType::*;

    explicit PropertyAdapterValueComponent(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                           const std::shared_ptr<PropertyValues>& propertyValues,
                                           const std::shared_ptr<CompositeAdapterType>& sourceCompositeAdapter,
                                           ComponentPointerType componentPtr,
                                           const ValueType& defaultValue);
    virtual ~PropertyAdapterValueComponent();

    virtual void refreshValue(const PropertyValues::Transaction& transaction) override;
    virtual OptionalResult<ValueType> getValue(const PropertyValues::Transaction& transaction) override;
    virtual VoidResult setValue(const ValueType& newValue, const PropertyValues::Transaction& transaction) override;
    virtual VoidResult getLastWriteResult() const override;
    virtual std::set<PropertyId> getSourcePropertyIds() const override;

private:
    void onValueChanged(size_t propertyInternalId, const PropertyValues::Transaction& transaction);

    std::shared_ptr<CompositeAdapterType> m_sourceCompositeAdapter;
    ComponentPointerType m_componentPtr;
};

// Impl

template<class ValueType, class CompositeAdapterType>
PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::PropertyAdapterValueComponent(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                                              const std::shared_ptr<PropertyValues>& propertyValues,
                                                                                              const std::shared_ptr<CompositeAdapterType>& sourceCompositeAdapter,
                                                                                              ComponentPointerType componentPtr,
                                                                                              const ValueType& defaultValue) :
    BaseClass(propertyId, getStatusForDeviceFunction),
    m_sourceCompositeAdapter(sourceCompositeAdapter),
    m_componentPtr(componentPtr)
{
    propertyValues.get()->valueChanged.connect(utils::autoBind(this, &PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::onValueChanged));

    m_sourceCompositeAdapter->addComponentTransformFunction([this](const CompositeType& compositeValue, const PropertyValues::Transaction& transaction)
    {
        auto transformedCompositeValue = compositeValue;

        ValueResult<ValueType>& componentValue = transformedCompositeValue.*m_componentPtr;
        if (componentValue.isOk())
        {
            const auto validationResult = this->validateValue(componentValue.getValue(), transaction);
            if (!validationResult.isOk())
            {
                componentValue = ValueResult<ValueType>::createFromError(validationResult);
            }
        }

        return transformedCompositeValue;
    });

    m_sourceCompositeAdapter->addComponentValidationForWriteFunction([this](const CompositeType& compositeValue, const PropertyValues::Transaction& transaction)
    {
        const ValueResult<ValueType>& componentValue = compositeValue.*m_componentPtr;
        if (!componentValue.isOk())
        {
            return RankedValidationResult::createError(componentValue.toVoidResult());
        }

        return this->validateValueForWrite(componentValue.getValue(), transaction);
    });

    auto compositeValue = m_sourceCompositeAdapter->getCompositeValueDefault();
    compositeValue.*m_componentPtr = defaultValue;

    m_sourceCompositeAdapter->setCompositeValueDefault(compositeValue);

    m_sourceCompositeAdapter->addSubsidiaryAdaptersPropertyId(this->getPropertyId());
}

template<class ValueType, class CompositeAdapterType>
PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::~PropertyAdapterValueComponent()
{
    m_sourceCompositeAdapter->removeSubsidiaryAdaptersPropertyId(this->getPropertyId());
}

template<class ValueType, class CompositeAdapterType>
void PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::refreshValue(const PropertyValues::Transaction& transaction)
{
    m_sourceCompositeAdapter->refreshValue(transaction);

    this->touchDependentProperties(transaction);
}

template<class ValueType, class CompositeAdapterType>
OptionalResult<ValueType> PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::getValue(const PropertyValues::Transaction& transaction)
{
    m_sourceCompositeAdapter->touch(transaction);

    return transaction.getValue<ValueType>(this->getPropertyId());
}

template<class ValueType, class CompositeAdapterType>
VoidResult PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::setValue(const ValueType& newValue, const PropertyValues::Transaction& transaction)
{
    if (!this->isWritable(transaction))
    {
        return VoidResult::createError("Unable to write!", utils::format("adapter in non-writable mode - property: {}", this->getPropertyId().getIdString()));
    }

    m_sourceCompositeAdapter->touch(transaction);

    // validation delegated to composite adapter - addComponentTransformFunction

    const auto compositeValue = m_sourceCompositeAdapter->getCurrentCompositeValueForWrite(transaction);
    if (!compositeValue.isOk())
    {
        return compositeValue.toVoidResult();
    }

    auto newCompositeValue = compositeValue.getValue();
    newCompositeValue.*m_componentPtr = newValue;

    return m_sourceCompositeAdapter->setValue(newCompositeValue, transaction);
}

template<class ValueType, class CompositeAdapterType>
VoidResult PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::getLastWriteResult() const
{
    return m_sourceCompositeAdapter->getLastWriteResult();
}

template<class ValueType, class CompositeAdapterType>
std::set<PropertyId> PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::getSourcePropertyIds() const
{
    std::set<PropertyId> sourcePropertyIds = m_sourceCompositeAdapter->getSourcePropertyIds();
    sourcePropertyIds.insert(m_sourceCompositeAdapter->getPropertyId());

    return sourcePropertyIds;
}

template<class ValueType, class CompositeAdapterType>
void PropertyAdapterValueComponent<ValueType, CompositeAdapterType>::onValueChanged(size_t propertyInternalId, const PropertyValues::Transaction& transaction)
{
    if (this->isReadable(transaction) && propertyInternalId == m_sourceCompositeAdapter->getPropertyId().getInternalId())
    {
        OptionalResult<ValueType> newValue;

        const auto compositeValue = transaction.getValue<CompositeType>(m_sourceCompositeAdapter->getPropertyId());
        if (compositeValue.containsValue())
        {
            newValue = compositeValue.getValue().*m_componentPtr;
            assert(!newValue.getResult().isOk() || transaction.validateValue(this->getPropertyId(), newValue.getValue()).isOk() && "Should be validated by composite adapter! - addComponentTransformFunction");
        }
        else if (compositeValue.containsError())
        {
            newValue = OptionalResult<ValueType>::createFromError(compositeValue.getResult());
        }
        this->beforeValueUpdate(newValue, transaction);

        transaction.setValue(this->getPropertyId(), newValue);

        this->touchDependentProperties(transaction);
    }
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUECOMPONENT_H
