#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QList>
#include <QVariant>

class Transaction;

struct TransactionFilter
{
    QString dateFrom;
    QString dateTo;
    QString type;
    int categoryId = 0;
    qint64 minimumAmount = 0;
    qint64 maximumAmount = 0;
};

class Database
{
private:
    QSqlDatabase m_db;
    QString m_lastError;

    bool execute(const QString &sql);

public:
    explicit Database(
        const QString &databasePath = "expense_tracker.db");

    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool open();
    bool initialize();
    bool isOpen() const;

    QString lastError() const;

    // Conti
    bool addAccount(
        const QString &name,
        const QString &passwordHash,
        const QString &creationDate,
        qint64 initialCredit,
        int *newId = nullptr,
        const QString &accountType = QStringLiteral("checking"));
    bool getAccounts(QList<QVariantMap> &accounts);
    bool updateAccount(int accountId, const QString &name, const QString &accountType);
    bool deleteAccount(int accountId);

    bool getAccountBalance(int accountId, qint64 &balance);
    bool addTransfer(
        int sourceAccountId,
        int destinationAccountId,
        const QString &date,
        qint64 amount);
    bool getAccountTransfers(int accountId, QList<QVariantMap> &transfers);

    // Categorie condivise
    bool addCategory(
        const QString &name,
        const QString &type,
        int *newId = nullptr);

    bool getCategories(QList<QVariantMap> &categories);

    // Transazioni
    bool addTransaction(const Transaction &transaction);
    bool updateTransaction(const Transaction &transaction);
    bool deleteTransaction(int transactionId);

    bool getTransactions(
        int accountId,
        QList<Transaction> &transactions,
        const TransactionFilter &filter = TransactionFilter{});
};