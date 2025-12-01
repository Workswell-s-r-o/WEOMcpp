#include "core/logging.h"
#include "core/misc/result.h"

#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions.hpp>

namespace logging
{

namespace boostLoggingExpressions = boost::log::expressions;

boostLoggingSource::severity_channel_logger<severityLevel, std::string_view> CORE_PROPERTIES(boostLogging::keywords::channel = CORE_PROPERTIES_CHANNEL_NAME);
boostLoggingSource::severity_channel_logger<severityLevel, std::string_view> CORE_CONNECTION(boostLogging::keywords::channel = CORE_CONNECTION_CHANNEL_NAME);

ChannelFilters* channelFilters = nullptr;

ChannelFilters::ChannelFilters()
#ifdef RESULT_STRING_WITH_DETAIL
    : m_coreConnectionSeverityLevel(severityLevel::info)
    , m_corePropertiesSeverityLevel(severityLevel::info)
#else
    : m_coreConnectionSeverityLevel(severityLevel::none)
    , m_corePropertiesSeverityLevel(severityLevel::none)
#endif
{}

severityLevel ChannelFilters::operator[](std::string_view channelName)
{
    if (channelName == CORE_CONNECTION_CHANNEL_NAME)
    {
        return m_coreConnectionSeverityLevel;
    }
    if (channelName == CORE_PROPERTIES_CHANNEL_NAME)
    {
        return m_corePropertiesSeverityLevel;
    }
    return severityLevel::none;
}

void ChannelFilters::set(std::string_view channelName, severityLevel level)
{
    if (channelName == CORE_CONNECTION_CHANNEL_NAME)
    {
        m_coreConnectionSeverityLevel = level;
    }
    if (channelName == CORE_PROPERTIES_CHANNEL_NAME)
    {
        m_corePropertiesSeverityLevel = level;
    }
    notifyCallbacks();
}

void ChannelFilters::addCallback(Callback cb)
{
    m_callbacks.push_back(cb);
}

void ChannelFilters::notifyCallbacks()
{
    for (auto cb : m_callbacks)
    {
        cb();
    }
}

void applyFilters()
{
    auto core = boostLogging::core::get();

    // I have to do this, since if I create an empty boostLogging::filter, it gets created with a filter that always returns true,
    // so no matter what filters I add it will allow all of the logs
    core->set_filter((boostLoggingExpressions::attr<std::string_view>("Channel") == CORE_CONNECTION_CHANNEL_NAME &&
                      boostLoggingExpressions::attr<severityLevel>("Severity") >= (*channelFilters)[CORE_CONNECTION_CHANNEL_NAME])
                     ||
                     (boostLoggingExpressions::attr<std::string_view>("Channel") == CORE_PROPERTIES_CHANNEL_NAME &&
                         boostLoggingExpressions::attr<severityLevel>("Severity") >= (*channelFilters)[CORE_PROPERTIES_CHANNEL_NAME])
    );
}

void initLogging(ChannelFilters* parentChannelFilters)
{
    if (parentChannelFilters)
    {
        channelFilters = parentChannelFilters;
    }
    else
    {
        static ChannelFilters staticFilters;
        channelFilters = &staticFilters;
    }

    boostLogging::add_common_attributes();

    boost::log::add_console_log(std::clog,
                                boost::log::keywords::format =
                                boost::log::expressions::stream
                                << "[" << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d:%m:%Y-%H:%M:%S:%f") << "]"
                                << "[" << boost::log::expressions::attr<std::string_view>("Channel") << "]"
                                << "[" << boost::log::expressions::attr<severityLevel>("Severity") << "] "
                                << boost::log::expressions::smessage
    );

    channelFilters->addCallback(&applyFilters);
    applyFilters();
}

core::VoidResult setChannelFilter(const std::string_view& channelName, severityLevel severity)
{
    channelFilters->set(channelName, severity);
    return {};
}

core::VoidResult setChannelEnabled(const std::string_view& channelName, bool isEnabled)
{
    channelFilters->set(channelName, isEnabled ? severityLevel::debug : severityLevel::fatal);
    return {};
}

severityLevel getLoggingLevel(const std::string_view& channelName)
{
    return (*channelFilters)[channelName];
}

std::vector<severityLevel> getSeverityLevels()
{
    return
    {
        logging::severityLevel::debug,
        logging::severityLevel::info,
        logging::severityLevel::warning,
        logging::severityLevel::critical,
        logging::severityLevel::fatal
    };
}

}
