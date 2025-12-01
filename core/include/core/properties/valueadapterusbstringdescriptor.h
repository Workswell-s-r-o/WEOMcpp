#pragma once

#include "core/properties/properties.h"
#include "core/properties/propertyadaptervaluedevice.h"

namespace core
{

class ValueAdapterUsbStringDescriptor : public core::PropertyAdapterValueDevice<std::string>
{
    using BaseClass = core::PropertyAdapterValueDevice<std::string>;
public:
    ValueAdapterUsbStringDescriptor(PropertyId propertyId,
                                    const GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                    const Properties::AdapterTaskCreator& taskCreator,
                                    uint16_t vendorId,
                                    uint16_t productId,
                                    uint8_t stringDescriptorIndex,
                                    std::shared_ptr<connection::IDataLinkInterface> datalink);
    ~ValueAdapterUsbStringDescriptor();

protected:
    virtual void addReadTask() override;
    virtual void addWriteTask(const std::string& value, const PropertyValues::Transaction& transaction) override;


public:
    class ValueAdapterUsbStringDescriptorImpl
    {
    public:
        virtual ~ValueAdapterUsbStringDescriptorImpl() = default;
        virtual ValueResult<std::string> getValue() = 0;
    };
private:
    std::unique_ptr<ValueAdapterUsbStringDescriptorImpl> m_impl;
};

}
