# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive.

## TODO

### High Priority

- Refactor
- Bugs
	- Multi-line comment `/* */`
	- When using the up and down arrow keys, cursor does not always line up
		on the same collumn due to tabs being wider than other characters
	- When opening file check for valid text during loading, not after
	- FIXMEs in code

- Text Selection
	- Select All (Ctrl+A)
	- Copy (Ctrl+C)
	- Cut (Ctrl+X)
	- Paste (Ctrl+V)

- Finish Navigation bar
	- Select file

### Medium Priority

- Multiple files open at the same time
- Ask to save when exiting
	- You have the following unsaved files: Display List with discard option
		next to every file
	- Save all, Discard All, Cancel
	- Modified flag for each open file

- Search and replace in file / folder

### Low Priority

- Multi-line editing
