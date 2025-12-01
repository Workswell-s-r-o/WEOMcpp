#ifndef CORE_TRANSACTIONSUMMARY_H
#define CORE_TRANSACTIONSUMMARY_H

#include "core/misc/lifetimechecker.h"

#include <future>


namespace core
{
class TransactionChanges;

class TransactionSummary
{
public:
    explicit TransactionSummary(const std::shared_ptr<TransactionChanges>& transactionChanges,
                                const std::shared_ptr<std::promise<bool>>& lifetimeObject);

    const TransactionChanges& getTransactionChanges() const;

    LifetimeChecker getLifetimeChecker() const;

private:
    std::shared_ptr<TransactionChanges> m_transactionChanges;

    std::shared_ptr<std::promise<bool>> m_lifetimeObject;
};

} // namespace core


#endif // CORE_TRANSACTIONSUMMARY_H
