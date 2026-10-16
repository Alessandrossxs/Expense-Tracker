#include "ExpenseManager.h"

#include <QSaveFile>
#include <QStringConverter>
#include <QTextStream>

#include <stdexcept>

namespace
{
QString csvField(const QString &value)
{
    // Quote every cell to prevent delimiter/quote breakout. Spreadsheet software may
    // still evaluate formula-like values, so neutralize dangerous prefixes as well.
    QString safeValue = value;
    int firstNonWhitespace = 0;
    while (firstNonWhitespace < safeValue.size() &&
           safeValue.at(firstNonWhitespace).isSpace()) {
        ++firstNonWhitespace;
    }

    if (firstNonWhitespace < safeValue.size()) {
        const QChar first = safeValue.at(firstNonWhitespace);
        const bool formulaLike =
            first == QLatin1Char('=') || first == QLatin1Char('+') ||
            first == QLatin1Char('-') || first == QLatin1Char('@') ||
            first == QChar(0xFF1D) || first == QChar(0xFF0B) ||
            first == QChar(0xFF0D) || first == QChar(0xFF20) ||
            first == QChar(0x0000);
        if (formulaLike) {
            // Insert immediately before the first non-whitespace character. This
            // also handles values with leading spaces without moving those spaces.
            safeValue.insert(firstNonWhitespace, QLatin1Char('\\t'));
        }
    }

    safeValue.replace(QLatin1Char('"'), QStringLiteral("\\"\\""));
    return QLatin1Char('"') + safeValue + QLatin1Char('"');
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
    int *newId,
    const QString &accountType)
{
    return recordResult(
        m_database.addAccount(
            name, passwordHash, creationDate, initialCredit, newId, accountType));
}

bool ExpenseManager::getAccounts(QList<QVariantMap> &accounts)
{
    return recordResult(m_database.getAccounts(accounts));
}

bool ExpenseManager::updateAccount(
    int accountId,
    const QString &name,
    const QString &accountType)
{
    return recordResult(m_database.updateAccount(accountId, name, accountType));
}

bool ExpenseManager::deleteAccount(int accountId)
{
    return recordResult(m_database.deleteAccount(accountId));
}

bool ExpenseManager::getAccountBalance(int accountId, qint64 &balance)
{
    return recordResult(m_database.getAccountBalance(accountId, balance));
}

bool ExpenseManager::addTransfer(
    int sourceAccountId,
    int destinationAccountId,
    const QString &date,
    qint64 amount)
{
    return recordResult(
        m_database.addTransfer(sourceAccountId, destinationAccountId, date, amount));
}

bool ExpenseManager::getAccountTransfers(int accountId, QList<QVariantMap> &transfers)
{
    return recordResult(m_database.getAccountTransfers(accountId, transfers));
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
    stream << "transaction_id,account_id,category_id,date,type,amount_cents,notes\n";
    for (const Transaction &transaction : transactions) {
        stream << csvField(QString::number(transaction.getId())) << ','
               << csvField(QString::number(transaction.getAccountId())) << ','
               << csvField(QString::number(transaction.getCategoryId())) << ','
               << csvField(transaction.getDate()) << ','
               << csvField(transaction.isIncome() ? QStringLiteral("income")
                                                   : QStringLiteral("expense"))
               << ',' << csvField(QString::number(transaction.getAmount())) << ','
               << csvField(transaction.getNotes()) << '\n';
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