#pragma once

#include "core/PrimitiveEditing.h"
#include "core/GeometryTypes.h"

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QWidget;

namespace PolyShow
{

class ColorField;

/// Displays selection details and field-level primitive editing controls.
class InspectorPanel final : public QWidget
{
    Q_OBJECT

public:
    /// Creates the inspector panel widget.
    explicit InspectorPanel(QWidget *parent = nullptr);

    /// Rebuilds the inspector from the current selection context.
    void loadSelectionContext(const DocumentData &documentData, const SelectionState &selectionState);

    /// Replaces the validation error for one style field.
    void setStyleFieldError(PrimitiveStyleField field, const QString &message);

    /// Clears the validation error for one style field.
    void clearStyleFieldError(PrimitiveStyleField field);

    /// Replaces the validation error for the coordinates editor.
    void setCoordinateError(const QString &message);

    /// Clears the validation error for the coordinates editor.
    void clearCoordinateError();

signals:
    /// Emitted when the user commits one style field.
    void styleChangeRequested(const PrimitiveStyleChangeRequest &request);

    /// Emitted when the user edits the coordinates text.
    void coordinateDraftChanged(const PrimitiveCoordinateDraft &draft);

private:
    /// Rebuilds the visible summary plus editor from the stored selection context.
    void updateContent();

    /// Loads the editor controls from one selected primitive.
    void loadPrimitiveEditor(const LayerData &layer, int primitiveIndex);

    /// Synchronizes one text field validation state.
    void setTextFieldError(QWidget *editor, QLabel *errorLabel, const QString &message);

    /// Applies the stored validation errors to every visible widget.
    void updateFieldErrors();

    /// Switches the editor-only controls for the current primitive kind.
    void updateVisibleEditorFields(PrimitiveKind kind);

    /// Returns whether a valid primitive selection is active.
    [[nodiscard]]
    bool hasActivePrimitiveSelection() const;

    /// Returns the currently selected primitive kind.
    [[nodiscard]]
    PrimitiveKind currentPrimitiveKind() const;

    /// Builds one style change request for the current selection.
    [[nodiscard]]
    PrimitiveStyleChangeRequest buildStyleChangeRequest(PrimitiveStyleField field) const;

    /// Builds the current coordinate draft for the selected primitive.
    [[nodiscard]]
    PrimitiveCoordinateDraft currentCoordinateDraft() const;

    DocumentData m_document_data;
    SelectionState m_selection_state;
    PrimitiveEditValidationErrors m_validation_errors;
    bool m_is_loading_form {false};
    QLabel *m_badge_label {nullptr};
    QLabel *m_title_label {nullptr};
    QLabel *m_meta_label {nullptr};
    QLabel *m_geometry_label {nullptr};
    QLabel *m_geometry_body_label {nullptr};
    QLabel *m_hint_label {nullptr};
    QLabel *m_editor_help_label {nullptr};
    QWidget *m_editor_widget {nullptr};
    QWidget *m_fill_section_widget {nullptr};
    QWidget *m_width_section_widget {nullptr};
    ColorField *m_stroke_color_field {nullptr};
    ColorField *m_fill_color_field {nullptr};
    QCheckBox *m_fill_enabled_check_box {nullptr};
    QLineEdit *m_width_line_edit {nullptr};
    QLineEdit *m_point_size_line_edit {nullptr};
    QLabel *m_width_error_label {nullptr};
    QLabel *m_point_size_error_label {nullptr};
    QPlainTextEdit *m_coordinates_text_edit {nullptr};
    QLabel *m_coordinates_error_label {nullptr};
};

} // namespace PolyShow
