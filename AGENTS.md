# AGENTS.md

Guidance for coding agents working in the PolyShow repository.

## Communication

- Address the user as `dady`.
- Prefer Chinese when the user writes in Chinese.
- Confirm behavior from the repository before making assumptions.
- Keep changes minimal, practical, and easy to review.
- Preserve user changes and unrelated worktree changes.

## Project Snapshot

- `polyshow` is a Qt 6 Widgets desktop application written in C++17.
- The app renders PolyShow's custom 2D `.ply` text format. It is not the Stanford PLY format.
- `CMakeLists.txt` is the build source of truth.
- `CODE_STYLE.md` is the coding-style source of truth when older notes conflict with current code.
- `DESIGN.md` is the UI design source of truth for layout, style, and prototype intent.
- `designs/ui-prototype/polyshow.pen` is the current V2 editable UI prototype.
- `designs/ui-prototype/polyshow-v1.pen` is the preserved V1 legacy prototype and should not be edited for new work.

## Build Commands

### Windows

```bash
cmake -G "Visual Studio 17 2022" -B build -DCMAKE_PREFIX_PATH="D:/Qt/6.9.1/msvc2022_64"
cmake --build build --config Release
cmake --build build --config Debug
```

Typical binaries:

- `build/Release/polyshow.exe`
- `build/Debug/polyshow.exe`

### Linux

```bash
cmake -G "Unix Makefiles" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Verification

- There is currently no dedicated test target registered in `CMakeLists.txt`.
- Do not assume `ctest` covers the project unless test targets are added first.
- For normal verification:
  1. Build the project.
  2. Launch the application.
  3. Load representative files from `test_data/ply`.
- Good smoke-test inputs include:
  - Valid: `triangle.ply`, `multi_shape.ply`, `style_colors.ply`, `fill_none.ply`
  - Invalid: `invalid_bad_number.ply`, `invalid_mid_shape_style.ply`, `invalid_unknown_command.ply`
- If you change parsing, rendering, IPC writes, or UI state sync, verify at least one valid and one invalid sample.

## Repository Conventions

- File names use PascalCase, for example `GeometryScene.h`, `MainWindow.cpp`.
- Module directories stay lowercase, for example `core`, `parsers`, `ui`.
- Classes, structs, enums, and type aliases use PascalCase.
- Functions, methods, local variables, and parameters use camelCase.
- Private data members use `m_` + lower_snake_case.
- Constants prefer `kPascalCase`.
- Namespace name is `PolyShow`.
- Prefer `final`, `override`, and `[[nodiscard]]` where appropriate.
- Use `QStringLiteral()` for Qt string literals.
- Prefer Qt ownership patterns and RAII over manual lifetime management.

## Formatting and Includes

- Use 4 spaces for indentation.
- Keep lines at or under 120 characters when practical.
- Use Allman braces for functions, classes, and namespaces.
- Use K&R braces for control flow blocks.
- Use `Type *name` and `Type &name`.
- In `.cpp` files, order includes as:
  1. Matching header
  2. Project headers
  3. Qt headers
  4. Standard library headers
  5. Third-party headers
- There is no repository `.clang-format` file at the moment, so avoid broad formatting-only churn.

## Comments and Documentation

- Use `///` for API comments in headers.
- Use short `//` comments for non-obvious logic in source files.
- Comment intent, constraints, or invariants instead of restating code line by line.
- TODO, FIXME, and HACK comments must include an owner and context.
- If behavior is defined by code and contradicted by outdated docs, trust the code and update the docs.

## UI Design Direction

PolyShow follows a Blender-inspired dark spatial editor style documented in `DESIGN.md`.

Important principles:

- Treat the center geometry viewport as the dominant editor region.
- Keep global commands in the menu bar.
- Keep viewport-specific controls inside the viewport panel.
- Use the left sidebar as a compact layer/primitive structure tree.
- Use the right inspector only for contextual selection details.
- Hide the inspector when nothing is selected.
- Use the bottom log for persistent operation history.
- Use the status bar for transient messages, counts, and cursor coordinates.
- Treat `polyshow.pen` as V2 current design and `polyshow-v1.pen` as historical reference only.

## UI Implementation Rules

- Use existing project UI components before creating new widget patterns.
- `PanelFrame` is the standard panel/card/canvas container.
- `PillButton` is the standard compact tool/action button.
- `ColorField` is the standard color input pattern.
- Keep colors centralized through light/dark style resources and `RenderTheme`.
- Support both light and dark theme tokens when adding styled widgets.
- Prefer Google Material Symbols Rounded for UI icons in both design and program implementation.
- Use checkboxes for binary visibility and combo boxes for option sets with three or more values.
- Icon-only controls must have a tooltip.
- Do not add decorative UI that does not help navigation, inspection, editing, or feedback.
- Avoid nested card-heavy layouts. One structural panel plus one inner content card is usually enough.

## SVG Icon Workflow

- Use SVG icons only when Material Symbols Rounded cannot express the required symbol.
- Custom SVG icons must be generated with `$svg-precision-skill`.
- SVG specs must use explicit dimensions, explicit `viewBox`, and deterministic geometry.
- Generated SVG files belong under `resources/icons/`.
- Validate generated SVGs with the skill tooling before using them in design or code.
- If program code needs the SVG, add it to the appropriate Qt resource `.qrc` file in the same implementation change.

## UI Prototype Rules

- When UI behavior or layout changes, update `designs/ui-prototype/polyshow.pen`.
- Preserve V1 separately in `designs/ui-prototype/polyshow-v1.pen`; do not keep V1 pages inside the active V2 file.
- Keep prototype screens state-based, not style variants.
- Current expected prototype states include:
  - `PolyShow V2 - Primitive Selected`
  - `PolyShow V2 - Layer Selected`
  - `PolyShow V2 - Empty State`
  - `PolyShow V2 - Search Open`
  - `PolyShow V2 - Menus and Dialogs`
  - `PolyShow V2 - Drawing IPC and Theme States`
- In Pencil, prefer `icon_font` nodes with `iconFontFamily: "Material Symbols Rounded"`.
- If the `.pen` prototype and Qt implementation disagree, inspect the current code and synchronize both docs and prototype.

## Editing Rules

- Preserve user changes and unrelated worktree changes.
- Do not rename files or directories unless the task requires it.
- Update `CMakeLists.txt` whenever source or header files are added, removed, or renamed.
- Keep Windows and Linux build behavior intact when touching CMake logic.
- Prefer small, focused patches over broad refactors.
- Do not add broad formatting-only changes.

## Project-Specific Notes

- The current target links `Qt::Core`, `Qt::Gui`, and `Qt::Widgets`.
- Windows packaging logic copies Qt runtime DLLs and the `qwindows` platform plugin after build.
- `README.md` may display with encoding issues in some terminals. Verify behavior from source files instead of relying on mojibake text.
- IPC layer behavior is restricted by layer type; do not allow external IPC writes into non-IPC layers.
- Layer and primitive visibility are separate UI states and must remain synchronized across sidebar, scene, and inspector.
