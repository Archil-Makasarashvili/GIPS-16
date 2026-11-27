# Firmware C style

Short local rules, based on the applicable readability guidance in the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). C and ESP-IDF conventions take precedence where they differ.

- Use 2 spaces for indentation and aim for lines under 80 columns. Put an opening `{` on the next line for functions and control blocks.
- Keep `.c` and `.h` basenames identical for a module. Keep related files in one driver folder.
- Use clear `snake_case` names and the existing `gips_` prefix for public functions and types.
- Public functions are declared in the matching `.h` file and defined without `static`. File-local functions use `static` and have prototypes near the top of the `.c` file, before any function definitions. ESP-IDF entry points such as `app_main` are exceptions to the header rule.
- In `.c` and `.h` files, use two empty lines between top-level groups (includes, constants, module state, declarations, local function prototypes) and between function definitions. Keep related includes together within the include group.
- Prefix module-private mutable variables with `_`; exported mutable globals with `g_`; named constants with `k_`. Macros use uppercase names. Avoid exported globals. Prefer local variables and function parameters over persistent state.
- Prefer standard C and ESP-IDF libraries and data structures when they fit. Add a project-specific type or helper only when it makes the code clearer or handles a real constraint.
- Keep functions short and control flow direct. Add brief comments around nontrivial logic, register bits, assumptions, and corner cases. Explain why a less obvious path is needed; do not narrate obvious statements.
- Use ESP-IDF error codes and check I/O errors. Validate inputs at public API boundaries.
