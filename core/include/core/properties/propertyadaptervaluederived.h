#ifndef CORE_PROPERTYADAPTERDERIVED_H
#define CORE_PROPERTYADAPTERDERIVED_H

#include "core/properties/propertyadaptervalue.h"


namespace core
{

template<class ValueType>
class PropertyAdapterValueDerived : public PropertyAdapterValue<ValueType>
{
    using BaseClass = PropertyAdapterValue<ValueType>;

public:
    explicit PropertyAdapterValueDerived(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                         const std::shared_ptr<PropertyValues>& propertyValues,
                                         const std::vector<std::shared_ptr<PropertyAdapterBase>>& sourceAdapters);
    ~PropertyAdapterValueDerived();

    virtual void refreshValue(const PropertyValues::Transaction& transaction) override;
    virtual OptionalResult<ValueType> getValue(const PropertyValues::Transaction& transaction) override;
    virtual VoidResult setValue(const ValueType& newValue, const PropertyValues::Transaction& transaction) override;
    virtual VoidResult getLastWriteResult() const override;
    virtual std::set<PropertyId> getSourcePropertyIds() const override;

protected:
    virtual OptionalResult<ValueType> getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                   const PropertyValues::Transaction& transaction) const = 0;

    void addSourceAdapter(const std::shared_ptr<PropertyAdapterBase>& sourceAdapter);
    void removeSourceAdapter(const std::shared_ptr<PropertyAdapterBase>& sourceAdapter);

private:
    void onValueChanged(size_t propertyInternalId, const PropertyValues::Transaction& transaction);

    std::vector<std::shared_ptr<PropertyAdapterBase>> m_sourceAdapters;
};

// Impl

template<class ValueType>
PropertyAdapterValueDerived<ValueType>::PropertyAdapterValueDerived(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                    const std::shared_ptr<PropertyValues>& propertyValues,
                                                                    const std::vector<std::shared_ptr<PropertyAdapterBase>>& sourceAdapters) :
    BaseClass(propertyId, getStatusForDeviceFunction)
{
    for (const auto& sourceAdapter : sourceAdapters)
    {
        addSourceAdapter(sourceAdapter);
    }

    propertyValues.get()->valueChanged.connect([this](size_t size, core::PropertyValues::Transaction transaction)
    {
        onValueChanged(size, transaction);
    });
}

template<class ValueType>
PropertyAdapterValueDerived<ValueType>::~PropertyAdapterValueDerived()
{
    for (const auto& sourceAdapter : m_sourceAdapters)
    {
        sourceAdapter->removeSubsidiaryAdaptersPropertyId(this->getPropertyId());
    }
}

template<class ValueType>
void PropertyAdapterValueDerived<ValueType>::refreshValue(const PropertyValues::Transaction& transaction)
{
    for (const auto& sourceAdapter : m_sourceAdapters)
    {
        sourceAdapter->refreshValue(transaction);
    }

    this->touchDependentProperties(transaction);
}

template<class ValueType>
OptionalResult<ValueType> PropertyAdapterValueDerived<ValueType>::getValue(const PropertyValues::Transaction& transaction)
{
    for (const auto& sourceAdapter : m_sourceAdapters)
    {
        sourceAdapter->touch(transaction);
    }

    return transaction.getValue<ValueType>(this->getPropertyId());
}

template<class ValueType>
VoidResult PropertyAdapterValueDerived<ValueType>::setValue(const ValueType& , const PropertyValues::Transaction& transaction)
{
    if (!this->isWritable(transaction))
    {
        return VoidResult::createError("Unable to write!", utils::format("adapter in non-writable mode - property: {}", this->getPropertyId().getIdString()));
    }

    assert(false && "Unable to write - derived property!");
    return VoidResult::createOk();
}

template<class ValueType>
VoidResult PropertyAdapterValueDerived<ValueType>::getLastWriteResult() const
{
    return VoidResult::createOk();
}

template<class ValueType>
std::set<PropertyId> PropertyAdapterValueDerived<ValueType>::getSourcePropertyIds() const
{
    std::set<PropertyId> sourcePropertyIds;

    for (const auto& sourceAdapter : m_sourceAdapters)
    {
        sourcePropertyIds.insert(sourceAdapter->getPropertyId());

        const auto& ids = sourceAdapter->getSourcePropertyIds();
        sourcePropertyIds.insert(ids.begin(), ids.end());
    }

    return sourcePropertyIds;
}

template<class ValueType>
void PropertyAdapterValueDerived<ValueType>::addSourceAdapter(const std::shared_ptr<PropertyAdapterBase>& sourceAdapter)
{
    m_sourceAdapters.push_back(sourceAdapter);

    sourceAdapter->addSubsidiaryAdaptersPropertyId(this->getPropertyId());
}

template<class ValueType>
void PropertyAdapterValueDerived<ValueType>::removeSourceAdapter(const std::shared_ptr<PropertyAdapterBase>& sourceAdapter)
{
    const auto it = std::find(m_sourceAdapters.begin(), m_sourceAdapters.end(), sourceAdapter);
    assert(it != m_sourceAdapters.end());

    sourceAdapter->removeSubsidiaryAdaptersPropertyId(this->getPropertyId());

    m_sourceAdapters.erase(it);
}

template<class ValueType>
void PropertyAdapterValueDerived<ValueType>::onValueChanged(size_t propertyInternalId, const PropertyValues::Transaction& transaction)
{
    if (this->isReadable(transaction))
    {
        const auto it = std::find_if(m_sourceAdapters.begin(), m_sourceAdapters.end(), [propertyInternalId](const std::shared_ptr<PropertyAdapterBase>& sourceAdapter)
        {
            return propertyInternalId == sourceAdapter->getPropertyId().getInternalId();
        });

        if (it != m_sourceAdapters.end())
        {
            std::vector<PropertyId> sourcePropertyIds;
            sourcePropertyIds.reserve(m_sourceAdapters.size());
            for (const auto& sourceAdapter : m_sourceAdapters)
            {
                sourcePropertyIds.push_back(sourceAdapter->getPropertyId());
            }

            auto newValue = getValueFromSourceProperties(sourcePropertyIds, transaction);
            if (newValue.containsValue())
            {
                const VoidResult result = transaction.validateValue(this->getPropertyId(), newValue.getValue());
                if (!result.isOk())
                {
                    newValue = OptionalResult<ValueType>::createFromError(result);
                }
            }
            this->beforeValueUpdate(newValue, transaction);

            transaction.setValue(this->getPropertyId(), newValue);

            this->touchDependentProperties(transaction);
        }
    }
}

} // namespace core

#endif // CORE_PROPERTYADAPTERDERIVED_H
