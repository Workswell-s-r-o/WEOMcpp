#ifndef PROPERTYVALUES_H
#define PROPERTYVALUES_H

#include "core/properties/propertyvalueenum.h"
#include "core/properties/propertyvaluearithmetic.h"
#include "core/misc/result.h"
#include "core/misc/deadlockdetectionmutex.h"


#include <set>


namespace core
{

class PropertyValueBase;

class PropertyValues
{
    explicit PropertyValues();

public:
    static std::shared_ptr<PropertyValues> createInstance();

    std::set<PropertyId> getPropertyIds() const;

    void addProperty(const std::shared_ptr<PropertyValueBase>& propertyValue);
    void removeProperty(PropertyId propertyId);

    class Transaction;

    Transaction createTransaction();

    boost::signals2::signal<void(size_t, core::PropertyValues::Transaction)> valueChanged;

private:
    void onPropertyValueChanged(size_t propertyInternalId);

    std::map<PropertyId, std::shared_ptr<PropertyValueBase>> m_values;

    class TransactionData;

    std::weak_ptr<PropertyValues> m_weakThis;
    std::weak_ptr<TransactionData> m_transactionData;
    DeadlockDetectionMutex m_mutex;
};


class PropertyValues::Transaction
{
    friend class PropertyValues;
    explicit Transaction(const std::shared_ptr<TransactionData>& transactionData);

public:
    void resetValue(PropertyId propertyId) const;

    [[nodiscard]] bool hasValueResult(PropertyId propertyId) const;
    [[nodiscard]] bool areValuesEqual(PropertyId propertyId1, PropertyId propertyId2) const;
    [[nodiscard]] VoidResult getPropertyValidationResult(PropertyId propertyId) const;
    std::string getValueAsString(PropertyId propertyId) const;

    const std::set<PropertyId>& getPropertiesChanged() const;

    template<class ValueType>
    OptionalResult<ValueType> getValue(PropertyId propertyId) const;

    template<class ValueType>
    std::map<ValueType, std::string> getValueToUserNameMap(PropertyId propertyId) const;

    template<class ValueType>
    std::vector<ValueType> getMinAndMaxValidValues(PropertyId propertyId) const;

    template<class ValueType>
    std::string convertToString(PropertyId propertyId, const ValueType& value) const;

    template<class ValueType>
    VoidResult validateValue(PropertyId propertyId, const ValueType& value) const;

    template<class ValueType>
    void setValue(PropertyId propertyId, const OptionalResult<ValueType>& newValue) const;

    PropertyValueBase* getPropertyValue(PropertyId propertyId) const;

private:
    std::shared_ptr<TransactionData> m_transactionData;
};

// Impl

template<class ValueType>
OptionalResult<ValueType> PropertyValues::Transaction::getValue(PropertyId propertyId) const
{
    if (const auto* propertyValue = dynamic_cast<const PropertyValue<ValueType>*>(getPropertyValue(propertyId)))
    {
        return propertyValue->getCurrentValue();
    }

    assert(false && "PropertyValue for different data type!");
    return std::nullopt;
}

template<class ValueType>
std::map<ValueType, std::string> PropertyValues::Transaction::getValueToUserNameMap(PropertyId propertyId) const
{
    if (const auto* propertyValue = dynamic_cast<const PropertyValueEnum<ValueType>*>(getPropertyValue(propertyId)))
    {
        return propertyValue->getValueToUserNameMap();
    }

    assert(false && "PropertyValue for different data type!");
    return {};
}

template<class ValueType>
std::vector<ValueType> PropertyValues::Transaction::getMinAndMaxValidValues(PropertyId propertyId) const
{
    if (const auto* propertyValue = dynamic_cast<const PropertyValueArithmetic<ValueType>*>(getPropertyValue(propertyId)))
    {
        return {propertyValue->getMinValidValue(), propertyValue->getMaxValidValue()};
    }

    assert(false && "PropertyValue for different data type!");
    return {};
}

template<class ValueType>
std::string PropertyValues::Transaction::convertToString(PropertyId propertyId, const ValueType& value) const
{
    if (const auto* propertyValue = dynamic_cast<const PropertyValue<ValueType>*>(getPropertyValue(propertyId)))
    {
        return propertyValue->convertToString(value);
    }

    assert(false && "PropertyValue for different data type!");
    return std::string();
}

template<class ValueType>
VoidResult PropertyValues::Transaction::validateValue(PropertyId propertyId, const ValueType& value) const
{
    if (const auto* propertyValue = dynamic_cast<const PropertyValue<ValueType>*>(getPropertyValue(propertyId)))
    {
        return propertyValue->validateValue(value);
    }

    assert(false && "PropertyValue for different data type!");
    return VoidResult::createOk();
}

template<class ValueType>
void PropertyValues::Transaction::setValue(PropertyId propertyId, const OptionalResult<ValueType>& newValue) const
{
    if (auto* propertyValue = dynamic_cast<PropertyValue<ValueType>*>(getPropertyValue(propertyId)))
    {
        return propertyValue->setCurrentValue(newValue);
    }

    assert(false && "PropertyValue for different data type!");
}

} // namespace core

#endif // PROPERTYVALUES_H
