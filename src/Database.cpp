#include "Database.h"

#include "Transaction.h"

#include <QDate>
#include <QHash>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <limits>

namespace
{
bool isValidIsoDate(const QString &date)
{
    const QDate parsedDate = QDate::fromString(date, Qt::ISODate);
    return parsedDate.isValid() && parsedDate.toString(Qt::ISODate) == date;
}

void setQueryError(QString &lastError, const QString &context, const QSqlQuery &query)
{
    lastError = context + ": " + query.lastError().text();
}

bool rollbackWithError(QSqlDatabase &database, QString &lastError)
{
    const QString originalError = lastError;
    if (!database.rollback()) {
        lastError = originalError + "; rollback failed: " + database.lastError().text();
    }
    return false;
}

bool adjustAccountBalance(
    QSqlDatabase &database,
    int accountId,
    qint64 adjustment,
    QString &lastError)
{
    QSqlQuery selectQuery(database);
    selectQuery.prepare(
        QStringLiteral("SELECT Credit FROM Bank_Account WHERE ID_Account = ?"));
    selectQuery.addBindValue(accountId);
    if (!selectQuery.exec()) {
        setQueryError(lastError, QStringLiteral("Failed to read account balance"), selectQuery);
        return false;
    }
    if (!selectQuery.next()) {
        lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
        return false;
    }

    const qint64 currentBalance = selectQuery.value(0).toLongLong();
    selectQuery.finish();
    if ((adjustment > 0 &&
         currentBalance > std::numeric_limits<qint64>::max() - adjustment) ||
        (adjustment < 0 &&
         currentBalance < std::numeric_limits<qint64>::min() - adjustment)) {
        lastError = QStringLiteral("Account balance would exceed the supported range");
        return false;
    }

    QSqlQuery updateQuery(database);
    updateQuery.prepare(
        QStringLiteral("UPDATE Bank_Account SET Credit = ? WHERE ID_Account = ?"));
    updateQuery.addBindValue(currentBalance + adjustment);
    updateQuery.addBindValue(accountId);
    if (!updateQuery.exec()) {
        setQueryError(lastError, QStringLiteral("Failed to update account balance"), updateQuery);
        return false;
    }
    if (updateQuery.numRowsAffected() != 1) {
        lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
        return false;
    }
    return true;
}

bool categoryMatchesType(
    QSqlDatabase &database,
    int categoryId,
    const QString &type,
    QString &lastError)
{
    QSqlQuery query(database);
    query.prepare(
        QStringLiteral("SELECT Type FROM Category WHERE ID_Category = ?"));
    query.addBindValue(categoryId);
    if (!query.exec()) {
        setQueryError(lastError, QStringLiteral("Failed to validate category"), query);
        return false;
    }
    if (!query.next()) {
        lastError = QStringLiteral("Category %1 does not exist").arg(categoryId);
        return false;
    }
    if (query.value(0).toString() != type) {
        lastError = QStringLiteral("Category type does not match transaction type");
        return false;
    }
    return true;
}

bool isValidTransaction(const Transaction &transaction, QString &lastError)
{
    if (transaction.getAccountId() <= 0) {
        lastError = QStringLiteral("Account ID must be positive");
    } else if (transaction.getCategoryId() <= 0) {
        lastError = QStringLiteral("Category ID must be positive");
    } else if (transaction.getAmount() <= 0) {
        lastError = QStringLiteral("Transaction amount must be positive");
    } else if (transaction.getNote().trimmed().size() > 500) {
        lastError = QStringLiteral("Transaction note cannot exceed 500 characters");
    } else if (!isValidIsoDate(transaction.getDate())) {
        lastError = QStringLiteral("Transaction date must be a valid ISO date (YYYY-MM-DD)");
    } else {
        return true;
    }
    return false;
}

QString transactionType(bool isIncome)
{
    return isIncome ? QStringLiteral("in") : QStringLiteral("out");
}

qint64 signedAmount(qint64 amount, bool isIncome)
{
    return isIncome ? amount : -amount;
}

// A null QString would be bound as SQL NULL; the Note column is NOT NULL.
QString noteForStorage(const Transaction &transaction)
{
    const QString note = transaction.getNote().trimmed();
    return note.isNull() ? QString(QLatin1String("")) : note;
}

const char *kTransactionColumns =
    "ID_Transaction, ID_Account, ID_Category, Date, Amount, Type, Note";

Transaction transactionFromQuery(const QSqlQuery &query)
{
    return Transaction(
        query.value(0).toInt(),
        query.value(1).toInt(),
        query.value(2).toInt(),
        query.value(3).toString(),
        query.value(4).toLongLong(),
        query.value(5).toString() == QStringLiteral("in"),
        query.value(6).toString());
}

bool validateFilter(const TransactionFilter &filter, QString &lastError)
{
    if ((!filter.dateFrom.isEmpty() && !isValidIsoDate(filter.dateFrom)) ||
        (!filter.dateTo.isEmpty() && !isValidIsoDate(filter.dateTo))) {
        lastError = QStringLiteral("Transaction filter dates must be valid ISO dates (YYYY-MM-DD)");
    } else if (!filter.dateFrom.isEmpty() && !filter.dateTo.isEmpty() &&
               filter.dateFrom > filter.dateTo) {
        lastError = QStringLiteral("Transaction filter start date must not be after end date");
    } else if (filter.categoryId < 0) {
        lastError = QStringLiteral("Category filter ID cannot be negative");
    } else if (filter.minimumAmount < 0 || filter.maximumAmount < 0 ||
               (filter.maximumAmount != 0 && filter.minimumAmount > filter.maximumAmount)) {
        lastError = QStringLiteral("Transaction amount filter range is invalid");
    } else {
        return true;
    }
    return false;
}

QString filterClauses(const TransactionFilter &filter)
{
    QString sql;
    if (!filter.dateFrom.isEmpty()) sql += QStringLiteral(" AND Date >= ?");
    if (!filter.dateTo.isEmpty()) sql += QStringLiteral(" AND Date <= ?");
    if (filter.categoryId > 0) sql += QStringLiteral(" AND ID_Category = ?");
    if (filter.minimumAmount > 0) sql += QStringLiteral(" AND Amount >= ?");
    if (filter.maximumAmount > 0) sql += QStringLiteral(" AND Amount <= ?");
    return sql;
}

void bindFilter(QSqlQuery &query, const TransactionFilter &filter)
{
    if (!filter.dateFrom.isEmpty()) query.addBindValue(filter.dateFrom);
    if (!filter.dateTo.isEmpty()) query.addBindValue(filter.dateTo);
    if (filter.categoryId > 0) query.addBindValue(filter.categoryId);
    if (filter.minimumAmount > 0) query.addBindValue(filter.minimumAmount);
    if (filter.maximumAmount > 0) query.addBindValue(filter.maximumAmount);
}

// Sets `exists` when another category of the same type already has this name.
bool categoryNameExists(
    QSqlDatabase &database,
    const QString &name,
    const QString &type,
    int excludeId,
    bool &exists,
    QString &lastError)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM Category "
        "WHERE Name = ? COLLATE NOCASE AND Type = ? AND ID_Category <> ?"));
    query.addBindValue(name);
    query.addBindValue(type);
    query.addBindValue(excludeId);
    if (!query.exec() || !query.next()) {
        setQueryError(lastError, QStringLiteral("Failed to check category name"), query);
        return false;
    }
    exists = query.value(0).toInt() > 0;
    return true;
}
}

