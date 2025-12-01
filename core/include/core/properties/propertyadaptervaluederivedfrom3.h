#ifndef CORE_PROPERTYADAPTERVALUEDERIVEDFROM3_H
#define CORE_PROPERTYADAPTERVALUEDERIVEDFROM3_H

#include "core/properties/propertyadaptervaluederived.h"


namespace core
{

template<class ValueType, class Source1ValueType, class Source2ValueType, class Source3ValueType>
class PropertyAdapterValueDerivedFrom3 : public PropertyAdapterValueDerived<ValueType>
{
    using BaseClass = PropertyAdapterValueDerived<ValueType>;

public:
    using GetValueFunction = std::function<OptionalResult<ValueType> (const OptionalResult<Source1ValueType>& source1Value,
                                                                      const OptionalResult<Source2ValueType>& source2Value,
                                                                      const OptionalResult<Source3ValueType>& source3Value,
                                                                      const PropertyValues::Transaction& transaction)>;

    explicit PropertyAdapterValueDerivedFrom3(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                              const std::shared_ptr<PropertyValues>& propertyValues,
                                              const std::shared_ptr<PropertyAdapterValue<Source1ValueType>>& source1Adapter,
                                              const std::shared_ptr<PropertyAdapterValue<Source2ValueType>>& source2Adapter,
                                              const std::shared_ptr<PropertyAdapterValue<Source3ValueType>>& source3Adapter,
                                              const GetValueFunction& getValueFunction);
protected:
    virtual OptionalResult<ValueType> getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                   const PropertyValues::Transaction& transaction) const override;

private:
    GetValueFunction m_getValueFunction;
};

// Impl

template<class ValueType, class Source1ValueType, class Source2ValueType, class Source3ValueType>
PropertyAdapterValueDerivedFrom3<ValueType, Source1ValueType, Source2ValueType, Source3ValueType>::PropertyAdapterValueDerivedFrom3(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                                                                                    const std::shared_ptr<PropertyValues>& propertyValues,
                                                                                                                                    const std::shared_ptr<PropertyAdapterValue<Source1ValueType>>& source1Adapter,
                                                                                                                                    const std::shared_ptr<PropertyAdapterValue<Source2ValueType>>& source2Adapter,
                                                                                                                                    const std::shared_ptr<PropertyAdapterValue<Source3ValueType>>& source3Adapter,
                                                                                                                                    const GetValueFunction& getValueFunction) :
    BaseClass(propertyId, getStatusForDeviceFunction, propertyValues, {source1Adapter, source2Adapter, source3Adapter}),
    m_getValueFunction(getValueFunction)
{
}

template<class ValueType, class Source1ValueType, class Source2ValueType, class Source3ValueType>
OptionalResult<ValueType> PropertyAdapterValueDerivedFrom3<ValueType, Source1ValueType, Source2ValueType, Source3ValueType>::getValueFromSourceProperties(const std::vector<PropertyId>& sourcePropertyIds,
                                                                                                                                                          const PropertyValues::Transaction& transaction) const
{
    assert(sourcePropertyIds.size() == 3);
    return m_getValueFunction(transaction.getValue<Source1ValueType>(sourcePropertyIds.at(0)),
                              transaction.getValue<Source2ValueType>(sourcePropertyIds.at(1)),
                              transaction.getValue<Source3ValueType>(sourcePropertyIds.at(2)),
                              transaction);
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUEDERIVEDFROM3_H
