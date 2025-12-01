#ifndef CORE_PROPERTYADAPTERVALUEDERIVEDFROM2_H
#define CORE_PROPERTYADAPTERVALUEDERIVEDFROM2_H

#include "core/properties/propertyadaptervaluederived.h"


namespace core
{

template<class ValueType, class Source1ValueType, class Source2ValueType>
class PropertyAdapterValueDerivedFrom2 : public PropertyAdapterValueDerived<ValueType>
{
    using BaseClass = PropertyAdapterValueDerived<ValueType>;

public:
    using GetValueFunction = std::function<OptionalResult<ValueType> (const OptionalResult<Source1ValueType>& source1Value,
                                                                      const OptionalResult<Source2ValueType>& source2Value,
                                                                      const PropertyValues::Transaction& transaction)>;

    explicit PropertyAdapterValueDerivedFrom2(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                              const std::shared_ptr<PropertyValues>& propertyValues,
                                              const std::shared_ptr<PropertyAdapterValue<Source1ValueType>>& source1Adapter,
                                              const std::shared_ptr<PropertyAdapterValue<Source2ValueType>>& source2Adapter,
                                              const GetValueFunction& getValueFunction);
protected:
    virtual OptionalResult<ValueType> getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                   const PropertyValues::Transaction& transaction) const override;

private:
    GetValueFunction m_getValueFunction;
};

// Impl

template<class ValueType, class Source1ValueType, class Source2ValueType>
PropertyAdapterValueDerivedFrom2<ValueType, Source1ValueType, Source2ValueType>::PropertyAdapterValueDerivedFrom2(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                                                                  const std::shared_ptr<PropertyValues>& propertyValues,
                                                                                                                  const std::shared_ptr<PropertyAdapterValue<Source1ValueType>>& source1Adapter,
                                                                                                                  const std::shared_ptr<PropertyAdapterValue<Source2ValueType>>& source2Adapter,
                                                                                                                  const GetValueFunction& getValueFunction) :
    BaseClass(propertyId, getStatusForDeviceFunction, propertyValues, {source1Adapter, source2Adapter}),
    m_getValueFunction(getValueFunction)
{
}

template<class ValueType, class Source1ValueType, class Source2ValueType>
OptionalResult<ValueType> PropertyAdapterValueDerivedFrom2<ValueType, Source1ValueType, Source2ValueType>::getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                                                                                        const PropertyValues::Transaction& transaction) const
{
    assert(sourcePropertyIds.size() == 2);
    return m_getValueFunction(transaction.getValue<Source1ValueType>(sourcePropertyIds.at(0)),
                              transaction.getValue<Source2ValueType>(sourcePropertyIds.at(1)),
                              transaction);
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUEDERIVEDFROM2_H