Database::Database(const QString &databasePath)
{
    const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    m_db.setDatabaseName(databasePath);
}

Database::~Database()
{
    const QString connectionName = m_db.connectionName();
    m_db.close();
    m_db = QSqlDatabase();
    if (!connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool Database::execute(const QString &sql)
{
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        setQueryError(m_lastError, QStringLiteral("SQL execution failed"), query);
        return false;
    }
    return true;
}

bool Database::open()
{
    m_lastError.clear();
    if (m_db.isOpen()) {
        return initialize();
    }
    if (!m_db.open()) {
        m_lastError = QStringLiteral("Failed to open database: ") + m_db.lastError().text();
        return false;
    }
    if (!execute(QStringLiteral("PRAGMA foreign_keys = ON")) || !initialize()) {
        m_db.close();
        return false;
    }
    return true;
}

bool Database::initialize()
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start schema transaction: ") +
                      m_db.lastError().text();
        return false;
    }

    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS Bank_Account ("
            "ID_Account INTEGER PRIMARY KEY AUTOINCREMENT, "
            "Creation_date TEXT NOT NULL, "
            "Name TEXT NOT NULL, "
            "Password_Hash TEXT NOT NULL, "
            "Initial_Credit INTEGER NOT NULL CHECK (Initial_Credit >= 0), "
            "Credit INTEGER NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS Category ("
            "ID_Category INTEGER PRIMARY KEY AUTOINCREMENT, "
            "Name TEXT NOT NULL, "
            "Type TEXT NOT NULL CHECK (Type IN ('in', 'out')))"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS Transactions ("
            "ID_Transaction INTEGER PRIMARY KEY AUTOINCREMENT, "
            "Amount INTEGER NOT NULL CHECK (Amount > 0), "
            "Date TEXT NOT NULL, "
            "Type TEXT NOT NULL CHECK (Type IN ('in', 'out')), "
            "ID_Category INTEGER NOT NULL, "
            "ID_Account INTEGER NOT NULL, "
            "Note TEXT NOT NULL DEFAULT '', "
            "FOREIGN KEY (ID_Category) REFERENCES Category(ID_Category) "
            "ON UPDATE RESTRICT ON DELETE RESTRICT, "
            "FOREIGN KEY (ID_Account) REFERENCES Bank_Account(ID_Account) "
            "ON UPDATE RESTRICT ON DELETE RESTRICT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS Transfers ("
            "ID_Transfer INTEGER PRIMARY KEY AUTOINCREMENT, "
            "ID_From INTEGER NOT NULL, "
            "ID_To INTEGER NOT NULL, "
            "Date TEXT NOT NULL, "
            "Amount INTEGER NOT NULL CHECK (Amount > 0), "
            "Note TEXT NOT NULL DEFAULT '', "
            "CHECK (ID_From <> ID_To), "
            "FOREIGN KEY (ID_From) REFERENCES Bank_Account(ID_Account) "
            "ON UPDATE RESTRICT ON DELETE RESTRICT, "
            "FOREIGN KEY (ID_To) REFERENCES Bank_Account(ID_Account) "
            "ON UPDATE RESTRICT ON DELETE RESTRICT)"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_transfers_dates "
            "ON Transfers(Date, ID_Transfer)"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_transactions_account_date "
            "ON Transactions(ID_Account, Date, ID_Transaction)"),
        QStringLiteral(
            "CREATE TRIGGER IF NOT EXISTS trg_transactions_category_type_insert "
            "BEFORE INSERT ON Transactions "
            "WHEN NOT EXISTS (SELECT 1 FROM Category "
            "WHERE ID_Category = NEW.ID_Category AND Type = NEW.Type) "
            "BEGIN SELECT RAISE(ABORT, 'transaction type does not match category'); END"),
        QStringLiteral(
            "CREATE TRIGGER IF NOT EXISTS trg_transactions_category_type_update "
            "BEFORE UPDATE OF Type, ID_Category ON Transactions "
            "WHEN NOT EXISTS (SELECT 1 FROM Category "
            "WHERE ID_Category = NEW.ID_Category AND Type = NEW.Type) "
            "BEGIN SELECT RAISE(ABORT, 'transaction type does not match category'); END")
    };

    for (const QString &statement : statements) {
        if (!execute(statement)) {
            return rollbackWithError(m_db, m_lastError);
        }
    }
    // Migration: databases created before the note feature lack the Note column.
    {
        QSqlQuery columns(m_db);
        if (!columns.exec(QStringLiteral("PRAGMA table_info(Transactions)"))) {
            setQueryError(m_lastError, QStringLiteral("Failed to inspect schema"), columns);
            return rollbackWithError(m_db, m_lastError);
        }
        bool hasNote = false;
        while (columns.next()) {
            if (columns.value(1).toString() == QStringLiteral("Note")) hasNote = true;
        }
        columns.finish();
        if (!hasNote && !execute(QStringLiteral(
                "ALTER TABLE Transactions ADD COLUMN Note TEXT NOT NULL DEFAULT ''"))) {
            return rollbackWithError(m_db, m_lastError);
        }
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit schema transaction: ") +
                      m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    return true;
}

