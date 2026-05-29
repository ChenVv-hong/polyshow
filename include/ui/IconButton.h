#pragma once

#include <QPushButton>
#include <QSize>
#include <QString>

class QHBoxLayout;
class QLabel;
class QEnterEvent;
class QEvent;

namespace PolyShow
{

class MaterialIconLabel;

/// Compact push button that uses Material Symbols Rounded for its icon.
class IconButton final : public QPushButton
{
    Q_OBJECT

public:
    /// Creates an icon-only or icon-plus-text button.
    explicit IconButton(const QString &iconName, const QString &text = QString(), QWidget *parent = nullptr);

    /// Replaces the displayed icon.
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
    void refreshContentLayout();
    void refreshContentStyle();

    QHBoxLayout *m_content_layout {nullptr};
    MaterialIconLabel *m_icon_label {nullptr};
    QLabel *m_text_label {nullptr};
    QString m_display_text;
};

} // namespace PolyShow
