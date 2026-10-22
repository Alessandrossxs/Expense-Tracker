#include "RecurringForm.h"

#include "Popup.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QtMath>

RecurringForm::RecurringForm(const QList<QVariantMap> &categories, int accountId, bool editing,
                             QWidget *parent)
    : QWidget(parent), m_categories(categories), m_accountId(accountId)
{
    m_type = new QComboBox;
    m_type->addItem(tr("Uscita"), false);
    m_type->addItem(tr("Entrata"), true);
    m_category = new QComboBox;
    m_amount = new QDoubleSpinBox;
    m_amount->setRange(0.01, 999999999.99);
    m_amount->setDecimals(2);
    m_amount->setSuffix(QStringLiteral(" €"));
    m_note = new QLineEdit;
    m_note->setMaxLength(200);
    m_note->setPlaceholderText(tr("Es. Affitto, Abbonamento, Stipendio"));

    m_interval = new QDoubleSpinBox;
    m_interval->setDecimals(0);
    m_interval->setRange(1, 999);
    m_interval->setSingleStep(1);
    m_interval->setValue(1);
    m_unit = new QComboBox;
    m_unit->addItem(QString(), QStringLiteral("daily"));
    m_unit->addItem(QString(), QStringLiteral("weekly"));
    m_unit->addItem(QString(), QStringLiteral("monthly"));
    m_unit->addItem(QString(), QStringLiteral("yearly"));
    m_unit->setCurrentIndex(2);
    updateUnitLabels();

    m_start = new QDateEdit(QDate::currentDate());
    m_start->setCalendarPopup(true);
    m_start->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_endMode = new QComboBox;
    m_endMode->addItem(tr("Mai"), false);
    m_endMode->addItem(tr("In una data"), true);
    m_end = new QDateEdit(QDate::currentDate().addYears(1));
    m_end->setCalendarPopup(true);
    m_end->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(12);
    row1->addWidget(Popup::field(tr("Tipo"), m_type, this));
    row1->addWidget(Popup::field(editing ? tr("Prossima esecuzione") : tr("Prima esecuzione"), m_start, this));
    layout->addLayout(row1);
    layout->addWidget(Popup::field(tr("Categoria"), m_category, this));
    layout->addWidget(Popup::field(tr("Importo"), m_amount, this));
    auto *row2 = new QHBoxLayout;
    row2->setSpacing(12);
    row2->addWidget(Popup::field(tr("Ripeti ogni"), m_interval, this), 1);
    row2->addWidget(Popup::field(tr("Periodo"), m_unit, this), 1);
    layout->addLayout(row2);
    auto *row3 = new QHBoxLayout;
    row3->setSpacing(12);
    row3->addWidget(Popup::field(tr("Fine"), m_endMode, this), 1);
    m_endField = Popup::field(tr("Data di fine"), m_end, this);
    row3->addWidget(m_endField, 1);
    layout->addLayout(row3);
    layout->addWidget(Popup::field(tr("Descrizione"), m_note, this));

    auto *info = new QLabel(editing
        ? tr("La pianificazione riparte dalla data indicata; le transazioni già create restano nel conto.")
        : tr("Se la prima esecuzione è nel passato, le transazioni arretrate vengono create subito."), this);
    info->setObjectName(QStringLiteral("subTitle"));
    info->setWordWrap(true);
    layout->addWidget(info);

    m_endField->setVisible(false);
    connect(m_type, &QComboBox::currentIndexChanged, this, [this] { reloadCategories(); });
    connect(m_interval, &QDoubleSpinBox::valueChanged, this, [this] { updateUnitLabels(); });
    connect(m_endMode, &QComboBox::currentIndexChanged, this,
            [this] { m_endField->setVisible(m_endMode->currentData().toBool()); });
    reloadCategories();
}

void RecurringForm::updateUnitLabels()
{
    const bool one = qRound(m_interval->value()) == 1;
    m_unit->setItemText(0, one ? tr("giorno") : tr("giorni"));
    m_unit->setItemText(1, one ? tr("settimana") : tr("settimane"));
    m_unit->setItemText(2, one ? tr("mese") : tr("mesi"));
    m_unit->setItemText(3, one ? tr("anno") : tr("anni"));
}

void RecurringForm::reloadCategories(int preferredCategory)
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

void RecurringForm::setRule(const RecurringRule &rule)
{
    m_ruleId = rule.id;
    m_type->setCurrentIndex(rule.isIncome ? 1 : 0);
    reloadCategories(rule.categoryId);
    m_amount->setValue(static_cast<double>(rule.amount) / 100.0);
    m_note->setText(rule.note);
    m_interval->setValue(rule.interval);
    m_unit->setCurrentIndex(qMax(0, m_unit->findData(rule.frequency)));
    const QDate next = QDate::fromString(rule.nextDate, Qt::ISODate);
    m_start->setDate(next.isValid() ? next : QDate::fromString(rule.startDate, Qt::ISODate));
    if (!rule.endDate.isEmpty()) {
        m_endMode->setCurrentIndex(1);
        m_end->setDate(QDate::fromString(rule.endDate, Qt::ISODate));
    }
}

QString RecurringForm::validate() const
{
    if (m_category->currentIndex() < 0)
        return tr("Crea prima una categoria per questo tipo di movimento.");
    if (m_endMode->currentData().toBool() && m_end->date() < m_start->date())
        return tr("La data di fine è precedente alla data di inizio.");
    return QString();
}

RecurringRule RecurringForm::rule() const
{
    RecurringRule r;
    r.id = m_ruleId;
    r.accountId = m_accountId;
    r.categoryId = m_category->currentData().toInt();
    r.amount = qRound64(m_amount->value() * 100.0);
    r.isIncome = m_type->currentData().toBool();
    r.note = m_note->text().trimmed();
    r.frequency = m_unit->currentData().toString();
    r.interval = qRound(m_interval->value());
    r.startDate = m_start->date().toString(Qt::ISODate);
    r.nextDate = r.startDate;
    r.endDate = m_endMode->currentData().toBool() ? m_end->date().toString(Qt::ISODate) : QString();
    return r;
}