bool Database::isOpen() const
{
    return m_db.isOpen();
}

QString Database::lastError() const
{
    return m_lastError;
}

bool Database::addAccount(
    const QString &name,
    const QString &passwordHash,
    const QString &creationDate,
    qint64 initialCredit,
    int *newId)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (name.trimmed().isEmpty() || passwordHash.isEmpty()) {
        m_lastError = QStringLiteral("Account name and password hash are required");
        return false;
    }
    if (!isValidIsoDate(creationDate)) {
        m_lastError = QStringLiteral("Creation date must be a valid ISO date (YYYY-MM-DD)");
        return false;
    }
    if (initialCredit < 0) {
        m_lastError = QStringLiteral("Initial credit cannot be negative");
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start account transaction: ") +
                      m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral(
            "INSERT INTO Bank_Account "
            "(Creation_date, Name, Password_Hash, Initial_Credit, Credit) "
            "VALUES (?, ?, ?, ?, ?)"));
    query.addBindValue(creationDate);
    query.addBindValue(name.trimmed());
    query.addBindValue(passwordHash);
    query.addBindValue(initialCredit);
    query.addBindValue(initialCredit);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to create account"), query);
        return rollbackWithError(m_db, m_lastError);
    }

    bool idOk = false;
    const qlonglong insertedId = query.lastInsertId().toLongLong(&idOk);
    if (!idOk || insertedId <= 0 || insertedId > std::numeric_limits<int>::max()) {
        m_lastError = QStringLiteral("Generated account ID is outside the supported range");
        return rollbackWithError(m_db, m_lastError);
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit account creation: ") +
                      m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    if (newId != nullptr) {
        *newId = static_cast<int>(insertedId);
    }
    return true;
}

bool Database::getAccountBalance(int accountId, qint64 &balance)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (accountId <= 0) {
        m_lastError = QStringLiteral("Account ID must be positive");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral("SELECT Credit FROM Bank_Account WHERE ID_Account = ?"));
    query.addBindValue(accountId);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to retrieve account balance"), query);
        return false;
    }
    if (!query.next()) {
        m_lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
        return false;
    }
    balance = query.value(0).toLongLong();
    return true;
}

