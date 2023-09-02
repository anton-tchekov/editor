# === DEFAULT MAKEFILE ===
# ~ Anton Tchekov

# This is a default Makefile designed for standard C projects. It automatically
# compiles all C files located in the source directory and all subdirectories.

# Executable Name
PROJECT := editor

# Compiler
CC := gcc

# Compiler flags
CFLAGS := -Wall -Wextra

# Linker flags
LDFLAGS :=

# Directory where source files are located
SRCDIR := src

# Directory where header files are located (can be the same as SRCDIR)
INCDIR := src

# Directory for object files
OBJDIR := obj

# Directory for binary output
BINDIR := .

# Processing
CFLAGS  += -I $(INCDIR)
TARGET  := $(BINDIR)/$(PROJECT)

SOURCES := $(shell find $(SRCDIR) -type f -name "*.c")
HEDEARS := $(shell find $(INCDIR) -type f -name "*.h")
OBJECTS := $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(addsuffix .o,$(basename $(SOURCES))))
DEPS    := $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(addsuffix .d,$(basename $(SOURCES))))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@$(CC) $(CFLAGS) -M $< -MT $@ > $(@:.o=.td)
	@cp $(@:.o=.td) $(@:.o=.d);
	@sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $(@:.o=.td) >> $(@:.o=.d);
	@rm -f $(@:.o=.td)

-include $(DEPS)

clean:
	rm $(OBJDIR)/* $(TARGET) -rf
