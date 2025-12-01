#include "core/properties/transactionchanges.h"
#include "core/utils.h"

namespace core
{

TransactionChanges::TransactionChanges(const std::set<PropertyId>& propertiesStatusChanged,
                                       const std::set<PropertyId>& propertiesValueChanged,
                                       const std::set<PropertyId>& propertiesValueWritten,
                                       const std::map<PropertyId, VoidResult>& propertiesLastWriteErrors,
                                       bool connectionChanged) :
    m_propertiesStatusChanged(propertiesStatusChanged),
    m_propertiesValueChanged(propertiesValueChanged),
    m_propertiesValueWritten(propertiesValueWritten),
    m_propertiesLastWriteErrors(propertiesLastWriteErrors),
    m_connectionChanged(connectionChanged)
{
}

bool TransactionChanges::statusChanged(PropertyId propertyId) const
{
    return m_propertiesStatusChanged.contains(propertyId);
}

bool TransactionChanges::valueChanged(PropertyId propertyId) const
{
    return m_propertiesValueChanged.contains(propertyId);
}

bool TransactionChanges::valueWritten(PropertyId propertyId) const
{
    return m_propertiesValueWritten.contains(propertyId);
}

const std::map<PropertyId, VoidResult>& TransactionChanges::getPropertiesLastWriteErrors() const
{
    return m_propertiesLastWriteErrors;
}

bool TransactionChanges::connectionChanged() const
{
    return m_connectionChanged;
}

bool TransactionChanges::isEmpty() const
{
    return m_propertiesStatusChanged.empty() && m_propertiesValueChanged.empty() && m_propertiesValueWritten.empty() &&
            m_propertiesLastWriteErrors.empty() && !m_connectionChanged;
}

std::string TransactionChanges::toString() const
{
    std::vector<std::string> list;

    auto addStr = [&](const std::string& name, const std::set<PropertyId>& properties)
    {
        std::vector<std::string> propertiesList;
        for (const auto& property : properties)
        {
            propertiesList.push_back(property.getIdString());
        }

        if (!propertiesList.empty())
        {
            list.push_back(utils::format("{} [{}]", name,  utils::joinStringVector(propertiesList, ", ")));
        }
    };

    addStr("Status:", m_propertiesStatusChanged);
    addStr("Value:", m_propertiesValueChanged);
    addStr("Written:", m_propertiesValueWritten);

    list.push_back(utils::format("writeErrors: {}, connectionChanged: {}", m_propertiesLastWriteErrors.size(), m_connectionChanged ? "Y" : "N"));

    return utils::joinStringVector(list, "\n");
}

const std::set<PropertyId>& TransactionChanges::getPropertiesStatusChanged() const
{
    return m_propertiesStatusChanged;
}

const std::set<PropertyId>& TransactionChanges::getPropertiesValueChanged() const
{
    return m_propertiesValueChanged;
}

const std::set<PropertyId>& TransactionChanges::getPropertiesValueWritten() const
{
    return m_propertiesValueWritten;
}

} // namespace core
