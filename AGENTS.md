# AGENTS.md

Guidance for coding agents working in the PolyShow repository.

## Communication

- Address the user as `dady`.
- Prefer replying in Chinese when the user writes in Chinese.
- Confirm behavior from the repository before making assumptions.
- Keep changes minimal, practical, and easy to review.

## Project Snapshot

- `polyshow` is a Qt 6 Widgets desktop application written in C++17.
- The app renders PolyShow's custom 2D `.ply` text format. It is not the Stanford PLY format.
- The current source layout is:
  - `Main.cpp`
  - `include/core`, `src/core`
  - `include/parsers`, `src/parsers`
  - `include/ui`, `src/ui`
  - `test_data/ply` for manual verification files
  - `designs/ui-prototype` for design references
- `CMakeLists.txt` is the build source of truth.
- `CODE_STYLE.md` is the coding-style source of truth when older notes conflict with current code.

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
- If you change parsing, rendering, or UI state sync, verify at least one valid and one invalid sample.

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
  2. Qt headers
  3. Standard library headers
  4. Third-party headers
- There is no repository `.clang-format` file at the moment, so avoid broad formatting-only churn.

## Comments and Documentation

- Use `///` for API comments in headers.
- Use short `//` comments for non-obvious logic in source files.
- Comment intent, constraints, or invariants instead of restating code line by line.
- Do not add noisy comment-per-line blocks just to satisfy an old template. Match the current repository style.
- TODO, FIXME, and HACK comments must include an owner and context.

## Editing Rules

- Preserve user changes and unrelated worktree changes.
- Do not rename files or directories unless the task requires it.
- Update `CMakeLists.txt` whenever source or header files are added, removed, or renamed.
- Keep Windows and Linux build behavior intact when touching CMake logic.
- Prefer small, focused patches over broad refactors.
- If behavior is defined by code and contradicted by outdated docs, trust the code and update the docs.

## Project-Specific Notes

- The current target links `Qt::Core`, `Qt::Gui`, and `Qt::Widgets`.
- Windows packaging logic copies Qt runtime DLLs and the `qwindows` platform plugin after build. Be careful when editing deployment logic.
- `README.md` may display with encoding issues in some terminals. Verify behavior from source files instead of relying on mojibake text.
