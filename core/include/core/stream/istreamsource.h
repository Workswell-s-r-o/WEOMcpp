#ifndef CORE_ISTREAMSOURCE_H
#define CORE_ISTREAMSOURCE_H

#include "core/stream/istream.h"

#include <memory>

namespace core
{

class IStreamSource
{
public:
    virtual ~IStreamSource() {}

    [[nodiscard]] virtual ValueResult<std::shared_ptr<IStream>> getOrCreateStream() = 0;
    [[nodiscard]] virtual ValueResult<std::shared_ptr<IStream>> getStream() = 0;
};

} // namespace core

#endif // CORE_ISTREAMSOURCE_H
