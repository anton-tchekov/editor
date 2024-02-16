# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive.

## TODO

- CTRL+Backspace, CTRL+Delete
- Text Selection
- Select All (Ctrl+A)
- Copy (Ctrl+C)
- Cut (Ctrl+X)
- Paste (Ctrl+V)

- Finish Navigation bar
	- Select file and open
	- Refactor navbar autocomplete ugliness

- Syntax Hightligher Multi-line comment `/* */`
- Performance: Optimize rendering process (> 90% cpu time)

- Search and replace in file / whole folder
- Multiple files open at the same time
- Create New File (Ctrl+N)
- Ask to save when exiting
	- You have the following unsaved files: Display List with
		discard and save option next to every file
	- Save all, Discard All, Cancel
	- Modified flag for each open file needed

- Add UTF-8 german umlaut and cyrillic support
- Undo / Redo
- Multi-line editing
