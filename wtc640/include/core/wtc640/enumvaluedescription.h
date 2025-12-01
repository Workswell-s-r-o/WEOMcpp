#ifndef CORE_ENUMVALUEDESCRIPTION_H
#define CORE_ENUMVALUEDESCRIPTION_H

#include <string>


namespace core
{

struct EnumValueDescription
{
    std::string userName;
    std::string pythonName;
};

struct EnumValueDeviceDescription : public EnumValueDescription
{
    uint32_t deviceValue;
};

} // namespace core

#endif // CORE_ENUMVALUEDESCRIPTION_H
