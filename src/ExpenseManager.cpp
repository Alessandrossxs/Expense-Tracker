#include "ExpenseManager.h"

#include <QSaveFile>
#include <QStringConverter>
#include <QTextStream>

#include <stdexcept>

namespace
{
QString csvField(const QString &value)
{
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

bool ExpenseManager::getStatistics(
    int accountId,
    Statistics &statistics,
    const TransactionFilter &filter)
{
    QList<Transaction> transactions;
    if (!getTransactions(accountId, transactions, filter)) {
        return false;
    }

    try {
        statistics = Statistics(transactions);
    } catch (const std::overflow_error &error) {
        m_lastError = QString::fromUtf8(error.what());
        return false;
    }
    m_lastError.clear();
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
    stream << "transaction_id,account_id,category_id,date,type,amount_cents\r\n";
    for (const Transaction &transaction : transactions) {
        stream << csvField(QString::number(transaction.getId())) << ','
               << csvField(QString::number(transaction.getAccountId())) << ','
               << csvField(QString::number(transaction.getCategoryId())) << ','
               << csvField(transaction.getDate()) << ','
               << csvField(transaction.isIncome() ? QStringLiteral("income")
                                                   : QStringLiteral("expense"))
               << ',' << csvField(QString::number(transaction.getAmount())) << "\r\n";
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