bool Database::addCategory(
    const QString &name,
    const QString &type,
    int *newId)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (name.trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Category name is required");
        return false;
    }
    if (type != QStringLiteral("in") && type != QStringLiteral("out")) {
        m_lastError = QStringLiteral("Category type must be 'in' or 'out'");
        return false;
    }

    bool exists = false;
    if (!categoryNameExists(m_db, name.trimmed(), type, 0, exists, m_lastError)) {
        return false;
    }
    if (exists) {
        m_lastError = QStringLiteral("A category with this name already exists");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT INTO Category (Name, Type) VALUES (?, ?)"));
    query.addBindValue(name.trimmed());
    query.addBindValue(type);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to create category"), query);
        return false;
    }

    bool idOk = false;
    const qlonglong insertedId = query.lastInsertId().toLongLong(&idOk);
    if (!idOk || insertedId <= 0 || insertedId > std::numeric_limits<int>::max()) {
        m_lastError = QStringLiteral("Generated category ID is outside the supported range");
        return false;
    }
    if (newId != nullptr) {
        *newId = static_cast<int>(insertedId);
    }
    return true;
}

bool Database::getCategories(QList<QVariantMap> &categories)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec(
            QStringLiteral(
                "SELECT ID_Category, Name, Type FROM Category "
                "ORDER BY Name COLLATE NOCASE, ID_Category"))) {
        setQueryError(m_lastError, QStringLiteral("Failed to retrieve categories"), query);
        return false;
    }

    QList<QVariantMap> result;
    while (query.next()) {
        QVariantMap category;
        category.insert(QStringLiteral("ID_Category"), query.value(0));
        category.insert(QStringLiteral("Name"), query.value(1));
        category.insert(QStringLiteral("Type"), query.value(2));
        result.append(category);
    }
    categories = result;
    return true;
}

