#include "ExpenseManager.h"

#include <QSaveFile>
#include <QStringConverter>
#include <QTextStream>

#include <stdexcept>

namespace
{
QString csvField(const QString &value)
{
    // RFC 4180: quote only fields that contain a separator, quote or line break.
    if (!value.contains(QLatin1Char(',')) && !value.contains(QLatin1Char('"'))
        && !value.contains(QLatin1Char('\n')) && !value.contains(QLatin1Char('\r'))) {
        return value;
    }
    QString escaped = value;
    escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}
}

ExpenseManager::ExpenseManager(const QString &databasePath)
    : m_database(databasePath)
{
}

bool ExpenseManager::recordResult(bool success)
{
    m_lastError = success ? QString() : m_database.lastError();
    return success;
}

bool ExpenseManager::open()
{
    return recordResult(m_database.open());
}

bool ExpenseManager::isOpen() const
{
    return m_database.isOpen();
}

QString ExpenseManager::lastError() const
{
    return m_lastError;
}

bool ExpenseManager::addAccount(
    const QString &name,
    const QString &passwordHash,
    const QString &creationDate,
    qint64 initialCredit,
    int *newId)
{
    return recordResult(
        m_database.addAccount(name, passwordHash, creationDate, initialCredit, newId));
}

bool ExpenseManager::getAccountBalance(int accountId, qint64 &balance)
{
    return recordResult(m_database.getAccountBalance(accountId, balance));
}

bool ExpenseManager::addCategory(const QString &name, const QString &type, int *newId)
{
    return recordResult(m_database.addCategory(name, type, newId));
}

bool ExpenseManager::getCategories(QList<QVariantMap> &categories)
{
    return recordResult(m_database.getCategories(categories));
}

bool ExpenseManager::addTransaction(const Transaction &transaction)
{
    return recordResult(m_database.addTransaction(transaction));
}

bool ExpenseManager::updateTransaction(const Transaction &transaction)
{
    return recordResult(m_database.updateTransaction(transaction));
}

bool ExpenseManager::deleteTransaction(int transactionId)
{
    return recordResult(m_database.deleteTransaction(transactionId));
}

bool ExpenseManager::getTransactions(
    int accountId,
    QList<Transaction> &transactions,
    const TransactionFilter &filter)
{
    return recordResult(m_database.getTransactions(accountId, transactions, filter));
}

bool ExpenseManager::getFirstAccountId(int &accountId)
{
    return recordResult(m_database.getFirstAccountId(accountId));
}

bool ExpenseManager::getAccountInitialCredit(int accountId, qint64 &initialCredit)
{
    return recordResult(m_database.getAccountInitialCredit(accountId, initialCredit));
}

bool ExpenseManager::setAccountInitialCredit(int accountId, qint64 initialCredit)
{
    return recordResult(m_database.setAccountInitialCredit(accountId, initialCredit));
}

bool ExpenseManager::updateCategory(int categoryId, const QString &name)
{
    return recordResult(m_database.updateCategory(categoryId, name));
}

bool ExpenseManager::deleteCategory(int categoryId)
{
    return recordResult(m_database.deleteCategory(categoryId));
}

bool ExpenseManager::getCategoryUsage(int categoryId, int &transactionCount)
{
    return recordResult(m_database.getCategoryUsage(categoryId, transactionCount));
}

bool ExpenseManager::getRecentTransactions(int accountId, int count, QList<Transaction> &transactions)
{
    return recordResult(m_database.getRecentTransactions(accountId, count, transactions));
}

bool ExpenseManager::getExpensesByCategory(int accountId, const TransactionFilter &filter, QList<CategoryTotal> &totals)
{
    return recordResult(m_database.getExpensesByCategory(accountId, filter, totals));
}

bool ExpenseManager::getMonthlyTotals(int accountId, const QString &fromDate, QList<MonthlyTotal> &totals)
{
    return recordResult(m_database.getMonthlyTotals(accountId, fromDate, totals));
}

bool ExpenseManager::getAccounts(QList<QVariantMap> &accounts)
{
    return recordResult(m_database.getAccounts(accounts));
}

bool ExpenseManager::renameAccount(int accountId, const QString &name)
{
    return recordResult(m_database.renameAccount(accountId, name));
}

bool ExpenseManager::deleteAccount(int accountId)
{
    return recordResult(m_database.deleteAccount(accountId));
}

bool ExpenseManager::addTransfer(int fromAccountId, int toAccountId, qint64 amount, const QString &date,
                                 const QString &note, int *newId)
{
    return recordResult(m_database.addTransfer(fromAccountId, toAccountId, amount, date, note, newId));
}

bool ExpenseManager::deleteTransfer(int transferId)
{
    return recordResult(m_database.deleteTransfer(transferId));
}

bool ExpenseManager::getTransfers(int accountId, QList<Transfer> &transfers, const TransactionFilter &filter)
{
    return recordResult(m_database.getTransfers(accountId, transfers, filter));
}

bool ExpenseManager::addRecurring(const RecurringRule &rule, int *newId)
{
    return recordResult(m_database.addRecurring(rule, newId));
}

bool ExpenseManager::updateRecurring(const RecurringRule &rule)
{
    return recordResult(m_database.updateRecurring(rule));
}

bool ExpenseManager::deleteRecurring(int recurringId)
{
    return recordResult(m_database.deleteRecurring(recurringId));
}

bool ExpenseManager::setRecurringActive(int recurringId, bool active, const QString &today)
{
    return recordResult(m_database.setRecurringActive(recurringId, active, today));
}

bool ExpenseManager::getRecurring(int accountId, QList<RecurringRule> &rules)
{
    return recordResult(m_database.getRecurring(accountId, rules));
}

bool ExpenseManager::processRecurring(const QString &today, int &created)
{
    return recordResult(m_database.processRecurring(today, created));
}

bool ExpenseManager::getStatistics(
    int accountId,
    Statistics &statistics,
    const TransactionFilter &filter)
{
    qint64 income = 0;
    qint64 expenses = 0;
    if (!recordResult(m_database.getTotals(accountId, income, expenses, filter))) {
        return false;
    }
    statistics = Statistics::fromTotals(income, expenses);
    return true;
}

bool ExpenseManager::exportTransactionsToCsv(
    int accountId,
    const QString &filePath,
    const TransactionFilter &filter)
{
    if (filePath.trimmed().isEmpty()) {
        m_lastError = QStringLiteral("CSV file path is required");
        return false;
    }

    QList<Transaction> transactions;
    if (!getTransactions(accountId, transactions, filter)) {
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Failed to open CSV file: ") + file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << "transaction_id,account_id,category_id,date,type,amount_cents,note\r\n";
    for (const Transaction &transaction : transactions) {
        stream << csvField(QString::number(transaction.getId())) << ','
               << csvField(QString::number(transaction.getAccountId())) << ','
               << csvField(QString::number(transaction.getCategoryId())) << ','
               << csvField(transaction.getDate()) << ','
               << csvField(transaction.isIncome() ? QStringLiteral("income")
                                                   : QStringLiteral("expense"))
               << ',' << csvField(QString::number(transaction.getAmount())) << ','
               << csvField(transaction.getNote()) << "\r\n";
    }
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        file.cancelWriting();
        m_lastError = QStringLiteral("Failed to write CSV data");
        return false;
    }
    if (!file.commit()) {
        m_lastError = QStringLiteral("Failed to save CSV file: ") + file.errorString();
        return false;
    }

    m_lastError.clear();
    return true;
}