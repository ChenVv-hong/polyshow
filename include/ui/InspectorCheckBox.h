#pragma once

#include <QCheckBox>

namespace PolyShow
{

/// Inspector checkbox painted to match the outliner visibility control.
class InspectorCheckBox final : public QCheckBox
{
    Q_OBJECT

public:
    /// Creates one inspector checkbox with shared outliner visual styling.
    explicit InspectorCheckBox(const QString &text, QWidget *parent = nullptr);

    /// Returns the compact control size used by inspector field rows.
    [[nodiscard]]
    QSize sizeHint() const override;

    /// Returns the minimum compact control size used by inspector field rows.
    [[nodiscard]]
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace PolyShow
