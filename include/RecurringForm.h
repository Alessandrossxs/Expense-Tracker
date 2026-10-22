#pragma once

#include "Recurring.h"

#include <QList>
#include <QVariantMap>
#include <QWidget>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;

// Fields of the add/edit recurring-rule popup (no buttons: Popup provides them).
class RecurringForm : public QWidget
{
public:
    RecurringForm(const QList<QVariantMap> &categories, int accountId, bool editing,
                  QWidget *parent = nullptr);
    void setRule(const RecurringRule &rule);
    RecurringRule rule() const;
    QString validate() const; // empty when the form is valid

private:
    QList<QVariantMap> m_categories;
    int m_accountId;
    int m_ruleId = 0;
    QComboBox *m_type = nullptr;
    QComboBox *m_category = nullptr;
    QDoubleSpinBox *m_amount = nullptr;
    QLineEdit *m_note = nullptr;
    QDoubleSpinBox *m_interval = nullptr;
    QComboBox *m_unit = nullptr;
    QDateEdit *m_start = nullptr;
    QComboBox *m_endMode = nullptr;
    QDateEdit *m_end = nullptr;
    QWidget *m_endField = nullptr;
    void reloadCategories(int preferredCategory = 0);
    void updateUnitLabels();
};
