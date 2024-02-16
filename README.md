# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive.

## TODO (in order)

- Paste (Ctrl+V)
- Performance: Optimize rendering process by using HW acceleration

- Finish Navigation bar
	- Select file and open
	- Refactor navbar autocomplete ugliness

- Syntax Hightligher Multi-line comment `/* */`

- Search and replace in file / whole folder
- Multiple files open at the same time
- Create New File (Ctrl+N)
- Ask to save when exiting
	- You have the following unsaved files: Display List with
		discard and save option next to every file
	- Save all, Discard All, Cancel
	- Modified flag for each open file needed

- Mouse support
- Collapse/Expand code block
- Add UTF-8 german umlaut and cyrillic support
- Undo / Redo
- Multi-line editing
