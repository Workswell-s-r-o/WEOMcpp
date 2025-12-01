#ifndef CORE_PROPERTYADAPTERVALUEDERIVEDFROM1_H
#define CORE_PROPERTYADAPTERVALUEDERIVEDFROM1_H

#include "core/properties/propertyadaptervaluederived.h"


namespace core
{

template<class ValueType, class SourceValueType>
class PropertyAdapterValueDerivedFrom1 : public PropertyAdapterValueDerived<ValueType>
{
    using BaseClass = PropertyAdapterValueDerived<ValueType>;

public:
    using GetValueFunction = std::function<OptionalResult<ValueType> (const OptionalResult<SourceValueType>& sourceValue, const PropertyValues::Transaction& transaction)>;

    explicit PropertyAdapterValueDerivedFrom1(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                              const std::shared_ptr<PropertyValues>& propertyValues,
                                              const std::shared_ptr<PropertyAdapterValue<SourceValueType>>& sourceAdapter,
                                              const GetValueFunction& getValueFunction);

protected:
    virtual OptionalResult<ValueType> getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                   const PropertyValues::Transaction& transaction) const override;

private:
    GetValueFunction m_getValueFunction;
};

// Impl

template<class ValueType, class SourceValueType>
PropertyAdapterValueDerivedFrom1<ValueType, SourceValueType>::PropertyAdapterValueDerivedFrom1(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                                               const std::shared_ptr<PropertyValues>& propertyValues,
                                                                                               const std::shared_ptr<PropertyAdapterValue<SourceValueType>>& sourceAdapter,
                                                                                               const GetValueFunction& getValueFunction) :
    BaseClass(propertyId, getStatusForDeviceFunction, propertyValues, {sourceAdapter}),
    m_getValueFunction(getValueFunction)
{
}

template<class ValueType, class SourceValueType>
OptionalResult<ValueType> PropertyAdapterValueDerivedFrom1<ValueType, SourceValueType>::getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                                                                     const PropertyValues::Transaction& transaction) const
{
    assert(sourcePropertyIds.size() == 1);
    return m_getValueFunction(transaction.getValue<SourceValueType>(sourcePropertyIds.at(0)),
                              transaction);
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUEDERIVEDFROM1_H
