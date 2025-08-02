# Compiler options
CFLAGS = -Wall -Wextra -Werror -std=c11 -g
INCLUDE_DIR = ./include
LDFLAGS = -lbfd

# Define where to find source files
vpath %.c src

# Source files
SRC_FILES = isos_inject.c parser.c elf_header_operations.c elf_section_operations.c elf_binary_operations.c elf_utilities.c
# Convert source file names to object file names, placing them in the current directory
OBJ_FILES = $(SRC_FILES:.c=.o)
# Convert source file names to dependency file names
DEP_FILES = $(SRC_FILES:.c=.dep)

# Header files
HEADERS = $(INCLUDE_DIR)/parser.h $(INCLUDE_DIR)/elf64_types.h $(INCLUDE_DIR)/elf_header_operations.h \
$(INCLUDE_DIR)/elf_section_operations.h $(INCLUDE_DIR)/elf_binary_operations.h $(INCLUDE_DIR)/elf_utilities.h 

# Output binary
OUTPUT = ./isos_inject

# Build dependencies
build_dependencies: $(SRC_FILES:.c=.dep)
	@cat $^ > make.test
	@rm $^

%.dep: %.c 
	@gcc -MM -MF $@ $< -I$(INCLUDE_DIR)

# Main target: Build the isos_inject program by linking object files
$(OUTPUT): $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES) $(CFLAGS) -I$(INCLUDE_DIR) $(LDFLAGS)

# Rule to compile source files to object files, also generate dependency files
%.o: src/%.c $(HEADERS)
	$(CC) -c $< $(CFLAGS) -I$(INCLUDE_DIR) -o $@
	$(CC) -MM -MF $(@:.o=.dep) $< -I$(INCLUDE_DIR)

# Include dependency files if they exist
-include $(DEP_FILES)

# Syntax checks
syntax_check1:
	clang -fsyntax-only $(CFLAGS) -Wuninitialized -Wpointer-arith -Wcast-qual -Wcast-align -I$(INCLUDE_DIR) $(addprefix src/,$(SRC_FILES))

syntax_check2:
	gcc -O2 -Warray-bounds -Wsequence-point -Walloc-zero -Wnull-dereference -Wpointer-arith -Wcast-qual -Wcast-align=strict -o isos_inject -I$(INCLUDE_DIR) $(addprefix src/,$(SRC_FILES)) -lbfd

syntax_check3:
	gcc -fanalyzer -o isos_inject -I$(INCLUDE_DIR) $(addprefix src/,$(SRC_FILES)) -lbfd -lz -ldl

syntax_check4:
	clang-tidy $(addprefix src/,$(SRC_FILES)) -- -I$(INCLUDE_DIR)

syntax_check5:
	clang-tidy $(addprefix src/,$(SRC_FILES)) --checks='cert-*,clang-analyzer-*' -- -I$(INCLUDE_DIR)

syntax_check6:
	clang -fsanitize=address -g $(addprefix src/,$(SRC_FILES)) -I$(INCLUDE_DIR) -o isos_inject $(LDFLAGS)

syntax_check7:
	clang -fsanitize=memory -g $(addprefix src/,$(SRC_FILES)) -I$(INCLUDE_DIR) -o isos_inject $(LDFLAGS)

syntax_check8:
	clang -fsanitize=undefined -g $(addprefix src/,$(SRC_FILES)) -I$(INCLUDE_DIR) -o isos_inject $(LDFLAGS)

# Clean: remove object files and the isos_inject binary
clean:
	rm -rf *.o *.dep *.test ./isos_inject

# Help: display usage information
help:
	@echo "Do make isos_inject to build, make clean to clean object files. Bye."

# Declare Phony Targets - avoid conflicts with files of the same name
.PHONY: isos_inject clean help
