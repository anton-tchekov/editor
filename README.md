# editor

The goal of this project is to create a lightweight code editor written in C,
that is optimized for C projects, is performant, and productive.

**This is what it looks like editing its own source code:**

![Screenshot of Editor](screenshot0.png)

## TODO

- Nav Bar Select file and open
- Search and replace in file / whole folder
- Undo / Redo

## Interesting Stuff

There are a lot of egde cases to consider:

Here is a funny example. Lets say I have one file open `test.txt`, which I have
modified, but not saved. Then I create a new file, write something in it
and press save. Then as the filename, I enter `test.txt`. So my code has to
check if the save target file is already open, and if it was modified.
If it was not modified, the tab with the old contents is simply closed.
If it was modified, a prompt needs to be shown to the user, to ask which
version they would like to keep and which to discard. And of course on every
step along the way the user could press cancel or the write operation could
fail because the permissions of the file changed for example.
