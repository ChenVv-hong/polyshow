#pragma once

#include "core/PrimitiveEditing.h"
#include "core/GeometryTypes.h"

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QWidget;

namespace PolyShow
{

/// Displays selection details and editable primitive controls.
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

    /// Shows field-level validation errors on the editor controls.
    void setValidationErrors(const PrimitiveEditValidationErrors &errors);

    /// Clears any active validation errors from the editor controls.
    void clearValidationErrors();

signals:
    /// Emitted when the user applies the current primitive edits.
    void applyRequested(const PrimitiveEditRequest &request);

    /// Emitted when the user resets the current primitive edits.
    void resetRequested();

private:
    /// Rebuilds all visible detail strings from the stored state.
    void updateContent();

    /// Loads the editor controls from one selected primitive.
    void loadPrimitiveEditor(const LayerData &layer, int primitiveIndex);

    /// Reloads the editor from the stored current selection.
    void reloadCurrentPrimitiveEditor();

    /// Returns the current editor values as one submission request.
    [[nodiscard]]
    PrimitiveEditRequest currentEditRequest() const;

    /// Updates button state and unsaved-change feedback.
    void updateEditorState();

    /// Applies one validation error state to a field and its hint label.
    void setFieldError(QWidget *editor, QLabel *errorLabel, const QString &message);

    /// Switches the editor-only controls for the current primitive kind.
    void updateVisibleEditorFields(PrimitiveKind kind);

    DocumentData m_document_data;
    SelectionState m_selection_state;
    PrimitiveEditRequest m_loaded_request;
    bool m_has_loaded_request {false};
    bool m_is_loading_form {false};
    QLabel *m_badge_label {nullptr};
    QLabel *m_title_label {nullptr};
    QLabel *m_meta_label {nullptr};
    QLabel *m_geometry_label {nullptr};
    QLabel *m_geometry_body_label {nullptr};
    QLabel *m_hint_label {nullptr};
    QWidget *m_editor_widget {nullptr};
    QWidget *m_fill_section_widget {nullptr};
    QWidget *m_width_section_widget {nullptr};
    QLabel *m_edit_state_label {nullptr};
    QLabel *m_stroke_color_error_label {nullptr};
    QLabel *m_fill_color_error_label {nullptr};
    QLabel *m_width_error_label {nullptr};
    QLabel *m_point_size_error_label {nullptr};
    QLabel *m_coordinates_error_label {nullptr};
    QLineEdit *m_stroke_color_line_edit {nullptr};
    QCheckBox *m_fill_enabled_check_box {nullptr};
    QLineEdit *m_fill_color_line_edit {nullptr};
    QLineEdit *m_width_line_edit {nullptr};
    QLineEdit *m_point_size_line_edit {nullptr};
    QPlainTextEdit *m_coordinates_text_edit {nullptr};
    QPushButton *m_apply_button {nullptr};
    QPushButton *m_reset_button {nullptr};
};

} // namespace PolyShow
