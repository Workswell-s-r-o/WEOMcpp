#include "core/properties/transactionsummary.h"


namespace core
{

TransactionSummary::TransactionSummary(const std::shared_ptr<TransactionChanges>& transactionChanges,
                                       const std::shared_ptr<std::promise<bool>>& lifetimeObject) :
    m_transactionChanges(transactionChanges),
    m_lifetimeObject(lifetimeObject)
{
}

const TransactionChanges& TransactionSummary::getTransactionChanges() const
{
    return *m_transactionChanges.get();
}

LifetimeChecker TransactionSummary::getLifetimeChecker() const
{
    return LifetimeChecker(*m_lifetimeObject.get(), (size_t)m_lifetimeObject.get());
}

} // namespace core
