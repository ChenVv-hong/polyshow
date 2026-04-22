#pragma once

#include <QWidget>

class QListWidget;

namespace PolyShow
{

/// Supported log message severities.
enum class LogSeverity
{
    Info,
    Warning,
    Error
};

/// Displays a compact single-tab log panel.
class LogPanel final : public QWidget
{
    Q_OBJECT

public:
    /// Creates the log panel widget.
    explicit LogPanel(QWidget *parent = nullptr);

    /// Appends one log row to the panel.
    void appendMessage(LogSeverity severity, const QString &message);

    /// Clears all stored log rows.
    void clearMessages();

private:
    QListWidget *m_list_widget {nullptr};
};

} // namespace PolyShow