bool Database::addTransaction(const Transaction &transaction)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (!isValidTransaction(transaction, m_lastError)) {
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start transaction: ") + m_db.lastError().text();
        return false;
    }

    const QString type = transactionType(transaction.isIncome());
    if (!categoryMatchesType(m_db, transaction.getCategoryId(), type, m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }

    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral(
            "INSERT INTO Transactions (Amount, Date, Type, ID_Category, ID_Account, Note) "
            "VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(transaction.getAmount());
    query.addBindValue(transaction.getDate());
    query.addBindValue(type);
    query.addBindValue(transaction.getCategoryId());
    query.addBindValue(transaction.getAccountId());
    query.addBindValue(noteForStorage(transaction));
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to create transaction"), query);
        return rollbackWithError(m_db, m_lastError);
    }

    if (!adjustAccountBalance(
            m_db,
            transaction.getAccountId(),
            signedAmount(transaction.getAmount(), transaction.isIncome()),
            m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit transaction: ") +
                      m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    return true;
}

bool Database::updateTransaction(const Transaction &transaction)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (transaction.getId() <= 0) {
        m_lastError = QStringLiteral("Transaction ID must be positive");
        return false;
    }
    if (!isValidTransaction(transaction, m_lastError)) {
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start transaction: ") + m_db.lastError().text();
        return false;
    }

    QSqlQuery oldQuery(m_db);
    oldQuery.prepare(
        QStringLiteral(
            "SELECT ID_Account, Amount, Type FROM Transactions "
            "WHERE ID_Transaction = ?"));
    oldQuery.addBindValue(transaction.getId());
    if (!oldQuery.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to read transaction"), oldQuery);
        return rollbackWithError(m_db, m_lastError);
    }
    if (!oldQuery.next()) {
        m_lastError = QStringLiteral("Transaction %1 does not exist").arg(transaction.getId());
        return rollbackWithError(m_db, m_lastError);
    }

    const int oldAccountId = oldQuery.value(0).toInt();
    const qint64 oldAmount = oldQuery.value(1).toLongLong();
    const bool oldIsIncome = oldQuery.value(2).toString() == QStringLiteral("in");
    oldQuery.finish();
    const QString newType = transactionType(transaction.isIncome());
    if (!categoryMatchesType(m_db, transaction.getCategoryId(), newType, m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }

    if (!adjustAccountBalance(
            m_db, oldAccountId, -signedAmount(oldAmount, oldIsIncome), m_lastError) ||
        !adjustAccountBalance(
            m_db,
            transaction.getAccountId(),
            signedAmount(transaction.getAmount(), transaction.isIncome()),
            m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }

    QSqlQuery updateQuery(m_db);
    updateQuery.prepare(
        QStringLiteral(
            "UPDATE Transactions "
            "SET Amount = ?, Date = ?, Type = ?, ID_Category = ?, ID_Account = ?, Note = ? "
            "WHERE ID_Transaction = ?"));
    updateQuery.addBindValue(transaction.getAmount());
    updateQuery.addBindValue(transaction.getDate());
    updateQuery.addBindValue(newType);
    updateQuery.addBindValue(transaction.getCategoryId());
    updateQuery.addBindValue(transaction.getAccountId());
    updateQuery.addBindValue(noteForStorage(transaction));
    updateQuery.addBindValue(transaction.getId());
    if (!updateQuery.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to update transaction"), updateQuery);
        return rollbackWithError(m_db, m_lastError);
    }
    if (updateQuery.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("Transaction %1 does not exist").arg(transaction.getId());
        return rollbackWithError(m_db, m_lastError);
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit transaction update: ") +
                      m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    return true;
}

bool Database::deleteTransaction(int transactionId)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (transactionId <= 0) {
        m_lastError = QStringLiteral("Transaction ID must be positive");
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start transaction: ") + m_db.lastError().text();
        return false;
    }

    QSqlQuery selectQuery(m_db);
    selectQuery.prepare(
        QStringLiteral(
            "SELECT ID_Account, Amount, Type FROM Transactions "
            "WHERE ID_Transaction = ?"));
    selectQuery.addBindValue(transactionId);
    if (!selectQuery.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to read transaction"), selectQuery);
        return rollbackWithError(m_db, m_lastError);
    }
    if (!selectQuery.next()) {
        m_lastError = QStringLiteral("Transaction %1 does not exist").arg(transactionId);
        return rollbackWithError(m_db, m_lastError);
    }

    const int accountId = selectQuery.value(0).toInt();
    const qint64 amount = selectQuery.value(1).toLongLong();
    const bool isIncome = selectQuery.value(2).toString() == QStringLiteral("in");
    selectQuery.finish();
    QSqlQuery deleteQuery(m_db);
    deleteQuery.prepare(
        QStringLiteral("DELETE FROM Transactions WHERE ID_Transaction = ?"));
    deleteQuery.addBindValue(transactionId);
    if (!deleteQuery.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to delete transaction"), deleteQuery);
        return rollbackWithError(m_db, m_lastError);
    }
    if (deleteQuery.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("Transaction %1 does not exist").arg(transactionId);
        return rollbackWithError(m_db, m_lastError);
    }
    if (!adjustAccountBalance(
            m_db, accountId, -signedAmount(amount, isIncome), m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit transaction deletion: ") +
                      m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    return true;
}

bool Database::getTransactions(
    int accountId,
    QList<Transaction> &transactions,
    const TransactionFilter &filter)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (accountId <= 0) {
        m_lastError = QStringLiteral("Account ID must be positive");
        return false;
    }
    if (!validateFilter(filter, m_lastError)) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral("SELECT ") + QLatin1String(kTransactionColumns) +
        QStringLiteral(" FROM Transactions WHERE ID_Account = ?") + filterClauses(filter) +
        QStringLiteral(" ORDER BY Date, ID_Transaction"));
    query.addBindValue(accountId);
    bindFilter(query, filter);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to retrieve transactions"), query);
        return false;
    }

    QList<Transaction> result;
    while (query.next()) {
        result.append(transactionFromQuery(query));
    }
    transactions = result;
    return true;
}

bool Database::getRecentTransactions(int accountId, int count, QList<Transaction> &transactions)
{
    m_lastError.clear();
    if (!isOpen() || accountId <= 0 || count <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Invalid account or count")
                               : QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral("SELECT ") + QLatin1String(kTransactionColumns) +
        QStringLiteral(" FROM Transactions WHERE ID_Account = ? "
                       "ORDER BY Date DESC, ID_Transaction DESC LIMIT ?"));
    query.addBindValue(accountId);
    query.addBindValue(count);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to retrieve transactions"), query);
        return false;
    }
    QList<Transaction> result;
    while (query.next()) {
        result.append(transactionFromQuery(query));
    }
    transactions = result;
    return true;
}

bool Database::getTotals(
    int accountId,
    qint64 &income,
    qint64 &expenses,
    const TransactionFilter &filter)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (accountId <= 0) {
        m_lastError = QStringLiteral("Account ID must be positive");
        return false;
    }
    if (!validateFilter(filter, m_lastError)) {
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral("SELECT Type, SUM(Amount) FROM Transactions WHERE ID_Account = ?") +
        filterClauses(filter) + QStringLiteral(" GROUP BY Type"));
    query.addBindValue(accountId);
    bindFilter(query, filter);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to compute totals"), query);
        return false;
    }
    income = 0;
    expenses = 0;
    while (query.next()) {
        (query.value(0).toString() == QStringLiteral("in") ? income : expenses) =
            query.value(1).toLongLong();
    }
    return true;
}

