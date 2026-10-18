#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QList>
#include <QVariant>

#include "Statistics.h"
#include "Transfer.h"

class Transaction;

struct TransactionFilter
{
    QString dateFrom;
    QString dateTo;
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
        int *newId = nullptr);

    bool getAccountBalance(int accountId, qint64 &balance);
    bool getFirstAccountId(int &accountId); // 0 when no account exists
    bool getAccounts(QList<QVariantMap> &accounts); // ID_Account, Name, Initial_Credit, Credit
    bool renameAccount(int accountId, const QString &name);
    bool deleteAccount(int accountId); // only empty accounts, never the last one
    bool getAccountInitialCredit(int accountId, qint64 &initialCredit);
    bool setAccountInitialCredit(int accountId, qint64 initialCredit);

    // Categorie condivise
    bool addCategory(
        const QString &name,
        const QString &type,
        int *newId = nullptr);

    bool getCategories(QList<QVariantMap> &categories);
    bool updateCategory(int categoryId, const QString &name);
    bool deleteCategory(int categoryId);
    bool getCategoryUsage(int categoryId, int &transactionCount);

    // Transazioni
    bool addTransaction(const Transaction &transaction);
    bool updateTransaction(const Transaction &transaction);
    bool deleteTransaction(int transactionId);

    bool getTransactions(
        int accountId,
        QList<Transaction> &transactions,
        const TransactionFilter &filter = TransactionFilter{});
    // Trasferimenti tra conti
    bool addTransfer(
        int fromAccountId,
        int toAccountId,
        qint64 amount,
        const QString &date,
        const QString &note,
        int *newId = nullptr);
    bool deleteTransfer(int transferId);
    bool getTransfers(
        int accountId,
        QList<Transfer> &transfers,
        const TransactionFilter &filter = TransactionFilter{});
    bool getRecentTransactions(int accountId, int count, QList<Transaction> &transactions);

    // Aggregati calcolati in SQL
    bool getTotals(
        int accountId,
        qint64 &income,
        qint64 &expenses,
        const TransactionFilter &filter = TransactionFilter{});
    bool getExpensesByCategory(
        int accountId,
        const TransactionFilter &filter,
        QList<CategoryTotal> &totals);
    bool getMonthlyTotals(
        int accountId,
        const QString &fromDate,
        QList<MonthlyTotal> &totals);
};