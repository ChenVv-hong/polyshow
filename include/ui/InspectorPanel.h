#pragma once

#include "core/GeometryTypes.h"

#include <QWidget>

class QLabel;

namespace PolyShow
{

/// Displays read-only details for the current layer or primitive selection.
class InspectorPanel final : public QWidget
{
    Q_OBJECT

public:
    /// Creates the inspector panel widget.
    explicit InspectorPanel(QWidget *parent = nullptr);

    /// Replaces the document used for the displayed details.
    void setDocumentData(const DocumentData &documentData);

    /// Replaces the active selection and refreshes the panel.
    void setSelectionState(const SelectionState &selectionState);

private:
    /// Rebuilds all visible detail strings from the stored state.
    void updateContent();

    DocumentData m_document_data;
    SelectionState m_selection_state;
    QLabel *m_badge_label {nullptr};
    QLabel *m_title_label {nullptr};
    QLabel *m_meta_label {nullptr};
    QLabel *m_geometry_label {nullptr};
    QLabel *m_geometry_body_label {nullptr};
    QLabel *m_coordinates_label {nullptr};
    QLabel *m_coordinates_body_label {nullptr};
    QLabel *m_style_label {nullptr};
    QLabel *m_style_body_label {nullptr};
    QLabel *m_hint_label {nullptr};
};

} // namespace PolyShow
