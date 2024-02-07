# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive for myself.

## TODO
Features that I plan to implement in the near future:

- Select Text combined with all other editing operations, plus:
	- Select All (Ctrl+A)
	- Copy (Ctrl+C)
	- Cut (Ctrl+X)
	- Paste (Ctrl+V)

- Refactor: Fix types, new allocator, remove old cruft
	- Replace "vector" with textbuffer
	- Replace malloc calls with own allocator

- Multi-line editing

- Load and save files
- Goto [file:line] in Ctrl+G mode

- Search and replace

## Bugs
- A multitude of crashes due to diverse reasons xD
- Tab Up Down Bug

### Syntax highlighting
- Multi-line comment
- Octal, Binary, Hex
