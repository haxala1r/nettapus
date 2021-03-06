# Contributing

If you have a feature request, bug fix or some idea you want to suggest, feel
free to simply open an issue/pull request.

# Code Style

When writing/contributing code to nettapus, make sure you follow these points
very carefully.

## Formatting

* Use LF at the end of lines
* Use tabs for indentation, spaces for alignment. Tab size does not matter.

  I.e. when you need to align macro values so that it looks cooler,
  use spaces. When you need to indent a block of code in a function, use tabs.


## C code

#### General

* Use `snake_case` for everything, except as noted. Abbrevs are allowed
  as long as they're obvious and/or they make sense.

* Asterisk to the right when declaring pointers:

  `int *ptr;`

* The same applies to function pointers, void pointers, and literally every
  type of pointer.

  `int32_t (*open)(struct file_vnode *, TASK *, uint8_t);`

#### Comments

* AVOID COMMENTS WHERE YOU CAN.

  Note that this does not mean that you shouldn't comment complex things,
  but rather when you feel you have to comment, you should be trying to
  simplify the code instead of writing a 3-page explanation. Then, only
  write that explanation when you feel it **absolutely cannot** be any
  simpler than it currently is.

  Ideally, there wouldn't be any comments, and what the code does would
  be obvious by just looking at it.

* Comments should use `/**/` instead of `//`, at all times.

#### Variables

* When making a short NULL-check (or something similar), do it on a single
  line (with braces) like this: `if (thing == NULL) { return NULL; };`

* Header files should declare things in this order:
  * Macros
  * Structures, unions, enums
  * Types
  * Functions

#### Structs

* Struct names should always be snake_case, and be descriptive of what the
  struct is for:
  `struct file_system { ... };`

* Typedef carefully. Generally, this rule follows along with the linux
  kernel's typedef policy. To quote:

  "In general, a pointer, or a struct that has elements that can reasonably
  be directly accessed should **never** be a typedef."

  When you *do* typedef, make the name of the type the same as the struct,
  except in all caps:

  `typedef struct semaphore SEMAPHORE;`

  The semaphore and the queue structures are a good example of what to typedef.
  They are not supposed to be modified/read in any way outside of their
  respective functions (they are "opaque")(there are exceptions, of course).

* When defining a variable with a certain struct, don't use the same name
  as a struct. Meaning:

  `struct file_system *fs;`

  is better than:

  `struct file_system *file_system;`

* When a struct's name is so generic that it matches generic names,
  you can simply use the first letter of the struct. I.e. if you're
  following the above rule, then you can't do this:

  `struct task *task;`

  In these cases, just do this:

  `struct task *t;`

  The type of the variable describes in enough detail the purpose of the
  variable, so there's no need to be more verbose.

* Macros should be `snake_case`, except in all caps. I.e. `SNAKE_CASE`.

#### Functions

* Function parameters should always be named, but kept as short as possible.
  If the type/struct of the parameter explains what purpose it serves, it
  is enough to just name it a single letter. If it's a generic type though
  (uint8_t for example), always provide a descriptive name for the parameter.

  `int32_t (*open)(struct file_vnode *n, struct task *t, uint8_t mode);`

  Still, try to keep your paramater names short, descriptive and simple.
  Don't name them `bytes_to_write` in a pointless endevaor to be descriptive.

  Just add a comment if you're desperate.

* Always put something in a function if you're going to use it more than once.
  (and sometimes even when you're *not* )

* A function should be no longer than a screenful. That said, keep it shorter
  than 25-30 lines if possible.

