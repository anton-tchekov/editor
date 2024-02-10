# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive.

## TODO

### Very High Priority

- Refactor
	- Vector simplify code with realloc
	- Navbar autocomplete ugliness

- Bugs
	- Multi-line comment `/* */`
	- When using the up and down arrow keys, cursor does not always line up
		on the same collumn due to tabs being wider than other characters

### High Priority

- Add UTF-8 german umlaut support and maybe cyrillic?

- Text Selection
	- Select All (Ctrl+A)
	- Copy (Ctrl+C)
	- Cut (Ctrl+X)
	- Paste (Ctrl+V)

- Finish Navigation bar
	- Select file

### Medium Priority

- Multiple files open at the same time
- Create New File (Ctrl+N)
- Ask to save when exiting
	- You have the following unsaved files: Display List with
		discard and save option next to every file
	- Save all, Discard All, Cancel
	- Modified flag for each open file needed

- Search and replace in file / whole folder

### Low Priority

- Multi-line editing
