#include "TransactionForm.h"

#include "Popup.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QtMath>

TransactionForm::TransactionForm(const QList<QVariantMap> &categories, int accountId, QWidget *parent)
    : QWidget(parent), m_categories(categories), m_accountId(accountId)
{
    m_type = new QComboBox;
    m_type->addItem(tr("Uscita"), false);
    m_type->addItem(tr("Entrata"), true);
    m_category = new QComboBox;
    m_date = new QDateEdit(QDate::currentDate());
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_amount = new QDoubleSpinBox;
    m_amount->setRange(0.01, 999999999.99);
    m_amount->setDecimals(2);
    m_amount->setSingleStep(1.0);
    m_amount->setSuffix(QStringLiteral(" €"));
    m_note = new QLineEdit;
    m_note->setMaxLength(200);
    m_note->setPlaceholderText(tr("Facoltativa, es. Spesa al supermercato"));
    m_note->setClearButtonEnabled(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);
    auto *row = new QHBoxLayout;
    row->setSpacing(12);
    row->addWidget(Popup::field(tr("Tipo"), m_type, this));
    row->addWidget(Popup::field(tr("Data"), m_date, this));
    layout->addLayout(row);
    layout->addWidget(Popup::field(tr("Categoria"), m_category, this));
    layout->addWidget(Popup::field(tr("Importo"), m_amount, this));
    layout->addWidget(Popup::field(tr("Nota"), m_note, this));

    connect(m_type, &QComboBox::currentIndexChanged, this, [this] { reloadCategories(); });
    reloadCategories();
}

void TransactionForm::reloadCategories(int preferredCategory)
{
    const bool income = m_type->currentData().toBool();
    m_category->clear();
    for (const QVariantMap &category : m_categories) {
        const QString type = category.value(QStringLiteral("Type")).toString();
        if ((income && type == QStringLiteral("in")) || (!income && type == QStringLiteral("out"))) {
            m_category->addItem(category.value(QStringLiteral("Name")).toString(),
                                category.value(QStringLiteral("ID_Category")).toInt());
        }
    }
    const int index = m_category->findData(preferredCategory);
    if (index >= 0)
        m_category->setCurrentIndex(index);
}

void TransactionForm::setTransaction(const Transaction &transaction)
{
    m_transactionId = transaction.getId();
    m_type->setCurrentIndex(transaction.isIncome() ? 1 : 0);
    reloadCategories(transaction.getCategoryId());
    m_date->setDate(QDate::fromString(transaction.getDate(), Qt::ISODate));
    m_amount->setValue(static_cast<double>(transaction.getAmount()) / 100.0);
    m_note->setText(transaction.getNote());
}

QString TransactionForm::validate() const
{
    if (m_category->currentIndex() < 0)
        return tr("Crea prima una categoria per questo tipo di transazione.");
    return QString();
}

Transaction TransactionForm::transaction() const
{
    return Transaction(m_transactionId, m_accountId, m_category->currentData().toInt(),
                       m_date->date().toString(Qt::ISODate), qRound64(m_amount->value() * 100.0),
                       m_type->currentData().toBool(), m_note->text().trimmed());
}
