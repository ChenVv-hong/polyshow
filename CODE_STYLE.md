# PolyShow Coding Style

This document captures the active coding style used in the PolyShow repository after the naming and formatting cleanup.

## 1. Core Principles

- Prefer readability over cleverness.
- Keep naming, comments, file organization, and build scripts consistent across the repository.
- When touching an area with outdated style, converge that area toward the current standard in the same change when practical.

## 2. Naming Rules

- File names use PascalCase: `GeometryScene.h`, `MainWindow.cpp`
- Classes, structs, enums, and type aliases use PascalCase: `GeometryScene`, `PrimitiveStyle`
- Functions, methods, signals, slots, local variables, and parameters use camelCase: `setRenderMode()`, `currentShapeLine`
- Non-private data members use lower_snake_case: `fill_color`, `point_size`
- Private data members use `m_` + lower_snake_case: `m_render_mode`, `m_status_info_label`
- Constants prefer Google-style `kPascalCase`: `kGridStep`
- Avoid macros; if a macro is required, use `UPPER_SNAKE_CASE`

## 3. File Organization

- Headers live under `include/<module>/`
- Sources live under `src/<module>/`
- Header and source names must match exactly, for example:
  `include/ui/MainWindow.h` <-> `src/ui/MainWindow.cpp`
- One file should contain one primary class or one tight group of related lightweight types
- Module directories remain semantic and lowercase: `core`, `parsers`, `ui`

## 4. Include Order

`.cpp` files should order includes like this:

1. Matching header
2. Qt headers, sorted alphabetically
3. Standard library headers, sorted alphabetically
4. Third-party headers

Headers should include only what they need for complete types; prefer forward declarations when possible.

## 5. Type and Interface Rules

- Add `[[nodiscard]]` to query-style interfaces when ignoring the result is likely a mistake
- Prefer `const T &` for non-trivial parameters unless move semantics are needed
- For QObject-derived types, prefer Qt parent-child ownership
- Mark QObject / QWidget classes `final` when subclassing is not planned
- Name booleans with meaningful prefixes such as `is`, `has`, or `should`

## 6. Comment Rules

- Every class, struct, public function, and private helper function must have a comment
- Every function should include comments for the important logic blocks, especially where intent is not obvious
- Use `///` for interface comments in headers
- Use `//` for implementation-flow comments in source files
- Comments should explain intent or constraints, not restate the code mechanically
- TODO / FIXME / HACK comments must include an owner and context, for example:
  `// TODO(zhangsan): support 3D view mode`

## 7. Formatting Rules

- Use 4 spaces for indentation, never tabs
- Keep lines at or under 120 characters when practical
- Use Allman braces for functions, classes, and namespaces
- Use K&R braces for `if`, `for`, `switch`, and similar control blocks
- Use `Type *name` and `Type &name`
- Keep spaces around operators and after commas
- Use `nullptr` instead of `NULL`
- Prefer range-based `for` unless the index is needed

## 8. C++ / Qt Rules

- Use C++17
- Prefer `QStringLiteral()` for string literals used in Qt APIs
- Prefer Qt types when the data flows through UI or signal-slot APIs
- Use RAII, smart pointers, or Qt ownership to manage lifetime
- Prefer Qt abstractions for cross-platform file and path handling

## 9. CMake Rules

- Source paths in CMake must match on-disk file names exactly, including case
- Prefer a target-name variable for complex targets to reduce repeated literals
- Always spell out `PRIVATE`, `PUBLIC`, or `INTERFACE` in `target_link_libraries()`
- Keep platform-specific logic localized instead of scattering duplicated snippets

## 10. Pre-Commit Checklist

- Confirm file names, class names, and member names follow this standard
- Confirm all new functions and important logic blocks have comments
- Confirm include order, spacing, and blank-line usage are consistent
- Confirm renamed files are reflected in CMake and any supporting docs
- Run at least one local build before submitting changes
