#include "Transaction.h"

Transaction::Transaction(
    int id,
    int accountId,
    int categoryId,
    const QString &date,
    qint64 amount,
    bool isIncome,
    const QString &notes)
    : m_id(id),
      m_accountId(accountId),
      m_categoryId(categoryId),
      m_date(date),
      m_amount(amount),
      m_isIncome(isIncome),
      m_notes(notes)
{
}

int Transaction::getId() const
{
    return m_id;
}

int Transaction::getAccountId() const
{
    return m_accountId;
}

int Transaction::getCategoryId() const
{
    return m_categoryId;
}

QString Transaction::getDate() const
{
    return m_date;
}

qint64 Transaction::getAmount() const
{
    return m_amount;
}

bool Transaction::isIncome() const
{
    return m_isIncome;
}

QString Transaction::getNotes() const
{
    return m_notes;
}

void Transaction::setId(int id)
{
    m_id = id;
}

void Transaction::setAccountId(int accountId)
{
    m_accountId = accountId;
}

void Transaction::setCategoryId(int categoryId)
{
    m_categoryId = categoryId;
}

void Transaction::setDate(const QString &date)
{
    m_date = date;
}

void Transaction::setAmount(qint64 amount)
{
    m_amount = amount;
}

void Transaction::setIncome(bool isIncome)
{
    m_isIncome = isIncome;
}

void Transaction::setNotes(const QString &notes)
{
    m_notes = notes;
}