#include "Database.h"

#include "Transaction.h"

#include <QDate>
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

bool isValidAccountType(const QString &type)
{
    return type == QStringLiteral("checking") ||
           type == QStringLiteral("debit_card") ||
           type == QStringLiteral("credit_card") ||
           type == QStringLiteral("savings") ||
           type == QStringLiteral("cash") ||
           type == QStringLiteral("other");
}

qint64 signedAmount(qint64 amount, bool isIncome)
{
    return isIncome ? amount : -amount;
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
    if (m_db.databaseName().trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Database path is empty; refusing to open a temporary database");
        return false;
    }
    if (m_db.isOpen()) {
        return initialize();
    }
    if (!m_db.open()) {
        m_lastError = QStringLiteral("Failed to open database: ") + m_db.lastError().text();
        return false;
    }
    if (!execute(QStringLiteral("PRAGMA foreign_keys = ON")) ||
        !execute(QStringLiteral("PRAGMA secure_delete = ON")) ||
        !initialize()) {
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
            "Credit INTEGER NOT NULL, "
            "Account_Type TEXT NOT NULL DEFAULT 'checking')"),
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
            "Notes TEXT NOT NULL DEFAULT '', "
            "FOREIGN KEY (ID_Category) REFERENCES Category(ID_Category) "
            "ON UPDATE RESTRICT ON DELETE RESTRICT, "
            "FOREIGN KEY (ID_Account) REFERENCES Bank_Account(ID_Account) "
            "ON UPDATE RESTRICT ON DELETE RESTRICT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS Account_Transfers ("
            "ID_Transfer INTEGER PRIMARY KEY AUTOINCREMENT, "
            "Source_Account INTEGER NOT NULL, "
            "Destination_Account INTEGER NOT NULL, "
            "Date TEXT NOT NULL, "
            "Amount INTEGER NOT NULL CHECK (Amount > 0), "
            "CHECK (Source_Account != Destination_Account), "
            "FOREIGN KEY (Source_Account) REFERENCES Bank_Account(ID_Account) "
            "ON UPDATE RESTRICT ON DELETE RESTRICT, "
            "FOREIGN KEY (Destination_Account) REFERENCES Bank_Account(ID_Account) "
            "ON UPDATE RESTRICT ON DELETE RESTRICT)"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_transfers_source_date "
            "ON Account_Transfers(Source_Account, Date, ID_Transfer)"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_transfers_destination_date "
            "ON Account_Transfers(Destination_Account, Date, ID_Transfer)"),
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
    QSqlQuery accountColumns(m_db);
    if (!accountColumns.exec(QStringLiteral("PRAGMA table_info(Bank_Account)"))) {
        setQueryError(m_lastError, QStringLiteral("Failed to inspect account schema"), accountColumns);
        return rollbackWithError(m_db, m_lastError);
    }
    bool hasAccountType = false;
    while (accountColumns.next()) {
        if (accountColumns.value(1).toString() == QStringLiteral("Account_Type")) {
            hasAccountType = true;
            break;
        }
    }
    accountColumns.finish();
    if (!hasAccountType &&
        !execute(QStringLiteral(
            "ALTER TABLE Bank_Account ADD COLUMN Account_Type TEXT NOT NULL "
            "DEFAULT 'checking'"))) {
        return rollbackWithError(m_db, m_lastError);
    }
    QSqlQuery transactionColumns(m_db);
    if (!transactionColumns.exec(QStringLiteral("PRAGMA table_info(Transactions)"))) {
        setQueryError(m_lastError, QStringLiteral("Failed to inspect transaction schema"),
                      transactionColumns);
        return rollbackWithError(m_db, m_lastError);
    }
    bool hasNotes = false;
    while (transactionColumns.next()) {
        if (transactionColumns.value(1).toString() == QStringLiteral("Notes")) {
            hasNotes = true;
            break;
        }
    }
    transactionColumns.finish();
    if (!hasNotes &&
        !execute(QStringLiteral(
            "ALTER TABLE Transactions ADD COLUMN Notes TEXT NOT NULL DEFAULT ''"))) {
        return rollbackWithError(m_db, m_lastError);
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
    int *newId,
    const QString &accountType)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (name.trimmed().isEmpty() || passwordHash.isEmpty() ||
        !isValidAccountType(accountType)) {
        m_lastError = QStringLiteral("Account name, password hash, and valid account type are required");
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
            "(Creation_date, Name, Password_Hash, Initial_Credit, Credit, Account_Type) "
            "VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(creationDate);
    query.addBindValue(name.trimmed());
    query.addBindValue(passwordHash);
    query.addBindValue(initialCredit);
    query.addBindValue(initialCredit);
    query.addBindValue(accountType);
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

bool Database::getAccounts(QList<QVariantMap> &accounts)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "SELECT ID_Account, Name, Account_Type, Credit, "
            "(SELECT COUNT(*) FROM Transactions "
            " WHERE Transactions.ID_Account = Bank_Account.ID_Account), "
            "(SELECT COUNT(*) FROM Account_Transfers "
            " WHERE Account_Transfers.Source_Account = Bank_Account.ID_Account "
            " OR Account_Transfers.Destination_Account = Bank_Account.ID_Account) "
            "FROM Bank_Account ORDER BY ID_Account"))) {
        setQueryError(m_lastError, QStringLiteral("Failed to retrieve accounts"), query);
        return false;
    }
    QList<QVariantMap> result;
    while (query.next()) {
        QVariantMap account;
        account.insert(QStringLiteral("ID_Account"), query.value(0));
        account.insert(QStringLiteral("Name"), query.value(1));
        account.insert(QStringLiteral("Type"), query.value(2));
        account.insert(QStringLiteral("Balance"), query.value(3));
        account.insert(QStringLiteral("TransactionCount"), query.value(4));
        account.insert(QStringLiteral("TransferCount"), query.value(5));
        result.append(account);
    }
    accounts = result;
    return true;
}