bool Database::getExpensesByCategory(
    int accountId,
    const TransactionFilter &filter,
    QList<CategoryTotal> &totals)
{
    m_lastError.clear();
    if (!isOpen() || accountId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Account ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    if (!validateFilter(filter, m_lastError)) {
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(
        QStringLiteral("SELECT ID_Category, SUM(Amount) FROM Transactions "
                       "WHERE ID_Account = ? AND Type = 'out'") +
        filterClauses(filter) +
        QStringLiteral(" GROUP BY ID_Category ORDER BY SUM(Amount) DESC, ID_Category"));
    query.addBindValue(accountId);
    bindFilter(query, filter);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to compute category totals"), query);
        return false;
    }
    QList<CategoryTotal> result;
    while (query.next()) {
        result.append({query.value(0).toInt(), query.value(1).toLongLong()});
    }
    totals = result;
    return true;
}

bool Database::getMonthlyTotals(int accountId, const QString &fromDate, QList<MonthlyTotal> &totals)
{
    m_lastError.clear();
    if (!isOpen() || accountId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Account ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    if (!isValidIsoDate(fromDate)) {
        m_lastError = QStringLiteral("Start date must be a valid ISO date (YYYY-MM-DD)");
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT substr(Date, 1, 7), Type, SUM(Amount) FROM Transactions "
        "WHERE ID_Account = ? AND Date >= ? "
        "GROUP BY substr(Date, 1, 7), Type ORDER BY substr(Date, 1, 7)"));
    query.addBindValue(accountId);
    query.addBindValue(fromDate);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to compute monthly totals"), query);
        return false;
    }
    QList<MonthlyTotal> result;
    QHash<QString, int> indexByMonth;
    while (query.next()) {
        const QString month = query.value(0).toString();
        if (!indexByMonth.contains(month)) {
            indexByMonth.insert(month, result.size());
            result.append(MonthlyTotal{month, 0, 0});
        }
        MonthlyTotal &entry = result[indexByMonth.value(month)];
        (query.value(1).toString() == QStringLiteral("in") ? entry.income : entry.expenses) =
            query.value(2).toLongLong();
    }
    totals = result;
    return true;
}

bool Database::getFirstAccountId(int &accountId)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT ID_Account FROM Bank_Account ORDER BY ID_Account LIMIT 1"))) {
        setQueryError(m_lastError, QStringLiteral("Failed to read accounts"), query);
        return false;
    }
    accountId = query.next() ? query.value(0).toInt() : 0;
    return true;
}

bool Database::getAccountInitialCredit(int accountId, qint64 &initialCredit)
{
    m_lastError.clear();
    if (!isOpen() || accountId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Account ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT Initial_Credit FROM Bank_Account WHERE ID_Account = ?"));
    query.addBindValue(accountId);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to read initial credit"), query);
        return false;
    }
    if (!query.next()) {
        m_lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
        return false;
    }
    initialCredit = query.value(0).toLongLong();
    return true;
}

