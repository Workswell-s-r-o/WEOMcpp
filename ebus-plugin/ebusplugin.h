#ifndef EBUSPLUGIN_H
#define EBUSPLUGIN_H

#include "core/connection/iebusplugin.h"
#include "core/stream/idatalinkwithbaudrateandstreamsource.h"
#include "core/logging.h"

namespace core
{
namespace connection
{

class EbusPlugin : public IEbusPlugin
{
public:
    [[nodiscard]] virtual std::vector<EbusDevice> findDevices() override;
    [[nodiscard]] virtual VoidResult setIpAddress(const EbusDevice& device, const NetworkSettings& settings) override;
    [[nodiscard]] virtual ValueResult<std::shared_ptr<IDataLinkWithBaudrateAndStreamSource>> createConnection(const EbusDevice& device, Baudrate::Item baudrate, EbusSerialPort port) override;
    static std::shared_ptr<IEbusPlugin> create(logging::ChannelFilters* parentChannelFilters);
};

} // namespace connection
} // namespace core

#endif // EBUSPLUGIN_H
