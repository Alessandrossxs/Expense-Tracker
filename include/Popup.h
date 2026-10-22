#pragma once

#include <QWidget>
#include <functional>

class QEventLoop;
class QFrame;
class QLabel;
class QMainWindow;
class QPushButton;

// Modal card drawn *inside* the main window over a dimmed scrim, used instead of
// separate dialog windows. exec() blocks like QDialog::exec() but never opens a
// new native window.
class Popup : public QWidget
{
public:
    // Returns an error message to keep the popup open, or an empty string to accept.
    using Validator = std::function<QString()>;

    Popup(QMainWindow *window, const QString &title, QWidget *content, const QString &okText,
          const QString &cancelText, bool danger);

    bool exec();

    static bool isOpen();
    static bool run(QMainWindow *window, const QString &title, QWidget *content,
                    const QString &okText, const Validator &validator = Validator(),
                    bool danger = false);
    static void message(QMainWindow *window, const QString &title, const QString &text);
    static bool confirm(QMainWindow *window, const QString &title, const QString &text,
                        const QString &okText, bool danger = false);
    // Label above the input widget, as used by every form.
    static QWidget *field(const QString &label, QWidget *input, QWidget *parent = nullptr);

    void setValidator(const Validator &validator) { m_validator = validator; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void tryAccept();
    void finish(bool accepted);

    QMainWindow *m_window;
    QWidget *m_host;
    QWidget *m_content;
    QLabel *m_error;
    QPushButton *m_ok;
    Validator m_validator;
    QEventLoop *m_loop = nullptr;
    bool m_accepted = false;
};
