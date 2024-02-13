# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive.

## TODO

- Performance: Optimize rendering process (> 90% cpu time)

- Syntax Hightligher Multi-line comment `/* */`

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