bool Database::setAccountInitialCredit(int accountId, qint64 initialCredit)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (accountId <= 0) {
        m_lastError = QStringLiteral("Account ID must be positive");
        return false;
    }
    if (initialCredit < 0) {
        m_lastError = QStringLiteral("Initial credit cannot be negative");
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start transaction: ") + m_db.lastError().text();
        return false;
    }
    qint64 oldInitial = 0;
    {
        QSqlQuery select(m_db);
        select.prepare(QStringLiteral("SELECT Initial_Credit FROM Bank_Account WHERE ID_Account = ?"));
        select.addBindValue(accountId);
        if (!select.exec()) {
            setQueryError(m_lastError, QStringLiteral("Failed to read initial credit"), select);
            return rollbackWithError(m_db, m_lastError);
        }
        if (!select.next()) {
            m_lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
            return rollbackWithError(m_db, m_lastError);
        }
        oldInitial = select.value(0).toLongLong();
    }
    // Both values are >= 0, so the difference cannot overflow.
    if (!adjustAccountBalance(m_db, accountId, initialCredit - oldInitial, m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }
    QSqlQuery update(m_db);
    update.prepare(QStringLiteral("UPDATE Bank_Account SET Initial_Credit = ? WHERE ID_Account = ?"));
    update.addBindValue(initialCredit);
    update.addBindValue(accountId);
    if (!update.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to update initial credit"), update);
        return rollbackWithError(m_db, m_lastError);
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit initial credit: ") + m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    return true;
}

bool Database::getCategoryUsage(int categoryId, int &transactionCount)
{
    m_lastError.clear();
    if (!isOpen() || categoryId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Category ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM Transactions WHERE ID_Category = ?"));
    query.addBindValue(categoryId);
    if (!query.exec() || !query.next()) {
        setQueryError(m_lastError, QStringLiteral("Failed to read category usage"), query);
        return false;
    }
    transactionCount = query.value(0).toInt();
    return true;
}

bool Database::updateCategory(int categoryId, const QString &name)
{
    m_lastError.clear();
    if (!isOpen() || categoryId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Category ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        m_lastError = QStringLiteral("Category name is required");
        return false;
    }
    QSqlQuery select(m_db);
    select.prepare(QStringLiteral("SELECT Type FROM Category WHERE ID_Category = ?"));
    select.addBindValue(categoryId);
    if (!select.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to read category"), select);
        return false;
    }
    if (!select.next()) {
        m_lastError = QStringLiteral("Category %1 does not exist").arg(categoryId);
        return false;
    }
    const QString type = select.value(0).toString();
    select.finish();
    bool exists = false;
    if (!categoryNameExists(m_db, trimmed, type, categoryId, exists, m_lastError)) {
        return false;
    }
    if (exists) {
        m_lastError = QStringLiteral("A category with this name already exists");
        return false;
    }
    QSqlQuery update(m_db);
    update.prepare(QStringLiteral("UPDATE Category SET Name = ? WHERE ID_Category = ?"));
    update.addBindValue(trimmed);
    update.addBindValue(categoryId);
    if (!update.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to rename category"), update);
        return false;
    }
    return true;
}

bool Database::deleteCategory(int categoryId)
{
    int usage = 0;
    if (!getCategoryUsage(categoryId, usage)) {
        return false;
    }
    if (usage > 0) {
        m_lastError = QStringLiteral("Category is used by %1 transaction(s)").arg(usage);
        return false;
    }
    QSqlQuery remove(m_db);
    remove.prepare(QStringLiteral("DELETE FROM Category WHERE ID_Category = ?"));
    remove.addBindValue(categoryId);
    if (!remove.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to delete category"), remove);
        return false;
    }
    if (remove.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("Category %1 does not exist").arg(categoryId);
        return false;
    }
    return true;
}

bool Database::getAccounts(QList<QVariantMap> &accounts)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "SELECT ID_Account, Name, Initial_Credit, Credit FROM Bank_Account ORDER BY ID_Account"))) {
        setQueryError(m_lastError, QStringLiteral("Failed to read accounts"), query);
        return false;
    }
    QList<QVariantMap> result;
    while (query.next()) {
        QVariantMap account;
        account.insert(QStringLiteral("ID_Account"), query.value(0).toInt());
        account.insert(QStringLiteral("Name"), query.value(1).toString());
        account.insert(QStringLiteral("Initial_Credit"), query.value(2).toLongLong());
        account.insert(QStringLiteral("Credit"), query.value(3).toLongLong());
        result.append(account);
    }
    accounts = result;
    return true;
}

bool Database::renameAccount(int accountId, const QString &name)
{
    m_lastError.clear();
    if (!isOpen() || accountId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Account ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        m_lastError = QStringLiteral("Account name is required");
        return false;
    }
    QSqlQuery update(m_db);
    update.prepare(QStringLiteral("UPDATE Bank_Account SET Name = ? WHERE ID_Account = ?"));
    update.addBindValue(trimmed);
    update.addBindValue(accountId);
    if (!update.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to rename account"), update);
        return false;
    }
    if (update.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
        return false;
    }
    return true;
}

