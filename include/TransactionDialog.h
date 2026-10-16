#pragma once

#include "Transaction.h"

#include <QDialog>
#include <QList>
#include <QVariantMap>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;

class TransactionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransactionDialog(
        const QList<QVariantMap> &categories,
        int accountId,
        QWidget *parent = nullptr);
    void setTransaction(const Transaction &transaction);
    Transaction transaction() const;

private:
    QList<QVariantMap> m_categories;
    int m_accountId;
    int m_transactionId = 0;
    QComboBox *m_type = nullptr;
    QComboBox *m_category = nullptr;
    QDateEdit *m_date = nullptr;
    QDoubleSpinBox *m_amount = nullptr;
    void reloadCategories(int preferredCategory = 0);
};