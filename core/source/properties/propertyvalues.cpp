#include "core/properties/propertyvalues.h"


namespace core
{

class PropertyValues::TransactionData
{
public:
    explicit TransactionData(const std::weak_ptr<PropertyValues>& propertyValues);
    ~TransactionData();

    PropertyValueBase* getProperty(PropertyId propertyId) const;

    void addPropertyChanged(PropertyId propertyId);
    const std::set<PropertyId>& getPropertiesChanged() const;

private:
    std::shared_ptr<PropertyValues> m_propertyValues;

    std::set<PropertyId> m_propertiesValueChanged;
};


PropertyValues::PropertyValues()
{
}

std::shared_ptr<PropertyValues> PropertyValues::createInstance()
{
    auto instance = std::shared_ptr<PropertyValues>(new PropertyValues);
    instance->m_weakThis = instance;
    return instance;
}

std::set<PropertyId> PropertyValues::getPropertyIds() const
{
    std::set<PropertyId> propertyIds;

    for (const auto& value : m_values)
    {
        propertyIds.insert(value.first);
    }

    return propertyIds;
}

void PropertyValues::addProperty(const std::shared_ptr<PropertyValueBase>& propertyValue)
{
    const bool added = m_values.emplace(propertyValue->getPropertyId(), propertyValue).second;
    if (!added)
    {
        assert(false && "Property value already exists!");
        return;
    }

    propertyValue.get()->valueChanged.connect(std::bind(&PropertyValues::onPropertyValueChanged, this, std::placeholders::_1));
}

void PropertyValues::removeProperty(PropertyId propertyId)
{
    const auto it = m_values.find(propertyId);
    if (it != m_values.end())
    {
        it->second.get()->valueChanged.disconnect_all_slots();

        m_values.erase(it);
    }
}

PropertyValues::Transaction PropertyValues::createTransaction()
{
    m_mutex.lock();

    const auto transactionData = std::make_shared<TransactionData>(m_weakThis);
    assert(m_transactionData.lock() == nullptr);
    m_transactionData = transactionData;
    return Transaction(transactionData);
}

void PropertyValues::onPropertyValueChanged(size_t propertyInternalId)
{
    auto transactionData = m_transactionData.lock();
    assert(transactionData != nullptr && "Data change outside of transaction!");

    const auto propertyId = core::PropertyId::getPropertyIdByInternalId(propertyInternalId);
    assert(propertyId.has_value() && "Invalid id!");
    if (propertyId.has_value())
    {
        transactionData->addPropertyChanged(propertyId.value());
    }

    valueChanged(propertyInternalId, Transaction(transactionData));
}

PropertyValues::TransactionData::TransactionData(const std::weak_ptr<PropertyValues>& propertyValues) :
    m_propertyValues(propertyValues.lock())
{
}

PropertyValues::TransactionData::~TransactionData()
{
    m_propertyValues->m_mutex.unlock();
}

PropertyValueBase* PropertyValues::TransactionData::getProperty(PropertyId propertyId) const
{
    const auto it = m_propertyValues->m_values.find(propertyId);
    if (it == m_propertyValues->m_values.end())
    {
        assert(false && "Value not found!");
        return nullptr;
    }

    return it->second.get();
}

void PropertyValues::TransactionData::addPropertyChanged(PropertyId propertyId)
{
    m_propertiesValueChanged.insert(propertyId);
}

const std::set<PropertyId>& PropertyValues::TransactionData::getPropertiesChanged() const
{
    return m_propertiesValueChanged;
}

PropertyValues::Transaction::Transaction(const std::shared_ptr<TransactionData>& transactionData) :
    m_transactionData(transactionData)
{
}

PropertyValueBase* PropertyValues::Transaction::getPropertyValue(PropertyId propertyId) const
{
    return m_transactionData->getProperty(propertyId);
}

void PropertyValues::Transaction::resetValue(PropertyId propertyId) const
{
    m_transactionData->getProperty(propertyId)->resetValue();
}

bool PropertyValues::Transaction::hasValueResult(PropertyId propertyId) const
{
    return m_transactionData->getProperty(propertyId)->hasValueResult();
}

bool PropertyValues::Transaction::areValuesEqual(PropertyId propertyId1, PropertyId propertyId2) const
{
    return m_transactionData->getProperty(propertyId1)->valueEquals(m_transactionData->getProperty(propertyId2));
}

VoidResult PropertyValues::Transaction::getPropertyValidationResult(PropertyId propertyId) const
{
    return m_transactionData->getProperty(propertyId)->getValidationResult();
}

std::string PropertyValues::Transaction::getValueAsString(PropertyId propertyId) const
{
    return m_transactionData->getProperty(propertyId)->getValueAsString();
}

const std::set<PropertyId>& PropertyValues::Transaction::getPropertiesChanged() const
{
    return m_transactionData->getPropertiesChanged();
}

} // namespace core
