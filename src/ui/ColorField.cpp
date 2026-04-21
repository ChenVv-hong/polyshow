#include "ui/ColorField.h"

#include "core/PrimitiveEditing.h"

#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QStyle>
#include <QVBoxLayout>

namespace PolyShow
{

namespace
{

/// Small push button that paints a color preview swatch.
class ColorSwatchButton final : public QPushButton
{
public:
    explicit ColorSwatchButton(QWidget *parent = nullptr)
        : QPushButton(parent)
    {
    }

    void setDisplayColor(const QColor &color, bool hasValidColor)
    {
        m_color = color;
        m_has_valid_color = hasValidColor;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPushButton::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRect fillRect = rect().adjusted(4, 4, -4, -4);
        const QColor fillColor = m_has_valid_color ? m_color : palette().color(QPalette::Button);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fillColor);
        painter.drawRoundedRect(fillRect, 4.0, 4.0);
    }

private:
    QColor m_color;
    bool m_has_valid_color {false};
};

/// Refreshes one widget after a validation property change.
void refreshValidationStyle(QWidget *widget)
{
    if (widget == nullptr)
    {
        return;
    }

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace

ColorField::ColorField(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto *rowLayout = new QHBoxLayout();
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    m_swatch_button = new ColorSwatchButton(this);
    m_swatch_button->setProperty("colorSwatch", true);
    m_swatch_button->setCursor(Qt::PointingHandCursor);
    m_swatch_button->setToolTip(QStringLiteral("Pick a color"));
    rowLayout->addWidget(m_swatch_button);

    m_line_edit = new QLineEdit(this);
    rowLayout->addWidget(m_line_edit, 1);

    layout->addLayout(rowLayout);

    m_error_label = new QLabel(this);
    m_error_label->setProperty("role", QStringLiteral("validationError"));
    m_error_label->setWordWrap(true);
    m_error_label->setVisible(false);
    layout->addWidget(m_error_label);

    connect(m_swatch_button, &QPushButton::clicked, this, [this]() {
        openColorDialog();
    });
    connect(m_line_edit, &QLineEdit::textChanged, this, [this](const QString &) {
        updateSwatch();
    });
    connect(m_line_edit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_is_loading)
        {
            commitCurrentText();
        }
    });

    updateSwatch();
}

void ColorField::setColorText(const QString &colorText)
{
    m_is_loading = true;
    m_line_edit->setText(colorText);
    m_is_loading = false;
    updateSwatch();
}

QString ColorField::colorText() const
{
    return m_line_edit->text().trimmed();
}

void ColorField::setPlaceholderText(const QString &text)
{
    m_line_edit->setPlaceholderText(text);
}

void ColorField::setErrorMessage(const QString &message)
{
    const QString validationState = message.isEmpty() ? QString() : QStringLiteral("error");
    m_line_edit->setProperty("validationState", validationState);
    m_swatch_button->setProperty("validationState", validationState);
    refreshValidationStyle(m_line_edit);
    refreshValidationStyle(m_swatch_button);

    m_error_label->setText(message);
    m_error_label->setVisible(!message.isEmpty());
}

void ColorField::commitCurrentText()
{
    emit colorTextCommitted(colorText());
}

void ColorField::openColorDialog()
{
    QColor initialColor;
    if (!parseColorText(colorText(), initialColor))
    {
        initialColor = QColor(Qt::white);
    }

    QColorDialog dialog(initialColor, this);
    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    setColorText(formatColorText(dialog.currentColor()));
    commitCurrentText();
}

void ColorField::updateSwatch()
{
    QColor previewColor;
    const bool hasValidColor = parseColorText(colorText(), previewColor);
    static_cast<ColorSwatchButton *>(m_swatch_button)->setDisplayColor(previewColor, hasValidColor);
}

} // namespace PolyShow
