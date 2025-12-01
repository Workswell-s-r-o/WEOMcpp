#include "core/misc/result.h"
#include "core/utils.h"

#include <ostream>

namespace core
{

bool Result::isOk() const
{
    return m_generalErrorMessage.empty();
}

const std::string& Result::getGeneralErrorMessage() const
{
    return m_generalErrorMessage;
}

const std::string& Result::getDetailErrorMessage() const
{
    return m_detailErrorMessage;
}

const core::ResultSpecificInfo* core::Result::getSpecificInfo() const
{
    return m_specificInfo;
}

std::string Result::toString() const
{
    if (isOk())
    {
        return "OK";
    }

#ifdef RESULT_STRING_WITH_DETAIL
    if (!m_detailErrorMessage.empty())
    {
        return utils::format("{} ({})", m_generalErrorMessage, m_detailErrorMessage);
    }
#endif

    return m_generalErrorMessage;
}

Result::Result(const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo) :
    m_generalErrorMessage(generalErrorMessage),
    m_detailErrorMessage(detailErrorMessage),
    m_specificInfo(specificInfo)
{
    if (isOk() && !m_detailErrorMessage.empty())
    {
        assert(false && "OK means OK - do not use error message!");
        m_detailErrorMessage = std::string();
    }
}

std::ostream& operator <<(std::ostream& stream, const Result& result)
{
    return stream << result.toString();
}


VoidResult::VoidResult() :
    VoidResult("Uninitialized", "Uninitialized VoidResult", nullptr)
{
}

VoidResult VoidResult::createOk()
{
    const VoidResult result(std::string(), std::string(), nullptr);
    assert(result.isOk());
    return result;
}

VoidResult VoidResult::createError(const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo)
{
    const VoidResult result(generalErrorMessage.c_str() == nullptr ? "" : generalErrorMessage, detailErrorMessage, specificInfo);
    assert(!result.isOk());
    return result;
}

VoidResult::VoidResult(const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo) :
    BaseClass(generalErrorMessage, detailErrorMessage, specificInfo)
{
}

} // namespace core
