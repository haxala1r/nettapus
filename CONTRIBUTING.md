# Code Style
## Formatting

 - Use TABs over spaces for indentation
 - Use LF at the end of lines

## C code
 - Use `snake_case`
 - Asterisk to the right when declaring pointers:
	`int *ptr;`
 - Comments should use `/**/` instead of `//`
 - Always put something in a function if you're going to use it more than once
 - Comments should be simple and straight-forward
 - When making a short NULL-check, do it on a single line like this:
	`if (thing == NULL) { return NULL; };`
 - Header files should declare things in this order:
	- Macros
	- Structures, unions, enums
	- Types
	- Functions
 - Struct names should always be snake_case, with an "_s" appended to it :
 `struct file_system_s { ... };`
 - Type names should always be the same as their corresponding struct,
 except in caps and without the "_s": `typedef struct file_system_s FILE_SYSTEM;`
 - Function and variable names should always be snake_case.
 - When defining variables or functions that use a certain struct, always
 use the type defined for that struct, and if one does not exist, make it.
 NEVER, EVER DEFINE A VARIABLE LIKE THIS: `struct file_system_s *fs;` UNLESS
 INSIDE THE STRUCT ITSELF, OR ANOTHER STRUCT IN THE SAME FILE.
 - Macros should be in all caps, and should not be similar to any type name.
