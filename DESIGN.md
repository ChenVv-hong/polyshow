# PolyShow Design Guide

## Purpose

This document defines the UI direction for PolyShow and the design constraints that should stay aligned with
`designs/ui-prototype/polyshow.pen`.

The current direction is V2: a Blender-inspired dark spatial editor style that is dense, task-focused, panelized, and
centered on a dominant geometry viewport. PolyShow should borrow Blender's product logic, not its branding.

The previous prototype is preserved as `designs/ui-prototype/polyshow-v1.pen`. The current source of truth is
`designs/ui-prototype/polyshow.pen`.

## References

Primary references used for this guide:

- Blender Manual: Workspaces
  https://docs.blender.org/manual/en/3.0/interface/window_system/workspaces.html
- Blender Manual: Areas
  https://docs.blender.org/manual/en/latest/interface/window_system/areas.html
- Blender Manual: Regions
  https://docs.blender.org/manual/en/latest/interface/window_system/regions.html
- Blender Manual: Tabs & Panels
  https://docs.blender.org/manual/en/latest/interface/window_system/tabs_panels.html
- Blender Manual: Status Bar
  https://docs.blender.org/manual/en/3.0/interface/window_system/status_bar.html
- Blender Developer Documentation: Layouts
  https://developer.blender.org/docs/features/interface/human_interface_guidelines/layouts/
- Blender Developer Documentation: Icons
  https://developer.blender.org/docs/features/interface/human_interface_guidelines/icons/

## Blender Style Takeaways

Blender's UI is organized as a spatial editor system:

- Workspaces are task-oriented layouts.
- Areas reserve screen space for editors.
- Editors have a dominant main region plus secondary regions such as headers, toolbars, sidebars, and panels.
- Panels are the smallest reusable organization unit and can collapse when detail is not needed.
- The status bar carries contextual shortcuts, messages, and statistics.
- Important and frequent actions stay visible; less common controls move lower or into panels.
- Icon-only controls require tooltips and must not reuse the same icon for different meanings.

## PolyShow Layout Model

PolyShow uses the same spatial hierarchy in a simpler 2D geometry context:

1. Window
2. Workspace root
3. Areas
4. Regions
5. Panels
6. Controls

The main window remains a desktop editor, not a web dashboard or marketing surface.

### Areas

PolyShow has these primary areas:

- Top menu bar: global application commands.
- Left structure area: layer and primitive navigation.
- Center editor area: geometry viewport and viewport-local tools.
- Right properties area: contextual inspector, hidden when nothing is selected.
- Bottom output area: log panel.
- Bottom status bar: transient messages, counts, and cursor position.

### Dominant Region

The geometry viewport is always the dominant visual region.

Rules:

- The viewport must receive the largest continuous space.
- Side panels should support the viewport, not compete with it.
- Main editor areas should touch through shared 1px borders, like Blender editor areas. Do not create card-like gutters between the outliner, viewport, inspector, and log.
- Viewport controls live inside the viewport panel because they operate on the editor region.
- Empty state should enlarge the viewport by hiding the inspector.

## Prototype Versioning

- `designs/ui-prototype/polyshow-v1.pen` is the V1 legacy reference. Do not edit it for new design work.
- `designs/ui-prototype/polyshow.pen` is the V2 current design source of truth.
- V2 should not keep old pages as hidden leftovers inside the active `.pen`; use the separate V1 file for comparison.

## Current Prototype Screens

The prototype currently includes these product states:

- `PolyShow V2 - Primitive Selected`
- `PolyShow V2 - Layer Selected`
- `PolyShow V2 - Empty State`
- `PolyShow V2 - Search Open`
- `PolyShow V2 - Menus and Dialogs`
- `PolyShow V2 - Drawing IPC and Theme States`

These are not separate styles. They are states of one editor system.

## Navigation and Structure

The left sidebar is PolyShow's equivalent of a simplified Outliner.

Rules:

- A file-backed import appears as a layer row.
- Internal layers and IPC layers use the same row structure with a type suffix.
- Primitive rows are direct children of their owning layer.
- Layer and primitive visibility are represented with checkboxes.
- A layer checkbox may represent mixed visibility when child primitives differ.
- Search is collapsed by default and expands only when requested.
- Search results should preserve hierarchy where possible.

