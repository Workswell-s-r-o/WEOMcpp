#ifndef IDATALINKWITHBAUDRATE_H
#define IDATALINKWITHBAUDRATE_H

#include "core/connection/idatalinkinterface.h"
#include "core/device.h"


namespace core
{
namespace connection
{

class IDataLinkWithBaudrate : public IDataLinkInterface
{
public:
    [[nodiscard]] virtual ValueResult<Baudrate::Item> getBaudrate() const = 0;
    [[nodiscard]] virtual VoidResult setBaudrate(Baudrate::Item baudrate) = 0;
};

} // namespace connection
} // namespace core

#endif // IDATALINKWITHBAUDRATE_H
