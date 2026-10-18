#include "TransactionDialog.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QtMath>

TransactionDialog::TransactionDialog(
    const QList<QVariantMap> &categories,
    int accountId,
    QWidget *parent)
    : QDialog(parent), m_categories(categories), m_accountId(accountId)
{
    setWindowTitle(tr("Nuova transazione"));
    setModal(true);
    setMinimumWidth(380);

    m_type = new QComboBox(this);
    m_type->addItem(tr("Uscita"), false);
    m_type->addItem(tr("Entrata"), true);

    m_category = new QComboBox(this);
    m_date = new QDateEdit(QDate::currentDate(), this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));

    m_amount = new QDoubleSpinBox(this);
    m_amount->setRange(0.01, 999999999.99);
    m_amount->setDecimals(2);
    m_amount->setSingleStep(1.0);
    m_amount->setSuffix(QStringLiteral(" €"));

    m_note = new QLineEdit(this);
    m_note->setMaxLength(200);
    m_note->setPlaceholderText(tr("Facoltativa, es. Spesa al supermercato"));

    auto *form = new QFormLayout;
    form->addRow(tr("Tipo"), m_type);
    form->addRow(tr("Categoria"), m_category);
    form->addRow(tr("Data"), m_date);
    form->addRow(tr("Importo"), m_amount);
    form->addRow(tr("Nota"), m_note);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (m_category->currentIndex() < 0) {
            QMessageBox::warning(this, tr("Categoria mancante"),
                                 tr("Crea prima una categoria per questo tipo di transazione."));
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_type, &QComboBox::currentIndexChanged, this, [this] {
        reloadCategories();
    });

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
    reloadCategories();
}

void TransactionDialog::reloadCategories(int preferredCategory)
{
    const bool income = m_type->currentData().toBool();
    m_category->clear();
    for (const QVariantMap &category : m_categories) {
        const QString type = category.value(QStringLiteral("Type")).toString();
        if ((income && type == QStringLiteral("in")) ||
            (!income && type == QStringLiteral("out"))) {
            m_category->addItem(
                category.value(QStringLiteral("Name")).toString(),
                category.value(QStringLiteral("ID_Category")).toInt());
        }
    }
    const int index = m_category->findData(preferredCategory);
    if (index >= 0)
        m_category->setCurrentIndex(index);
}

void TransactionDialog::setTransaction(const Transaction &transaction)
{
    m_transactionId = transaction.getId();
    m_type->setCurrentIndex(transaction.isIncome() ? 1 : 0);
    reloadCategories(transaction.getCategoryId());
    m_date->setDate(QDate::fromString(transaction.getDate(), Qt::ISODate));
    m_amount->setValue(static_cast<double>(transaction.getAmount()) / 100.0);
    m_note->setText(transaction.getNote());
    setWindowTitle(tr("Modifica transazione"));
}

Transaction TransactionDialog::transaction() const
{
    const qint64 cents = qRound64(m_amount->value() * 100.0);
    return Transaction(
        m_transactionId,
        m_accountId,
        m_category->currentData().toInt(),
        m_date->date().toString(Qt::ISODate),
        cents,
        m_type->currentData().toBool(),
        m_note->text().trimmed());
}