bool Database::updateAccount(
    int accountId,
    const QString &name,
    const QString &accountType)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (accountId <= 0 || name.trimmed().isEmpty() || !isValidAccountType(accountType)) {
        m_lastError = QStringLiteral("Account ID, name, or type is invalid");
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE Bank_Account SET Name = ?, Account_Type = ? WHERE ID_Account = ?"));
    query.addBindValue(name.trimmed());
    query.addBindValue(accountType);
    query.addBindValue(accountId);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to update account"), query);
        return false;
    }
    if (query.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
        return false;
    }
    return true;
}

bool Database::deleteAccount(int accountId)
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
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start account deletion: ") +
                      m_db.lastError().text();
        return false;
    }
    QSqlQuery countQuery(m_db);
    if (!countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM Bank_Account"))) {
        setQueryError(m_lastError, QStringLiteral("Failed to count accounts"), countQuery);
        return rollbackWithError(m_db, m_lastError);
    }
    if (!countQuery.next() || countQuery.value(0).toInt() <= 1) {
        m_lastError = QStringLiteral("The last account cannot be deleted");
        return rollbackWithError(m_db, m_lastError);
    }
    countQuery.finish();

    QSqlQuery usageQuery(m_db);
    usageQuery.prepare(QStringLiteral(
        "SELECT "
        "(SELECT COUNT(*) FROM Transactions WHERE ID_Account = ?), "
        "(SELECT COUNT(*) FROM Account_Transfers "
        " WHERE Source_Account = ? OR Destination_Account = ?), "
        "(SELECT Credit FROM Bank_Account WHERE ID_Account = ?)"));
    usageQuery.addBindValue(accountId);
    usageQuery.addBindValue(accountId);
    usageQuery.addBindValue(accountId);
    usageQuery.addBindValue(accountId);
    if (!usageQuery.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to check account history"), usageQuery);
        return rollbackWithError(m_db, m_lastError);
    }
    if (!usageQuery.next()) {
        m_lastError = QStringLiteral("Failed to inspect account history");
        return rollbackWithError(m_db, m_lastError);
    }
    if (usageQuery.value(0).toLongLong() != 0 ||
        usageQuery.value(1).toLongLong() != 0 ||
        usageQuery.value(2).isNull() ||
        usageQuery.value(2).toLongLong() != 0) {
        m_lastError = QStringLiteral(
            "Only an existing account with no activity and a zero balance can be deleted");
        return rollbackWithError(m_db, m_lastError);
    }
    usageQuery.finish();

    QSqlQuery deleteQuery(m_db);
    deleteQuery.prepare(QStringLiteral(
        "DELETE FROM Bank_Account WHERE ID_Account = ?"));
    deleteQuery.addBindValue(accountId);
    if (!deleteQuery.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to delete account"), deleteQuery);
        return rollbackWithError(m_db, m_lastError);
    }
    if (deleteQuery.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("Account %1 does not exist").arg(accountId);
        return rollbackWithError(m_db, m_lastError);
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit account deletion: ") +
                      m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
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

bool Database::addTransfer(
    int sourceAccountId,
    int destinationAccountId,
    const QString &date,
    qint64 amount)
{
    m_lastError.clear();
    if (!isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }
    if (sourceAccountId <= 0 || destinationAccountId <= 0 ||
        sourceAccountId == destinationAccountId || amount <= 0 ||
        !isValidIsoDate(date)) {
        m_lastError = QStringLiteral("Transfer accounts, date, and amount are invalid");
        return false;
    }
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("Failed to start transfer: ") + m_db.lastError().text();
        return false;
    }
    QSqlQuery insertQuery(m_db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO Account_Transfers "
        "(Source_Account, Destination_Account, Date, Amount) VALUES (?, ?, ?, ?)"));
    insertQuery.addBindValue(sourceAccountId);
    insertQuery.addBindValue(destinationAccountId);
    insertQuery.addBindValue(date);
    insertQuery.addBindValue(amount);
    if (!insertQuery.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to create transfer"), insertQuery);
        return rollbackWithError(m_db, m_lastError);
    }
    insertQuery.finish();
    if (!adjustAccountBalance(m_db, sourceAccountId, -amount, m_lastError) ||
        !adjustAccountBalance(m_db, destinationAccountId, amount, m_lastError)) {
        return rollbackWithError(m_db, m_lastError);
    }
    if (!m_db.commit()) {
        m_lastError = QStringLiteral("Failed to commit transfer: ") + m_db.lastError().text();
        return rollbackWithError(m_db, m_lastError);
    }
    return true;
}