## Viewport

The viewport is PolyShow's primary editor region.

It should include:

- Grid and axis rendering.
- Geometry primitives.
- Selection bounds.
- Drawing preview.
- Interaction hint overlay.
- Scale bar overlay.
- Viewport-local controls for browse/draw modes, fit, zoom, reset, grid, and render mode.

Rules:

- Do not move viewport-local operations into global navigation.
- Keep tool controls compact and scan-friendly.
- Use segmented or pill controls for mutually exclusive tools.
- Use a combo box for render mode because it has three options and can grow later.
- Show contextual feedback in the status bar and log panel, not as large decorative banners.

## Inspector

The right inspector is contextual and selection-driven.

States:

- No selection: hidden.
- Layer selection: show layer type, summary, primitive counts, and source meaning.
- Primitive selection: show primitive identity, geometry summary, style fields, and coordinates.

Rules:

- Group related fields as one inspector section container with a distinct header background and a distinct content background.
- Labels such as `Geometry`, `Style`, and `Coordinates` should sit in the section header area, while the fields sit in the content area beneath it.
- The header and content backgrounds may differ, but they must remain visually attached as one section.
- In the V2 prototype, the inspector sidebar surface uses `#303236`, section headers use `#282A2E`, and section content areas use `#26282E`.
- Draw the subtle `#45484F` border around the whole section container so it wraps both the header and the content together.
- Keep inspector content indentation very compact. Rows and field groups should read as Blender-style controls, not as nested subpanels.
- Keep editable fields in predictable order: identity, geometry, style, coordinates, hints.
- Validation errors stay near the field that caused them.
- Invalid coordinates should visibly mark the text edit and suppress misleading previews.
- Avoid isolated section titles that look detached from their content.
- Avoid making inspector group headers look like detached floating labels.
- Avoid nested decorative cards; use functional panel boundaries only.

## Menus and Dialogs

Menus are global command surfaces.

Current menu model:

- File: New Layer, Open, Export Active Layer, Exit.
- View: Fit to View, Zoom In, Zoom Out, Reset View.
- Render: Solid, Wireframe, Points.
- IPC: Start IPC Listener, Stop IPC Listener.
- Help: About.

Dialogs should remain plain Qt-style utility dialogs:

- New Layer: name input, type selector, validation text, OK/Cancel.
- About: short feature summary.
- Export/Open failures: concise messages and direct recovery path.

## Status and Logs

The status bar should follow Blender's status-bar pattern:

- Left: current transient message.
- Right: scene counts and cursor coordinates.

The log panel is the persistent record:

- `info`: normal successful operations.
- `warning`: recoverable or surprising behavior.
- `error`: failed user action, parser failure, IPC rejection, or validation failure.

Do not split the log into multiple tabs until the product has enough output volume to justify it.

## Visual Style

PolyShow should feel like a compact technical editor. V2 is dark-first.

### V2 Prototype Dark Theme

Current V2 prototype tokens:

- Window background: `#202124`
- Top bar: `#2C2D31`
- Panel: `#303236`
- Secondary panel: `#282A2E`
- Viewport/canvas: `#1F2228`
- Border: `#45484F`
- Text: `#E9EAEC`
- Muted text: `#AEB4BC`
- Primary accent: `#63A7FF`
- Primary selected background: `#274763`
- Success accent: `#6EC987`
- Warning accent: `#E7BF67`
- Error accent: `#ED8A8A`

### Runtime Light Theme

Current Qt tokens:

- Window background: `#F5F5F5`
- Panel: `#FFFFFF`
- Card: `#FAFAFA`
- Border: `#D3D3D3`
- Button: `#EFEFEF`
- Text: `#202020`
- Muted text: `#6E6E6E`
- Primary accent: `#2D74FF`
- Primary selected background: `#E4EEFF`
- Success accent: `#39A36E`
- Error accent: `#C45A5A`

### Runtime Dark Theme

Current Qt tokens:

