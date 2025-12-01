#include "core/misc/conversions.h"
#include "core/utils.h"

namespace core
{

std::string Conversions::getTimeAsString(long timeInMilliseconds)
{
    if (timeInMilliseconds < 1000)
    {
        return utils::format("{} ms", timeInMilliseconds, std::dec);
    }

    const long timeInSeconds = timeInMilliseconds / 1000;
    if (timeInSeconds < 60)
    {
        return utils::format("{} s", timeInSeconds, std::dec);
    }

    const int seconds = timeInSeconds % 60;
    if (seconds > 0)
    {
        return utils::format("{} min {} s", timeInSeconds / 60, std::dec, seconds, std::dec);
    }
    else
    {
        return utils::format("{} min", timeInSeconds / 60, std::dec);
    }
}

} // namespace core
