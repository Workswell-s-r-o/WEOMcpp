#include "core/properties/valueadapterusbstringdescriptor.h"

namespace core
{

class ValueAdapterUsbStringDescriptorImplDummy : public ValueAdapterUsbStringDescriptor::ValueAdapterUsbStringDescriptorImpl
{
public:
    ValueAdapterUsbStringDescriptorImplDummy(uint16_t vendorId, uint16_t productId, uint8_t stringDescriptorIndex, std::shared_ptr<connection::IDataLinkInterface> datalink)
    {}

    ValueResult<std::string> getValue() override
    {
        return ValueResult<std::string>::createError("Not implemented");
    }
};

ValueAdapterUsbStringDescriptor::ValueAdapterUsbStringDescriptorImpl* createValueAdapterUsbStringDescriptorImpl(uint16_t vendorId, uint16_t productId, uint8_t stringDescriptorIndex, std::shared_ptr<connection::IDataLinkInterface> datalink)
{
    return new ValueAdapterUsbStringDescriptorImplDummy(vendorId, productId, stringDescriptorIndex, datalink);
}

}