bool Database::deleteAccount(int accountId)
{
    m_lastError.clear();
    if (!isOpen() || accountId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Account ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery count(m_db);
    count.prepare(QStringLiteral(
        "SELECT (SELECT COUNT(*) FROM Bank_Account), "
        "(SELECT COUNT(*) FROM Transactions WHERE ID_Account = ?), "
        "(SELECT COUNT(*) FROM Transfers WHERE ID_From = ? OR ID_To = ?)"));
    count.addBindValue(accountId);
    count.addBindValue(accountId);
    count.addBindValue(accountId);
    if (!count.exec() || !count.next()) {
        setQueryError(m_lastError, QStringLiteral("Failed to inspect account"), count);
        return false;
    }
    if (count.value(0).toInt() <= 1) {
        m_lastError = QStringLiteral("The last account cannot be deleted");
        return false;
    }
    if (count.value(1).toInt() > 0 || count.value(2).toInt() > 0) {
        m_lastError = QStringLiteral("Only accounts without transactions or transfers can be deleted");
        return false;
    }
    QSqlQuery remove(m_db);
    remove.prepare(QStringLiteral("DELETE FROM Bank_Account WHERE ID_Account = ?"));
    remove.addBindValue(accountId);
    if (!remove.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to delete account"), remove);
        return false;
    }
    if (remove.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
        return false;
    }
    return true;
}

bool Database::addTransfer(
    int fromAccountId,
    int toAccountId,
    qint64 amount,
    const QString &date,
    const QString &note,
    int *newId)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (fromAccountId <= 0 || toAccountId <= 0 || fromAccountId == toAccountId) {
        m_lastError = QStringLiteral("A transfer needs two different accounts");
        return false;
    }
    if (amount <= 0) {
        m_lastError = QStringLiteral("Transfer amount must be positive");
        return false;
    }
    if (!isValidIsoDate(date)) {
        m_lastError = QStringLiteral("Transfer date must be a valid ISO date (YYYY-MM-DD)");
        return false;
    }
    if (note.trimmed().size() > 500) {
        m_lastError = QStringLiteral("Transfer note cannot exceed 500 characters");
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start transaction: ") + m_db.lastError().text();
        return false;
    }
    if (!adjustAccountBalance(m_db, fromAccountId, -amount, m_lastError) ||
        !adjustAccountBalance(m_db, toAccountId, amount, m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }
    QSqlQuery insert(m_db);
    insert.prepare(QStringLiteral(
        "INSERT INTO Transfers (ID_From, ID_To, Date, Amount, Note) VALUES (?, ?, ?, ?, ?)"));
    insert.addBindValue(fromAccountId);
    insert.addBindValue(toAccountId);
    insert.addBindValue(date);
    insert.addBindValue(amount);
    const QString trimmedNote = note.trimmed();
    insert.addBindValue(trimmedNote.isNull() ? QString(QLatin1String("")) : trimmedNote);
    if (!insert.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to create transfer"), insert);
        return rollbackWithError(m_db, m_lastError);
    }
    const int id = insert.lastInsertId().toInt();
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit transfer: ") + m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    if (newId) *newId = id;
    return true;
}

bool Database::deleteTransfer(int transferId)
{
    m_lastError.clear();
    if (!isOpen() || transferId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Transfer ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start transaction: ") + m_db.lastError().text();
        return false;
    }
    int from = 0;
    int to = 0;
    qint64 amount = 0;
    {
        QSqlQuery select(m_db);
        select.prepare(QStringLiteral("SELECT ID_From, ID_To, Amount FROM Transfers WHERE ID_Transfer = ?"));
        select.addBindValue(transferId);
        if (!select.exec()) {
            setQueryError(m_lastError, QStringLiteral("Failed to read transfer"), select);
            return rollbackWithError(m_db, m_lastError);
        }
        if (!select.next()) {
            m_lastError = QStringLiteral("Transfer %1 does not exist").arg(transferId);
            return rollbackWithError(m_db, m_lastError);
        }
        from = select.value(0).toInt();
        to = select.value(1).toInt();
        amount = select.value(2).toLongLong();
    }
    if (!adjustAccountBalance(m_db, from, amount, m_lastError) ||
        !adjustAccountBalance(m_db, to, -amount, m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }
    QSqlQuery remove(m_db);
    remove.prepare(QStringLiteral("DELETE FROM Transfers WHERE ID_Transfer = ?"));
    remove.addBindValue(transferId);
    if (!remove.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to delete transfer"), remove);
        return rollbackWithError(m_db, m_lastError);
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit transfer deletion: ") + m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    return true;
}

bool Database::getTransfers(int accountId, QList<Transfer> &transfers, const TransactionFilter &filter)
{
    m_lastError.clear();
    if (!isOpen() || accountId <= 0) {
        m_lastError = isOpen() ? QStringLiteral("Account ID must be positive")
                               : QStringLiteral("Database is not open");
        return false;
    }
    if (!validateFilter(filter, m_lastError)) {
        return false;
    }
    QString sql = QStringLiteral(
        "SELECT ID_Transfer, ID_From, ID_To, Date, Amount, Note FROM Transfers "
        "WHERE (ID_From = ? OR ID_To = ?)");
    if (!filter.dateFrom.isEmpty()) sql += QStringLiteral(" AND Date >= ?");
    if (!filter.dateTo.isEmpty()) sql += QStringLiteral(" AND Date <= ?");
    if (filter.minimumAmount > 0) sql += QStringLiteral(" AND Amount >= ?");
    if (filter.maximumAmount > 0) sql += QStringLiteral(" AND Amount <= ?");
    sql += QStringLiteral(" ORDER BY Date, ID_Transfer");
    QSqlQuery query(m_db);
    query.prepare(sql);
    query.addBindValue(accountId);
    query.addBindValue(accountId);
    if (!filter.dateFrom.isEmpty()) query.addBindValue(filter.dateFrom);
    if (!filter.dateTo.isEmpty()) query.addBindValue(filter.dateTo);
    if (filter.minimumAmount > 0) query.addBindValue(filter.minimumAmount);
    if (filter.maximumAmount > 0) query.addBindValue(filter.maximumAmount);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to retrieve transfers"), query);
        return false;
    }
    QList<Transfer> result;
    while (query.next()) {
        Transfer t;
        t.id = query.value(0).toInt();
        t.fromAccountId = query.value(1).toInt();
        t.toAccountId = query.value(2).toInt();
        t.date = query.value(3).toString();
        t.amount = query.value(4).toLongLong();
        t.note = query.value(5).toString();
        result.append(t);
    }
    transfers = result;
    return true;
}
