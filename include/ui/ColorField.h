#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace PolyShow
{

/// Displays a clickable color swatch plus editable hex text.
class ColorField final : public QWidget
{
    Q_OBJECT

public:
    /// Creates the color field widget.
    explicit ColorField(QWidget *parent = nullptr);

    /// Replaces the current color text.
    void setColorText(const QString &colorText);

    /// Returns the current color text.
    [[nodiscard]]
    QString colorText() const;

    /// Replaces the placeholder text for the input field.
    void setPlaceholderText(const QString &text);

    /// Shows or clears the current validation error.
    void setErrorMessage(const QString &message);

signals:
    /// Emitted when the user commits the current text value.
    void colorTextCommitted(const QString &colorText);

private:
    /// Commits the current text value to listeners.
    void commitCurrentText();

    /// Opens the Qt color picker and commits the chosen value.
    void openColorDialog();

    /// Updates the swatch fill from the current text.
    void updateSwatch();

    QPushButton *m_swatch_button {nullptr};
    QLineEdit *m_line_edit {nullptr};
    QLabel *m_error_label {nullptr};
    bool m_is_loading {false};
};

} // namespace PolyShow
