#ifndef CORE_PROPERTYVALUECONVERSION_H
#define CORE_PROPERTYVALUECONVERSION_H

#include "core/device.h"
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>


namespace core
{

template<class ValueType>
struct PropertyValueConversion
{
    static std::string toString(const ValueType& value)
    {
        assert(false && "Conversion toString not defined!");
        return std::string();
    }
};

template<>
struct PropertyValueConversion<std::string>
{
    static std::string toString(const std::string& value)
    {
        return value;
    }
};

template<>
struct PropertyValueConversion<Version>
{
    static std::string toString(const Version& value)
    {
        return value.toString();
    }
};

template<>
struct PropertyValueConversion<double>
{
    static std::string toString(double value)
    {
        return boost::lexical_cast<std::string>(value);;
    }
};

template<>
struct PropertyValueConversion<unsigned>
{
    static std::string toString(unsigned value)
    {
        return boost::lexical_cast<std::string>(value);
    }
};

template<>
struct PropertyValueConversion<signed>
{
    static std::string toString(signed value)
    {
        return boost::lexical_cast<std::string>(value);
    }
};

template<>
struct PropertyValueConversion<bool>
{
    static std::string toString(bool value)
    {
        return value ? "True" : "False";
    }
};

template<>
struct PropertyValueConversion<boost::posix_time::ptime>
{
    static std::string toString(const boost::posix_time::ptime& value)
    {
        return boost::posix_time::to_iso_extended_string(value);
    }
};

} // namespace core

#endif // CORE_PROPERTYVALUECONVERSION_H
