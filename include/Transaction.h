#pragma once

#include <QString>

class Transaction
{
private:
    int m_id = 0;         ///< Unique identifier for the transaction.
    int m_accountId = 0;  ///< ID of the associated account.
    int m_categoryId = 0; ///< ID of the associated category.
    QString m_date;       ///< Date of the transaction in ISO format (YYYY-MM-DD).
    qint64 m_amount = 0;  ///< Amount of the transaction in cents.
    bool m_isIncome = false; ///< True for income and false for expense.
    QString m_notes;

public:
    /**
     * @brief Constructs an empty transaction.
     */
    Transaction() = default;

    /**
     * @brief Constructs a transaction with the supplied values.
     * @param id Unique transaction ID, or 0 for a transaction not yet stored.
     * @param accountId ID of the associated account.
     * @param categoryId ID of the associated category.
     * @param date Transaction date in ISO format (YYYY-MM-DD).
     * @param amount Transaction amount in cents.
     * @param isIncome True for income and false for expense.
     */
    Transaction(
        int id,
        int accountId,
        int categoryId,
        const QString &date,
        qint64 amount,
        bool isIncome,
        const QString &notes = QString());

    /**
     * @brief Returns the transaction ID.
     * @return Unique transaction ID.
     */
    int getId() const;

    /**
     * @brief Returns the associated account ID.
     * @return Account ID.
     */
    int getAccountId() const;

    /**
     * @brief Returns the associated category ID.
     * @return Category ID.
     */
    int getCategoryId() const;

    /**
     * @brief Returns the transaction date.
     * @return Date in ISO format (YYYY-MM-DD).
     */
    QString getDate() const;

    /**
     * @brief Returns the transaction amount.
     * @return Amount in cents.
     */
    qint64 getAmount() const;

    /**
     * @brief Checks whether this transaction is income.
     * @return True for income and false for expense.
     */
    bool isIncome() const;
    QString getNotes() const;

    /**
     * @brief Sets the transaction ID.
     * @param id Unique transaction ID.
     */
    void setId(int id);

    /**
     * @brief Sets the associated account ID.
     * @param accountId Account ID.
     */
    void setAccountId(int accountId);

    /**
     * @brief Sets the associated category ID.
     * @param categoryId Category ID.
     */
    void setCategoryId(int categoryId);

    /**
     * @brief Sets the transaction date.
     * @param date Date in ISO format (YYYY-MM-DD).
     */
    void setDate(const QString &date);

    /**
     * @brief Sets the transaction amount.
     * @param amount Amount in cents.
     */
    void setAmount(qint64 amount);

    /**
     * @brief Sets whether the transaction is income.
     * @param isIncome True for income and false for expense.
     */
    void setIncome(bool isIncome);
    void setNotes(const QString &notes);
};