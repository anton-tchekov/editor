# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive.

**This is what it looks like editing its own source code:**

![Screenshot of Editor](scrnshot.png)

## TODO

- Highlight focused word
- Search and replace in file and whole folder
- Indent Selected using Tab (small bug with cursors)
- Shift tab work with spaces
- Abbreviations
- Align defines handle comments
- Unicode
- Undo / Redo
- Multiple Cursors

## Project overview

### Abbreviations

Two or three letter abbreviations are used as a module prefix.

```
BF  = Buffer Management [buffers.c]
TB  = Text Buffer [textbuf.c]

TF  = Text Field [textfld.c]
DD  = Dropdown Menu [dropdown.c]

GT  = Go to location [goto.c]
OB  = Open Buffers [opened.c]
CF  = Confirmation dialog [confirm.c]
MSG = Message (Error or Info) [msg.c]

VEC = Vector (Dynamic array) [vector.c]
KW  = Keyword hashtable (for syntax highlighting) [keyword.c]

SR  = Search and Replace [search.c]
SV  = Save File (As) [save_as.c]
OP  = Open File [open.c]
RR  = Rendering [render.c]
CR  = Cursor [cursor.c]
SEL = Selection [cursor.c]
```

### Common Functions

`example_init`\
Initialize example (for the first time)

`example_destroy`\
Cleanup example, when it is no longer used (Free memory, etc.)

`example_key`\
Example receives a keypress

`example_open`\
Make example visible on the screen and focus it

`example_clear`\
Reset example to initial state
(For example, text field to empty string)

`example_render`\
Example is rendered to the screen
