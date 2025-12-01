#pragma once

#define TRY_RESULT(EXPRESSION)                                                                    \
    if (auto result = EXPRESSION; !result.isOk()) { return ResultType::createFromError(result); } \
    void(0)

#define UNIQUE_VAR(var, suffix) UNIQUE_VAR_IMPL(var, suffix)
#define UNIQUE_VAR_IMPL(var, suffix) var##suffix

#define TRY_GET_RESULT(VAL, EXPRESSION)                                   \
    auto UNIQUE_VAR(result, __LINE__) = EXPRESSION;                       \
    if (!UNIQUE_VAR(result, __LINE__).isOk()) {                           \
        return ResultType::createFromError(UNIQUE_VAR(result, __LINE__)); \
    }                                                                     \
    VAL = std::move(UNIQUE_VAR(result, __LINE__)).releaseValue()