bool Database::getAccountTransfers(int accountId, QList<QVariantMap> &transfers)
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
    query.prepare(QStringLiteral(
        "SELECT t.ID_Transfer, t.Date, t.Amount, t.Source_Account, "
        "source.Name, destination.Name, "
        "CASE WHEN t.Source_Account = ? THEN 'out' ELSE 'in' END "
        "FROM Account_Transfers t "
        "JOIN Bank_Account source ON source.ID_Account = t.Source_Account "
        "JOIN Bank_Account destination ON destination.ID_Account = t.Destination_Account "
        "WHERE t.Source_Account = ? OR t.Destination_Account = ? "
        "ORDER BY t.Date DESC, t.ID_Transfer DESC"));
    query.addBindValue(accountId);
    query.addBindValue(accountId);
    query.addBindValue(accountId);
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to retrieve transfers"), query);
        return false;
    }
    QList<QVariantMap> result;
    while (query.next()) {
        const bool outgoing = query.value(6).toString() == QStringLiteral("out");
        QVariantMap transfer;
        transfer.insert(QStringLiteral("ID_Transfer"), query.value(0));
        transfer.insert(QStringLiteral("Date"), query.value(1));
        transfer.insert(QStringLiteral("Amount"), query.value(2));
        transfer.insert(QStringLiteral("Direction"), outgoing
                            ? QStringLiteral("out")
                            : QStringLiteral("in"));
        transfer.insert(QStringLiteral("OtherAccount"), query.value(outgoing ? 5 : 4));
        result.append(transfer);
    }
    transfers = result;
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
            "INSERT INTO Transactions "
            "(Amount, Date, Type, ID_Category, ID_Account, Notes) "
            "VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(transaction.getAmount());
    query.addBindValue(transaction.getDate());
    query.addBindValue(type);
    query.addBindValue(transaction.getCategoryId());
    query.addBindValue(transaction.getAccountId());
    query.addBindValue(transaction.getNotes().isNull()
                           ? QStringLiteral("")
                           : transaction.getNotes());
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
            "SET Amount = ?, Date = ?, Type = ?, ID_Category = ?, ID_Account = ?, Notes = ? "
            "WHERE ID_Transaction = ?"));
    updateQuery.addBindValue(transaction.getAmount());
    updateQuery.addBindValue(transaction.getDate());
    updateQuery.addBindValue(newType);
    updateQuery.addBindValue(transaction.getCategoryId());
    updateQuery.addBindValue(transaction.getAccountId());
    updateQuery.addBindValue(transaction.getNotes().isNull()
                                 ? QStringLiteral("")
                                 : transaction.getNotes());
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
    if ((!filter.dateFrom.isEmpty() && !isValidIsoDate(filter.dateFrom)) ||
        (!filter.dateTo.isEmpty() && !isValidIsoDate(filter.dateTo))) {
        m_lastError = QStringLiteral("Transaction filter dates must be valid ISO dates (YYYY-MM-DD)");
        return false;
    }
    if (!filter.dateFrom.isEmpty() && !filter.dateTo.isEmpty() &&
        filter.dateFrom > filter.dateTo) {
        m_lastError = QStringLiteral("Transaction filter start date must not be after end date");
        return false;
    }
    if (filter.categoryId < 0) {
        m_lastError = QStringLiteral("Category filter ID cannot be negative");
        return false;
    }
    if (!filter.type.isEmpty() &&
        filter.type != QStringLiteral("in") &&
        filter.type != QStringLiteral("out")) {
        m_lastError = QStringLiteral("Transaction type filter must be 'in' or 'out'");
        return false;
    }
    if (filter.minimumAmount < 0 || filter.maximumAmount < 0 ||
        (filter.maximumAmount != 0 && filter.minimumAmount > filter.maximumAmount)) {
        m_lastError = QStringLiteral("Transaction amount filter range is invalid");
        return false;
    }

    QString sql =
        QStringLiteral(
            "SELECT ID_Transaction, ID_Account, ID_Category, Date, Amount, Type, Notes "
            "FROM Transactions WHERE ID_Account = ?");
    if (!filter.dateFrom.isEmpty()) {
        sql += QStringLiteral(" AND Date >= ?");
    }
    if (!filter.dateTo.isEmpty()) {
        sql += QStringLiteral(" AND Date <= ?");
    }
    if (!filter.type.isEmpty()) {
        sql += QStringLiteral(" AND Type = ?");
    }
    if (filter.categoryId > 0) {
        sql += QStringLiteral(" AND ID_Category = ?");
    }
    if (filter.minimumAmount > 0) {
        sql += QStringLiteral(" AND Amount >= ?");
    }
    if (filter.maximumAmount > 0) {
        sql += QStringLiteral(" AND Amount <= ?");
    }
    sql += QStringLiteral(" ORDER BY Date, ID_Transaction");
    QSqlQuery query(m_db);
    query.prepare(sql);
    query.addBindValue(accountId);
    if (!filter.dateFrom.isEmpty()) {
        query.addBindValue(filter.dateFrom);
    }
    if (!filter.dateTo.isEmpty()) {
        query.addBindValue(filter.dateTo);
    }
    if (!filter.type.isEmpty()) {
        query.addBindValue(filter.type);
    }
    if (filter.categoryId > 0) {
        query.addBindValue(filter.categoryId);
    }
    if (filter.minimumAmount > 0) {
        query.addBindValue(filter.minimumAmount);
    }
    if (filter.maximumAmount > 0) {
        query.addBindValue(filter.maximumAmount);
    }
    if (!query.exec()) {
        setQueryError(m_lastError, QStringLiteral("Failed to retrieve transactions"), query);
        return false;
    }

    QList<Transaction> result;
    while (query.next()) {
        result.append(Transaction(
            query.value(0).toInt(),
            query.value(1).toInt(),
            query.value(2).toInt(),
            query.value(3).toString(),
            query.value(4).toLongLong(),
            query.value(5).toString() == QStringLiteral("in"),
            query.value(6).toString()));
    }
    transactions = result;
    return true;
}
