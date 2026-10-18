#include "Popup.h"

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QEvent>
#include <QEventLoop>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
int g_openPopups = 0;
}

Popup::Popup(QMainWindow *window, const QString &title, QWidget *content, const QString &okText,
             const QString &cancelText, bool danger)
    : QWidget(window), m_window(window), m_host(window->centralWidget()), m_content(content)
{
    setObjectName(QStringLiteral("popupScrim"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);
    window->installEventFilter(this);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(30, 30, 30, 30);
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("popupCard"));
    card->setFixedWidth(460);
    outer->addWidget(card, 0, Qt::AlignCenter);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(16);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("popupTitle"));
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);

    content->setParent(card);
    content->setObjectName(QStringLiteral("transparentBox"));
    layout->addWidget(content);

    m_error = new QLabel(card);
    m_error->setObjectName(QStringLiteral("popupError"));
    m_error->setWordWrap(true);
    m_error->hide();
    layout->addWidget(m_error);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();
    if (!cancelText.isEmpty()) {
        auto *cancel = new QPushButton(cancelText, card);
        cancel->setObjectName(QStringLiteral("secondaryButton"));
        cancel->setCursor(Qt::PointingHandCursor);
        cancel->setAutoDefault(false);
        connect(cancel, &QPushButton::clicked, this, [this] { finish(false); });
        buttons->addWidget(cancel);
    }
    m_ok = new QPushButton(okText, card);
    m_ok->setObjectName(danger ? QStringLiteral("dangerButton") : QStringLiteral("primaryButton"));
    m_ok->setCursor(Qt::PointingHandCursor);
    m_ok->setAutoDefault(false);
    connect(m_ok, &QPushButton::clicked, this, [this] { tryAccept(); });
    buttons->addWidget(m_ok);
    layout->addLayout(buttons);
}

bool Popup::isOpen() { return g_openPopups > 0; }

bool Popup::exec()
{
    ++g_openPopups;
    if (m_host)
        m_host->setEnabled(false); // keeps keyboard focus inside the popup
    setGeometry(m_window->rect());
    show();
    raise();

    QWidget *first = nullptr;
    for (QWidget *w : m_content->findChildren<QWidget *>()) {
        if ((qobject_cast<QLineEdit *>(w) && !w->parentWidget()->inherits("QAbstractSpinBox") &&
             !w->parentWidget()->inherits("QComboBox")) ||
            qobject_cast<QAbstractSpinBox *>(w) || qobject_cast<QComboBox *>(w)) {
            first = w;
            break;
        }
    }
    (first ? first : static_cast<QWidget *>(m_ok))->setFocus();

    QEventLoop loop;
    m_loop = &loop;
    loop.exec();
    m_loop = nullptr;

    m_window->removeEventFilter(this);
    if (m_host)
        m_host->setEnabled(true);
    --g_openPopups;
    hide();
    deleteLater(); // content stays readable by the caller until control returns to the main loop
    return m_accepted;
}

void Popup::tryAccept()
{
    if (m_validator) {
        const QString error = m_validator();
        if (!error.isEmpty()) {
            m_error->setText(error);
            m_error->show();
            return;
        }
    }
    finish(true);
}

void Popup::finish(bool accepted)
{
    m_accepted = accepted;
    if (m_loop)
        m_loop->quit();
}

void Popup::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        finish(false);
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        tryAccept();
    else
        QWidget::keyPressEvent(event);
}

bool Popup::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window && event->type() == QEvent::Resize)
        setGeometry(m_window->rect());
    return QWidget::eventFilter(watched, event);
}

bool Popup::run(QMainWindow *window, const QString &title, QWidget *content, const QString &okText,
                const Validator &validator, bool danger)
{
    auto *popup = new Popup(window, title, content, okText, tr("Annulla"), danger);
    popup->setValidator(validator);
    return popup->exec();
}

void Popup::message(QMainWindow *window, const QString &title, const QString &text)
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("popupText"));
    label->setWordWrap(true);
    auto *popup = new Popup(window, title, label, tr("OK"), QString(), false);
    popup->exec();
}

bool Popup::confirm(QMainWindow *window, const QString &title, const QString &text,
                    const QString &okText, bool danger)
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("popupText"));
    label->setWordWrap(true);
    auto *popup = new Popup(window, title, label, okText, tr("Annulla"), danger);
    return popup->exec();
}

QWidget *Popup::field(const QString &label, QWidget *input, QWidget *parent)
{
    auto *box = new QWidget(parent);
    box->setObjectName(QStringLiteral("transparentBox"));
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    auto *caption = new QLabel(label, box);
    caption->setObjectName(QStringLiteral("fieldLabel"));
    layout->addWidget(caption);
    input->setParent(box);
    layout->addWidget(input);
    return box;
}
