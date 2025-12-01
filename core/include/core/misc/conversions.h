#ifndef CORE_CONVERSIONS_H
#define CORE_CONVERSIONS_H

#include <string>

namespace core
{

class Conversions
{
    Conversions();
public:

    static std::string getTimeAsString(long timeInMilliseconds);
};

} // namespace core

#endif // CORE_CONVERSIONS_H
