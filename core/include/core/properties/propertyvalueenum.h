#ifndef CORE_PROPERTYVALUEENUM_H
#define CORE_PROPERTYVALUEENUM_H

#include "core/properties/propertyvalue.h"
#include "core/utils.h"

namespace core
{

template<class ValueType>
class PropertyValueEnum : public PropertyValue<ValueType>
{
    using BaseClass = PropertyValue<ValueType>;
    static_assert(std::is_enum_v<ValueType> || std::is_integral_v<ValueType>);

public:
    using ValueToUserNameMap = std::map<ValueType, std::string>;
    explicit PropertyValueEnum(PropertyId propertyId, const ValueToUserNameMap& valueToUserNameMap,
                               const typename BaseClass::ValidationFunction& validationFunction = nullptr);

    virtual std::string convertToString(const OptionalResult<ValueType>& value) const override;

    const ValueToUserNameMap& getValueToUserNameMap() const;

private:
    ValueToUserNameMap m_valueToUserNameMap;
};

// Impl

template<class ValueType>
PropertyValueEnum<ValueType>::PropertyValueEnum(PropertyId propertyId, const ValueToUserNameMap& valueToUserNameMap, const typename BaseClass::ValidationFunction& validationFunction) :
    BaseClass(propertyId, [this, validationFunction](const ValueType& value)
                        {
                            if (m_valueToUserNameMap.find(value) == m_valueToUserNameMap.end())
                            {
                                return VoidResult::createError("Value out of range!", utils::format("value: {}", static_cast<int>(value)));
                            }

                            if (validationFunction)
                            {
                                return validationFunction(value);
                            }

                            return VoidResult::createOk();
                        }),
    m_valueToUserNameMap(valueToUserNameMap)
{
}

template<class ValueType>
std::string PropertyValueEnum<ValueType>::convertToString(const OptionalResult<ValueType>& value) const
{
    if (!value.containsValue())
    {
        return std::string();
    }

    if (this->getCustomConvertToStringFunction())
    {
        return this->getCustomConvertToStringFunction()(value.getValue());
    }

    auto it = m_valueToUserNameMap.find(value.getValue());
    if (it != m_valueToUserNameMap.end())
    {
        return it->second;
    }

    assert(false && "Value out of range");
    return std::string();
}

template<class ValueType>
const typename PropertyValueEnum<ValueType>::ValueToUserNameMap& PropertyValueEnum<ValueType>::getValueToUserNameMap() const
{
    return m_valueToUserNameMap;
}

} // namespace core

#endif // CORE_PROPERTYVALUEENUM_H
