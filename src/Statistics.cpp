#include "Statistics.h"

#include <limits>
#include <stdexcept>

namespace
{
qint64 checkedAdd(qint64 total, qint64 amount)
{
    if ((amount > 0 && total > std::numeric_limits<qint64>::max() - amount) ||
        (amount < 0 && total < std::numeric_limits<qint64>::min() - amount)) {
        throw std::overflow_error("Statistics total exceeds the supported range");
    }
    return total + amount;
}

qint64 checkedSubtract(qint64 left, qint64 right)
{
    if ((right > 0 && left < std::numeric_limits<qint64>::min() + right) ||
        (right < 0 && left > std::numeric_limits<qint64>::max() + right)) {
        throw std::overflow_error("Statistics balance exceeds the supported range");
    }
    return left - right;
}
}

Statistics::Statistics(const QList<Transaction> &transactions)
{
    for (const Transaction &transaction : transactions) {
        if (transaction.isIncome()) {
            m_totalIncome = checkedAdd(m_totalIncome, transaction.getAmount());
        } else {
            m_totalExpenses = checkedAdd(m_totalExpenses, transaction.getAmount());
        }
    }
}

qint64 Statistics::getTotalIncome() const
{
    return m_totalIncome;
}

qint64 Statistics::getTotalExpenses() const
{
    return m_totalExpenses;
}

qint64 Statistics::getBalance() const
{
    return checkedSubtract(m_totalIncome, m_totalExpenses);
}