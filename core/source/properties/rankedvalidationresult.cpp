#include "core/properties/rankedvalidationresult.h"
#include <cassert>


namespace core
{

RankedValidationResult::RankedValidationResult()
{
}

RankedValidationResult::RankedValidationResult(const VoidResult& error, ErrorRank errorRank) :
    m_result(error),
    m_errorRank(errorRank)
{
    assert(!error.isOk());
}

RankedValidationResult RankedValidationResult::createOk()
{
    return RankedValidationResult();
}

RankedValidationResult RankedValidationResult::createDataForValidationNotReady(const std::string& detailErrorMessage)
{
    return RankedValidationResult(VoidResult::createError("Data not ready!", detailErrorMessage), ErrorRank::DATA_FOR_VALIDATION_NOT_READY);
}

RankedValidationResult RankedValidationResult::createError(const VoidResult& error)
{
    return RankedValidationResult(error, ErrorRank::FATAL_ERROR);
}

RankedValidationResult RankedValidationResult::createError(const std::string& generalErrorMessage, const std::string& detailErrorMessage)
{
    return RankedValidationResult(VoidResult::createError(generalErrorMessage, detailErrorMessage), ErrorRank::FATAL_ERROR);
}

RankedValidationResult RankedValidationResult::createWarning(const VoidResult& error)
{
    return RankedValidationResult(error, ErrorRank::WARNING);
}

RankedValidationResult RankedValidationResult::createWarning(const std::string& generalErrorMessage, const std::string& detailErrorMessage)
{
    return RankedValidationResult(VoidResult::createError(generalErrorMessage, detailErrorMessage), ErrorRank::WARNING);
}

const VoidResult& RankedValidationResult::getResult() const
{
    return m_result;
}

const std::optional<RankedValidationResult::ErrorRank>& RankedValidationResult::getErrorRank() const
{
    return m_errorRank;
}

bool RankedValidationResult::isAcceptable() const
{
    assert(m_result.isOk() || m_errorRank == ErrorRank::FATAL_ERROR || m_errorRank == ErrorRank::WARNING || m_errorRank == ErrorRank::DATA_FOR_VALIDATION_NOT_READY);
    return m_result.isOk() || m_errorRank == ErrorRank::WARNING;
}

} // namespace core
