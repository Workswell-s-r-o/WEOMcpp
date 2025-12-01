#include "core/properties/valueadapterusbstringdescriptor.h"

namespace core
{

// Factory function to create the OS-specific implementation.
extern ValueAdapterUsbStringDescriptor::ValueAdapterUsbStringDescriptorImpl* createValueAdapterUsbStringDescriptorImpl(uint8_t stringDescriptorIndex, std::shared_ptr<connection::IDataLinkInterface> datalink);

ValueAdapterUsbStringDescriptor::ValueAdapterUsbStringDescriptor(PropertyId propertyId,
                                                               const GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                               const Properties::AdapterTaskCreator& taskCreator,
                                                               uint16_t vendorId,
                                                               uint16_t productId,
                                                               uint8_t stringDescriptorIndex,
                                                               std::shared_ptr<connection::IDataLinkInterface> datalink)
    : BaseClass(propertyId, getStatusForDeviceFunction, taskCreator, {}) // Empty address ranges
{
    m_impl.reset(createValueAdapterUsbStringDescriptorImpl(stringDescriptorIndex, datalink));
}

ValueAdapterUsbStringDescriptor::~ValueAdapterUsbStringDescriptor() = default;

void ValueAdapterUsbStringDescriptor::addReadTask()
{
    const auto task = [this](connection::IDeviceInterface* , const Properties::AdapterTaskCreator::GetTaskResultTransactionFunction& getTransactionFunction)
    {
        const auto newValue = m_impl->getValue();
        const auto transaction = getTransactionFunction();
        this->updateValueAfterRead(newValue, transaction.getValuesTransaction());
        return newValue.toVoidResult();
    };

    this->getTaskCreator().createTaskSimpleRead({connection::AddressRange::firstAndSize(0xFFFFFFFF, 1)}, task); // Empty address ranges
}

void ValueAdapterUsbStringDescriptor::addWriteTask(const std::string& , const PropertyValues::Transaction& )
{
    // Read-only
}

}