- Window background: `#2B2B2B`
- Panel: `#353535`
- Card: `#303030`
- Border: `#494949`
- Button: `#3A3A3A`
- Text: `#E4E4E4`
- Muted text: `#A0A0A0`
- Primary accent: `#4E8CFF`
- Primary selected background: `#254A73`
- Success accent: `#5FBF8F`
- Error accent: `#C76969`

## Typography

Use Qt's current UI typography:

- `Segoe UI` for ordinary UI text.
- `Consolas` or `IBM Plex Mono` for logs, coordinates, paths, and structured numeric data.

Use smaller text in panels. Reserve large type for screen titles in design boards only, not in the application UI.

## Icon System

PolyShow uses Google Material Symbols Rounded as the default icon language.

Rules:

- In Pencil, use `icon_font` with `iconFontFamily: "Material Symbols Rounded"` before considering any custom asset.
- In Qt UI work, prefer the Google Material Symbols Rounded icon font for menus, toolbars, buttons, panels, and status UI.
- In Qt implementation, render Material Symbols Rounded as ligature text in styled widgets whenever practical. QSS owns the icon font family, weight, color, and state rules; C++ may only set the ligature name, fixed bounds, and size/state properties for those widgets.
- Use `MaterialIcon::icon()` only for Qt APIs that require a native `QIcon`, such as `QAction`, `QComboBox` items, or third-party/native widgets. It must render from a sufficiently large default size and include high-DPI pixmaps so icons are not blurred by upscaling.
- Use text labels with icons for important commands; icon-only controls require tooltips.
- Do not mix Material Symbols Rounded with other icon families unless a deliberate migration plan exists.
- Use custom SVG only when Material Symbols Rounded cannot express the required shape or when the icon is a product-specific diagram.

### SVG Exception Workflow

When a custom SVG icon is genuinely required:

1. Use `$svg-precision-skill`.
2. Define fixed dimensions, explicit `viewBox`, and deterministic geometry.
3. Build and validate the SVG with the skill tooling.
4. Place the generated SVG under `resources/icons/`.
5. If the program needs the icon, add it to the relevant Qt `.qrc` file in the implementation phase.
6. If the design needs the icon, reference the same asset in the Pencil design rather than recreating a separate version.

## Component Rules

- When the Qt implementation is being matched to the HTML reference in `html/`, use the HTML files as the visual contract for shell layout, spacing, color tokens, panel states, and icon choices. Preserve the existing Qt runtime logic unless the task explicitly asks for behavior changes.
- Keep style declarations in `resources/style/darkstyle.qss` and `resources/style/lightstyle.qss`; new C++ widgets should expose object names, properties, and reusable structure for QSS to target instead of carrying inline stylesheet strings.
- QSS selectors should prefer stable `objectName` selectors for app-specific surfaces and controls. Type selectors are acceptable for shared Qt primitives only when they intentionally apply across the app.
- Repeated HTML structures should become reusable widgets such as panel headers, icon controls, or inspector sections before being reused in multiple panels.
- `PanelFrame` is the standard panel, card, and canvas container.
- `PillButton` is the standard viewport/tool action button.
- `ColorField` is the standard color editor pattern.
- Reusable UI behavior belongs in a reusable widget, not as one-off `MainWindow` code.
- Prefer Material Symbols Rounded icons for component affordances.
- Use checkboxes for binary visibility.
- Use combo boxes for option sets with three or more values.
- Use compact labels and tooltips for dense tool controls.

## Anti-Patterns

Avoid:

- Marketing-style hero sections.
- Large decorative cards around every element.
- Floating visual ornaments that do not support editing.
- Multiple equal-weight focal regions.
- Duplicating the same command in unrelated locations.
- Icon-only controls without tooltips.
- Full-width warning banners for routine validation.
- UI text that explains the UI instead of showing state.

## Change Control

When UI behavior changes:

1. Update the Qt implementation.
2. Update `designs/ui-prototype/polyshow.pen`.
3. Update this guide if the layout model, style, or state behavior changed.
4. Keep `AGENTS.md` aligned with any implementation rules that coding agents must follow.
5. If custom SVG icons were added, validate that the same asset path is usable by both the program and the design.
