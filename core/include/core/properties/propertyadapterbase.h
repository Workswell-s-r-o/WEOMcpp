#ifndef CORE_PROPERTYADAPTERBASE_H
#define CORE_PROPERTYADAPTERBASE_H

#include "core/properties/propertydependencyvalidator.h"
#include "core/properties/propertyvalues.h"
#include "core/properties/propertyid.h"
#include "core/device.h"
#include "core/connection/addressrange.h"

#include <boost/signals2.hpp>


namespace core
{

class PropertyAdapterBase
{

public:
    enum class Status
    {
        DISABLED,
        ENABLED_READ_ONLY,
        ENABLED_WRITE_ONLY,
        ENABLED_READ_WRITE,
    };

    using GetStatusForDeviceFunction = std::function<PropertyAdapterBase::Status (const std::optional<DeviceType>& deviceType)>;
    explicit PropertyAdapterBase(PropertyId propertyId, const GetStatusForDeviceFunction& getStatusForDeviceFunction);

    PropertyId getPropertyId() const;

    bool isReadable(const PropertyValues::Transaction& transaction) const;
    bool isWritable(const PropertyValues::Transaction& transaction) const;
    Status getStatus(const PropertyValues::Transaction& transaction) const;
    void updateStatusDeviceChanged(const std::optional<DeviceType>& currentDeviceType, const PropertyValues::Transaction& transaction);
    void updateStatusValueChanged(const PropertyValues::Transaction& transaction);

    using GetStatusConstraintByValuesFunction = std::function<PropertyAdapterBase::Status (const PropertyValues::Transaction& transaction)>;
    void setStatusConstraintByValuesFunction(const GetStatusConstraintByValuesFunction& getStatusConstraintByValuesFunction,
                                             const std::vector<std::shared_ptr<PropertyAdapterBase>>& statusConstraintAdapters,
                                             PropertyValues* propertyValues);

    virtual const std::type_info& getTypeInfo() = 0;

    bool isActiveForDeviceType(const std::optional<DeviceType>& deviceType) const;

    std::string getValueAsString(const PropertyValues::Transaction& transaction);

    void addDependencyValidator(const std::shared_ptr<PropertyDependencyValidator>& validator);
    std::vector<RankedValidationResult> getValueDependencyValidationResults() const;
    const std::set<PropertyId>& getValidationDependencyPropertyIds() const;

    virtual void touch(const PropertyValues::Transaction& transaction) = 0;
    virtual void invalidateValue(const PropertyValues::Transaction& transaction) = 0;
    virtual void refreshValue(const PropertyValues::Transaction& transaction) = 0;
    virtual VoidResult setValueAccording(PropertyAdapterBase* sourceAdapter, const PropertyValues::Transaction& transaction) = 0;
    virtual RankedValidationResult validateSourcePropertyValueForWrite(PropertyId sourcePropertyId, const PropertyValues::Transaction& transaction) const = 0;
    virtual VoidResult getLastWriteResult() const = 0;

    virtual connection::AddressRanges getAddressRanges() const;
    virtual std::set<PropertyId> getSourcePropertyIds() const;

    const std::set<PropertyId>& getSubsidiaryAdaptersPropertyIds() const;
    void addSubsidiaryAdaptersPropertyId(PropertyId propertyId);
    void removeSubsidiaryAdaptersPropertyId(PropertyId propertyId);

    static bool isReadableStatus(Status status);
    static bool isWritableStatus(Status status);

    boost::signals2::signal<void(size_t, Status)> statusChanged;
    boost::signals2::signal<void(size_t, const std::string&, const std::string&)> valueWriteFinished;
    boost::signals2::signal<void(size_t)> touchDependentProperty;

protected:
    const std::vector<std::shared_ptr<PropertyDependencyValidator>>& getDependencyValidators() const;

private:
    virtual void setStatus(Status status, const PropertyValues::Transaction& transaction);

    PropertyId m_propertyId;

    GetStatusForDeviceFunction m_getStatusForDeviceFunction;
    Status m_statusForDevice {Status::DISABLED};
    GetStatusConstraintByValuesFunction m_getStatusConstraintByValuesFunction;
    std::vector<std::shared_ptr<PropertyAdapterBase>> m_statusConstraintAdapters;
    Status m_status {Status::DISABLED};

    std::vector<std::shared_ptr<PropertyDependencyValidator>> m_dependencyValidators;
    std::set<PropertyId> m_validationDependencyPropertyIds;
    std::set<PropertyId> m_subsidiaryAdaptersPropertyIds;
    boost::signals2::scoped_connection m_valueChangedConnection;
};

} // namespace core

#endif // CORE_PROPERTYADAPTERBASE_H
