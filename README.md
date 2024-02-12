# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive.

## TODO

- Syntax Hightligher Multi-line comment `/* */`

- BUG: When using the up and down arrow keys, cursor does not always line
	up on the same collumn due to tabs being wider than other characters

- Add UTF-8 german umlaut and cyrillic support

- Text Selection
	- Select All (Ctrl+A)
	- Copy (Ctrl+C)
	- Cut (Ctrl+X)
	- Paste (Ctrl+V)

- Finish Navigation bar
	- Select file
	- Refactor navbar autocomplete ugliness

- Multiple files open at the same time
- Create New File (Ctrl+N)
- Ask to save when exiting
	- You have the following unsaved files: Display List with
		discard and save option next to every file
	- Save all, Discard All, Cancel
	- Modified flag for each open file needed

- Search and replace in file / whole folder

- Multi-line editing

- Undo / Redo
