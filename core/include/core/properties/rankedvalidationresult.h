#ifndef CORE_RANKEDVALIDATIONRESULT_H
#define CORE_RANKEDVALIDATIONRESULT_H

#include "core/misc/result.h"


namespace core
{

class RankedValidationResult
{
public:
    enum class ErrorRank
    {
        FATAL_ERROR,
        WARNING,
        DATA_FOR_VALIDATION_NOT_READY,
    };

private:
    RankedValidationResult();
    RankedValidationResult(const VoidResult& error, ErrorRank errorRank);

public:
    static RankedValidationResult createOk();
    static RankedValidationResult createDataForValidationNotReady(const std::string& detailErrorMessage);
    static RankedValidationResult createError(const VoidResult& error);
    static RankedValidationResult createError(const std::string& generalErrorMessage, const std::string& detailErrorMessage = std::string());
    static RankedValidationResult createWarning(const VoidResult& error);
    static RankedValidationResult createWarning(const std::string& generalErrorMessage, const std::string& detailErrorMessage = std::string());

    const VoidResult& getResult() const;
    const std::optional<ErrorRank>& getErrorRank() const;

    bool isAcceptable() const;

    bool operator==(const RankedValidationResult& other) const = default;

private:
    VoidResult m_result {VoidResult::createOk()};
    std::optional<ErrorRank> m_errorRank;
};

} // namespace core

#endif // CORE_RANKEDVALIDATIONRESULT_H
