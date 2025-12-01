#ifndef CORE_TRANSACTIONCHANGES_H
#define CORE_TRANSACTIONCHANGES_H

#include "core/properties/propertyid.h"
#include "core/misc/result.h"

#include <set>
#include <algorithm>


namespace core
{

class TransactionChanges
{
public:
    explicit TransactionChanges(const std::set<PropertyId>& propertiesStatusChanged,
                                const std::set<PropertyId>& propertiesValueChanged,
                                const std::set<PropertyId>& propertiesValueWritten,
                                const std::map<PropertyId, VoidResult>& propertiesLastWriteErrors,
                                bool connectionChanged);

    bool statusChanged(PropertyId propertyId) const;
    bool valueChanged(PropertyId propertyId) const;
    bool valueWritten(PropertyId propertyId) const;

    const std::map<PropertyId, VoidResult>& getPropertiesLastWriteErrors() const;

    template<class IteratorType>
    bool anyStatusChanged(IteratorType propertyIdsBegin, IteratorType propertyIdsEnd) const;

    template<class IteratorType>
    bool anyValueChanged(IteratorType propertyIdsBegin, IteratorType propertyIdsEnd) const;

    bool connectionChanged() const;

    bool isEmpty() const;

    std::string toString() const;

    const std::set<PropertyId>& getPropertiesStatusChanged() const;
    const std::set<PropertyId>& getPropertiesValueChanged() const;
    const std::set<PropertyId>& getPropertiesValueWritten() const;

private:
    std::set<PropertyId> m_propertiesStatusChanged;
    std::set<PropertyId> m_propertiesValueChanged;
    std::set<PropertyId> m_propertiesValueWritten;
    std::map<PropertyId, VoidResult> m_propertiesLastWriteErrors;

    bool m_connectionChanged {false};
};

// Impl

template<class IteratorType>
bool TransactionChanges::anyStatusChanged(IteratorType propertyIdsBegin, IteratorType propertyIdsEnd) const
{
    return std::find_if(propertyIdsBegin, propertyIdsEnd, [this](const PropertyId propertyId) { return statusChanged(propertyId); }) != propertyIdsEnd;
}

template<class IteratorType>
bool TransactionChanges::anyValueChanged(IteratorType propertyIdsBegin, IteratorType propertyIdsEnd) const
{
    return std::find_if(propertyIdsBegin, propertyIdsEnd, [this](const PropertyId propertyId) { return valueChanged(propertyId); }) != propertyIdsEnd;
}

} // namespace core

#endif // CORE_TRANSACTIONCHANGES_H
