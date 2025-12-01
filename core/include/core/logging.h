#ifndef LOGGING_H
#define LOGGING_H

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <atomic>
#include <vector>
#include <string_view>

namespace core
{
    class VoidResult;
}

namespace logging
{

namespace boostLogging = boost::log;
namespace boostLoggingSource = boost::log::sources;

enum class severityLevel : std::uint8_t { debug, info, warning, critical, fatal, none };

inline std::ostream& operator<<(std::ostream& os, severityLevel level)
{
    switch (level)
    {
        case severityLevel::debug: os << "DEBUG"; break;
        case severityLevel::info: os << "INFO"; break;
        case severityLevel::warning: os << "WARNING"; break;
        case severityLevel::critical: os << "CRITICAL"; break;
        case severityLevel::fatal: os << "FATAL"; break;
        default: os << "UNKNOWN"; break;
    }
    return os;
}

extern boostLoggingSource::severity_channel_logger<severityLevel, std::string_view> CORE_PROPERTIES;
extern boostLoggingSource::severity_channel_logger<severityLevel, std::string_view> CORE_CONNECTION;

static constexpr std::string_view CORE_CONNECTION_CHANNEL_NAME = "CORE_CONNECTION";
static constexpr std::string_view CORE_PROPERTIES_CHANNEL_NAME = "CORE_PROPERTIES";

struct ChannelFilters final
{
    ChannelFilters();

    using Callback = void(*)();

    severityLevel operator[](std::string_view channelName);
    void set(std::string_view channelName, severityLevel level);

    void addCallback(Callback cb);
    void notifyCallbacks();

private:
    std::atomic<severityLevel> m_coreConnectionSeverityLevel;
    std::atomic<severityLevel> m_corePropertiesSeverityLevel;

    std::vector<Callback> m_callbacks;
};

extern ChannelFilters* channelFilters;

void initLogging(ChannelFilters* channelFilters = nullptr);
core::VoidResult setChannelFilter(const std::string_view& channelName, severityLevel severity);
core::VoidResult setChannelEnabled(const std::string_view& channelName, bool isEnabled);
std::vector<severityLevel> getSeverityLevels();
severityLevel getLoggingLevel(const std::string_view& channelName);
} // namespace logging

#define WW_LOG_CONNECTION_DEBUG    BOOST_LOG_SEV(logging::CORE_CONNECTION, logging::severityLevel::debug)
#define WW_LOG_CONNECTION_INFO     BOOST_LOG_SEV(logging::CORE_CONNECTION, logging::severityLevel::info)
#define WW_LOG_CONNECTION_WARNING  BOOST_LOG_SEV(logging::CORE_CONNECTION, logging::severityLevel::warning)
#define WW_LOG_CONNECTION_CRITICAL BOOST_LOG_SEV(logging::CORE_CONNECTION, logging::severityLevel::critical)
#define WW_LOG_CONNECTION_FATAL    BOOST_LOG_SEV(logging::CORE_CONNECTION, logging::severityLevel::fatal)

#define WW_LOG_PROPERTIES_DEBUG    BOOST_LOG_SEV(logging::CORE_PROPERTIES, logging::severityLevel::debug)
#define WW_LOG_PROPERTIES_INFO     BOOST_LOG_SEV(logging::CORE_PROPERTIES, logging::severityLevel::info)
#define WW_LOG_PROPERTIES_WARNING  BOOST_LOG_SEV(logging::CORE_PROPERTIES, logging::severityLevel::warning)
#define WW_LOG_PROPERTIES_CRITICAL BOOST_LOG_SEV(logging::CORE_PROPERTIES, logging::severityLevel::critical)
#define WW_LOG_PROPERTIES_FATAL    BOOST_LOG_SEV(logging::CORE_PROPERTIES, logging::severityLevel::fatal)

#endif // LOGGING_H
