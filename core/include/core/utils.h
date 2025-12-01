#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

namespace utils
{

// Replaces the next "{}" in formatString with value, and writes to output stream
template<typename T>
void replaceNext(std::ostringstream& oss, const std::string& formatString, size_t& lastPos, const T& value)
{
    size_t pos = formatString.find("{}", lastPos);
    if (pos == std::string::npos)
        return;

    oss.write(formatString.data() + lastPos, pos - lastPos);

    oss << value;
    //surely there is a better way of doing the line below?
    oss << std::dec;
    lastPos = pos + 2;
}

// Overload for handling stream manipulators like std::hex, std::dec, etc.
inline void replaceNext(std::ostringstream& oss, const std::string&, size_t&, std::ios_base& (&fmt)(std::ios_base&))
{
    oss << fmt;  // Apply the formatting manipulator
}

// Recursive helper function to process all arguments
template<typename T, typename... Args>
void formatHelper(std::ostringstream& oss, const std::string& format, size_t& lastPos, T&& value, Args&&... args)
{
    replaceNext(oss, format, lastPos, std::forward<T>(value));

    if constexpr (sizeof...(args) > 0)
    {
        formatHelper(oss, format, lastPos, std::forward<Args>(args)...);
    }
}

// Variadic function to utils::format string with multiple arguments
template<typename... Args>
std::string format(const std::string& format, Args&&... args)
{
    std::ostringstream oss;
    size_t lastPos = 0;

    if constexpr (sizeof...(args) > 0)
    {
        formatHelper(oss, format, lastPos, std::forward<Args>(args)...);
    }

    oss.write(format.data() + lastPos, format.size() - lastPos);

    return oss.str();
}

inline std::string joinStringVector(const std::vector<std::string>& list, const std::string& separator)
{
    if (list.empty())
    {
        return "";
    }

    std::string result = list[0];
    for (size_t i = 1; i < list.size(); ++i)
    {
        result += separator + list[i];
    }
    return result;
}

inline std::string justifyRight(const std::string& content, const char& fillchar, const int& fillAmount)
{
    if(content.size() > fillAmount)
    {
        return content;
    }

    std::string returnValue;
    returnValue.reserve(content.size() + fillAmount);
    returnValue = content;
    returnValue.insert(0, fillAmount-content.size(), fillchar);
    return returnValue;
}

inline std::string justifyLeft(const std::string& content, const char& fillchar, const int& fillAmount)
{
    if(content.size() > fillAmount)
    {
        return content;
    }

    std::string returnValue;
    returnValue.reserve(content.size() + fillAmount);
    returnValue = content;
    returnValue.insert(content.size(), fillAmount-content.size(), fillchar);
    return returnValue;
}

template <typename T>
requires std::is_unsigned_v<T>
std::string numberToHex(const T& number, bool includePrefix)
{
    static constexpr char HEX_CHARS[] = "0123456789ABCDEF";

    if (number == 0)
        return includePrefix ? "0x00" : "00";

    std::string result;
    T temp = number;

    while (temp > 0)
    {
        char low = HEX_CHARS[temp & 0xF];
        temp >>= 4;
        result.insert(result.begin(), low);
    }

    // Ensure even number of characters (e.g. 0x0A not 0xA)
    if (result.size() % 2 != 0)
        result.insert(result.begin(), '0');

    if (includePrefix)
        result.insert(0, "0x");

    return result;
}

inline std::string stringToUpperTrimmed(const std::string& str)
{
    std::string tmp;
    tmp.reserve(str.size());

    std::copy_if(str.begin(), str.end(), std::back_inserter(tmp),[](unsigned char c)
    {
        return std::isprint(c);
    });

    std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char c)
    {
        return std::toupper(c);
    });

    return tmp;
}

template <typename T, typename MemFn>
auto autoBind(T* instance, MemFn fn)
{
    return [=](auto&&... args)
    {
        (instance->*fn)(std::forward<decltype(args)>(args)...);
    };
}

} // namespace utils
#endif // CORE_UTILS_H
