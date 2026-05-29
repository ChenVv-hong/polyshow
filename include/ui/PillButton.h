#pragma once

#include <QPushButton>
#include <QSize>
#include <QString>

class QEnterEvent;
class QEvent;
class QHBoxLayout;
class QLabel;

namespace PolyShow
{

class MaterialIconLabel;

/// Reusable rounded button used by compact toolbars.
class PillButton final : public QPushButton
{
    Q_OBJECT

public:
    /// Visual variants used by the shared toolbar buttons.
    enum class Variant
    {
        Neutral,
        Primary,
        Success
    };

    /// Creates one pill button with the shared control styling.
    explicit PillButton(const QString &text = QString(), QWidget *parent = nullptr);

    /// Applies one visual variant to the button.
    void setVariant(Variant variant);

    /// Adds or replaces the Material Symbols Rounded icon shown in the button.
    void setIconName(const QString &iconName);

    /// Replaces the visible button text while keeping native button painting text-free.
    void setText(const QString &text);

    [[nodiscard]]
    QSize sizeHint() const override;

    [[nodiscard]]
    QSize minimumSizeHint() const override;

protected:
    void changeEvent(QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    /// Maps the enum to the stylesheet property string.
    static QString variantName(Variant variant);

    void refreshContentLayout();
    void refreshContentStyle();

    QHBoxLayout *m_content_layout {nullptr};
    MaterialIconLabel *m_icon_label {nullptr};
    QLabel *m_text_label {nullptr};
    QString m_display_text;
};

} // namespace PolyShow
