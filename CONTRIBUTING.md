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
	- Structures, unions, enums
	- Types
	- Functions
