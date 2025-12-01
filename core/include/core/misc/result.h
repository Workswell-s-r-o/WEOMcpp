#ifndef CORE_RESULT_H
#define CORE_RESULT_H

#include "resultmacros.h"
#include <optional>
#include <string>
#include <cassert>

namespace core
{

/**
 * @brief A struct for holding specific information about a result.
 */
struct ResultSpecificInfo
{
    /**
     * @brief Default constructor.
     */
    constexpr explicit ResultSpecificInfo() {}
    /**
     * @brief Default destructor.
     */
    constexpr virtual ~ResultSpecificInfo() {}
};


/**
 * @brief A class for holding the result of an operation.
 */
class Result
{
public:
    /**
     * @brief Checks if the result is OK.
     * @return True if the result is OK, false otherwise.
     */
    bool isOk() const;
    /**
     * @brief Gets the general error message.
     * @return The general error message.
     */
    const std::string& getGeneralErrorMessage() const;
    /**
     * @brief Gets the detailed error message.
     * @return The detailed error message.
     */
    const std::string& getDetailErrorMessage() const;

    /**
     * @brief Gets the specific info.
     * @return The specific info.
     */
    const ResultSpecificInfo* getSpecificInfo() const;

    /**
     * @brief Converts the result to a string.
     * @return The string representation of the result.
     */
    std::string toString() const;

    /**
     * @brief Equality operator.
     * @param other The other result to compare with.
     * @return True if the results are equal, false otherwise.
     */
    bool operator==(const Result& other) const = default;

protected:
    /**
     * @brief Constructor.
     * @param generalErrorMessage The general error message.
     * @param detailErrorMessage The detailed error message.
     * @param specificInfo The specific info.
     */
    explicit Result(const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo);
    /**
     * @brief Destructor.
     */
    ~Result() = default;

private:
    std::string m_generalErrorMessage;
    std::string m_detailErrorMessage;

    const ResultSpecificInfo* m_specificInfo {nullptr};
};

/**
 * @brief Stream insertion operator for Result.
 * @param stream The output stream.
 * @param result The result to insert.
 * @return The output stream.
 */
std::ostream& operator <<(std::ostream& stream, const Result& result);


/**
 * @brief A class for holding a void result.
 */
class VoidResult final : public Result
{
    using BaseClass = Result;

public:
    /**
     * @brief Default constructor.
     */
    VoidResult();

    /**
     * @brief Creates an OK result.
     * @return An OK result.
     */
    [[nodiscard]] static VoidResult createOk();
    /**
     * @brief Creates an error result.
     * @param generalErrorMessage The general error message.
     * @param detailErrorMessage The detailed error message.
     * @param specificInfo The specific info.
     * @return An error result.
     */
    [[nodiscard]] static VoidResult createError(const std::string& generalErrorMessage, const std::string& detailErrorMessage = std::string(), const ResultSpecificInfo* specificInfo = nullptr);

    /**
     * @brief Equality operator.
     * @param other The other result to compare with.
     * @return True if the results are equal, false otherwise.
     */
    bool operator==(const VoidResult& other) const = default;

private:
    /**
     * @brief Constructor.
     * @param generalErrorMessage The general error message.
     * @param detailErrorMessage The detailed error message.
     * @param specificInfo The specific info.
     */
    explicit VoidResult(const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo);
};


/**
 * @brief A class for holding a result with a value.
 * @tparam T The type of the value.
 */
template<class T>
class ValueResult final : public Result
{
    using BaseClass = Result;

public:
    using ValueType = T;

    /**
     * @brief Constructor.
     * @param value The value.
     */
    ValueResult(const T& value);
    /**
     * @brief Default constructor.
     */
    ValueResult();

    /**
     * @brief Gets the value.
     * @return The value.
     */
    [[nodiscard]] const T& getValue() const;
    /**
     * @brief Releases the value.
     * @return The value.
     */
    [[nodiscard]] T&& releaseValue() &&;

    /**
     * @brief Converts the result to a void result.
     * @return The void result.
     */
    [[nodiscard]] VoidResult toVoidResult() const;

    /**
     * @brief Creates an error result.
     * @param generalErrorMessage The general error message.
     * @param detailErrorMessage The detailed error message.
     * @param specificInfo The specific info.
     * @return An error result.
     */
    [[nodiscard]] static ValueResult<T> createError(const std::string& generalErrorMessage, const std::string& detailErrorMessage = std::string(), const ResultSpecificInfo* specificInfo = nullptr);

    /**
     * @brief Creates a result from an error result.
     * @tparam ErrorResultType The type of the error result.
     * @param errorResult The error result.
     * @return A result with the error from the given result.
     */
    template<class ErrorResultType>
    [[nodiscard]] static ValueResult<T> createFromError(const ErrorResultType& errorResult);

    /**
     * @brief Equality operator.
     * @param other The other result to compare with.
     * @return True if the results are equal, false otherwise.
     */
    bool operator==(const ValueResult<T>& other) const = default;

private:
    /**
     * @brief Constructor.
     * @param value The value.
     * @param generalErrorMessage The general error message.
     * @param detailErrorMessage The detailed error message.
     * @param specificInfo The specific info.
     */
    explicit ValueResult(const std::optional<T>& value, const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo);

