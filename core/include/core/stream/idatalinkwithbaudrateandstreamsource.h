#ifndef CORE_IDATALINKWITHBAUDRATEANDSTREAMSOURCE_H
#define CORE_IDATALINKWITHBAUDRATEANDSTREAMSOURCE_H

#include <core/stream/istreamsource.h>
#include <core/connection/idatalinkwithbaudrate.h>


namespace core
{

class IDataLinkWithBaudrateAndStreamSource : public connection::IDataLinkWithBaudrate, public IStreamSource
{
};

} // namespace core

#endif // CORE_IDATALINKWITHBAUDRATEANDSTREAMSOURCE_H
