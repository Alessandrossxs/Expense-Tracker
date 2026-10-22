#pragma once

#include "Transaction.h"

#include <QList>
#include <QVariantMap>
#include <QWidget>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;

// Fields of the add/edit transaction popup (no buttons: Popup provides them).
class TransactionForm : public QWidget
{
public:
    TransactionForm(const QList<QVariantMap> &categories, int accountId, QWidget *parent = nullptr);
    void setTransaction(const Transaction &transaction);
    Transaction transaction() const;
    QString validate() const; // empty when the form is valid

private:
    QList<QVariantMap> m_categories;
    int m_accountId;
    int m_transactionId = 0;
    QComboBox *m_type = nullptr;
    QComboBox *m_category = nullptr;
    QDateEdit *m_date = nullptr;
    QDoubleSpinBox *m_amount = nullptr;
    QLineEdit *m_note = nullptr;
    void reloadCategories(int preferredCategory = 0);
};