    std::optional<T> m_value;
};


/**
 * @brief A class for holding an optional result.
 * @tparam T The type of the value.
 */
template<class T>
class OptionalResult final
{
public:
    /**
     * @brief Default constructor.
     */
    OptionalResult();
    /**
     * @brief Constructor.
     * @param nullopt A nullopt to indicate that the result is empty.
     */
    OptionalResult(std::nullopt_t);
    /**
     * @brief Constructor.
     * @param value The value.
     */
    OptionalResult(const T& value);
    /**
     * @brief Constructor.
     * @param result The result.
     */
    OptionalResult(const ValueResult<T>& result);

    /**
     * @brief Creates an error result.
     * @param generalErrorMessage The general error message.
     * @param detailErrorMessage The detailed error message.
     * @param specificInfo The specific info.
     * @return An error result.
     */
    [[nodiscard]] static OptionalResult<T> createError(const std::string& generalErrorMessage, const std::string& detailErrorMessage = std::string(), const ResultSpecificInfo* specificInfo = nullptr);

    /**
     * @brief Creates a result from an error result.
     * @tparam ErrorResultType The type of the error result.
     * @param errorResult The error result.
     * @return A result with the error from the given result.
     */
    template<class ErrorResultType>
    [[nodiscard]] static OptionalResult<T> createFromError(const ErrorResultType& errorResult);

    /**
     * @brief Checks if the result has a value.
     * @return True if the result has a value, false otherwise.
     */
    bool hasResult() const;
    /**
     * @brief Checks if the result contains an error.
     * @return True if the result contains an error, false otherwise.
     */
    bool containsError() const;
    /**
     * @brief Checks if the result contains a value.
     * @return True if the result contains a value, false otherwise.
     */
    bool containsValue() const;

    /**
     * @brief Gets the result.
     * @return The result.
     */
    const ValueResult<T>& getResult() const;
    /**
     * @brief Gets the value.
     * @return The value.
     */
    const T& getValue() const;

    /**
     * @brief Equality operator.
     * @param other The other result to compare with.
     * @return True if the results are equal, false otherwise.
     */
    bool operator==(const OptionalResult<T>& other) const = default;

private:
    std::optional<ValueResult<T>> m_result;
};

// Impl

template<class T>
ValueResult<T>::ValueResult(const T& value) :
    ValueResult(value, std::string(), std::string(), nullptr)
{
    assert(isOk());
}

template<class T>
ValueResult<T>::ValueResult() :
    ValueResult(std::nullopt, "Uninitialized", "Uninitialized ValueResult", nullptr)
{
    assert(!isOk());
}

template<class T>
ValueResult<T>::ValueResult(const std::optional<T>& value, const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo) :
    BaseClass(generalErrorMessage, detailErrorMessage, specificInfo),
    m_value(value)
{
    assert(isOk() == m_value.has_value());
}

template<class T>
const T& ValueResult<T>::getValue() const
{
    assert(m_value.has_value());
    return m_value.value();
}

template<class T>
T&& ValueResult<T>::releaseValue() &&
{
    assert(m_value.has_value());
    return std::move(m_value.value());
}

template<class T>
VoidResult ValueResult<T>::toVoidResult() const
{
    return isOk() ? VoidResult::createOk() :
                    VoidResult::createError(getGeneralErrorMessage(), getDetailErrorMessage(), getSpecificInfo());
}

template<class T>
ValueResult<T> ValueResult<T>::createError(const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo)
{
    const ValueResult result(std::nullopt, generalErrorMessage.c_str() == nullptr ? "" : generalErrorMessage, detailErrorMessage, specificInfo);
    assert(!result.isOk());
    return result;
}

template<class T>
template<class ErrorResultType>
ValueResult<T> ValueResult<T>::createFromError(const ErrorResultType& errorResult)
{
    assert(!errorResult.isOk());
    return createError(errorResult.getGeneralErrorMessage(), errorResult.getDetailErrorMessage(), errorResult.getSpecificInfo());
}

template<class T>
OptionalResult<T>::OptionalResult()
{
    assert(!hasResult());
}

template<class T>
OptionalResult<T>::OptionalResult(std::nullopt_t)
{
    assert(!hasResult());
}

template<class T>
OptionalResult<T>::OptionalResult(const T& value) :
    m_result(value)
{
    assert(containsValue());
}

template<class T>
OptionalResult<T>::OptionalResult(const ValueResult<T>& result) :
    m_result(ValueResult<T>(result))
{
}

template<class T>
OptionalResult<T> OptionalResult<T>::createError(const std::string& generalErrorMessage, const std::string& detailErrorMessage, const ResultSpecificInfo* specificInfo)
{
    OptionalResult<T> result;
    result.m_result = ValueResult<T>::createError(generalErrorMessage, detailErrorMessage, specificInfo);
    return result;
}

template<class T>
template<class ErrorResultType>
OptionalResult<T> OptionalResult<T>::createFromError(const ErrorResultType& errorResult)
{
    assert(!errorResult.isOk());
    return createError(errorResult.getGeneralErrorMessage(), errorResult.getDetailErrorMessage(), errorResult.getSpecificInfo());
}

template<class T>
bool OptionalResult<T>::hasResult() const
{
    return m_result.has_value();
}

template<class T>
bool OptionalResult<T>::containsError() const
{
    return m_result.has_value() && !m_result->isOk();
}

template<class T>
bool OptionalResult<T>::containsValue() const
{
    return m_result.has_value() && m_result->isOk();
}

template<class T>
const ValueResult<T>& OptionalResult<T>::getResult() const
{
    assert(m_result.has_value());

    return m_result.value();
}

template<class T>
const T& OptionalResult<T>::getValue() const
{
    assert(m_result.has_value() && m_result->isOk());

    return m_result->getValue();
}
} // namespace core

#endif // CORE_RESULT_H
