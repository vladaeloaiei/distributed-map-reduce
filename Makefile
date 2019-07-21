# COMPILER / LINKER
CC		= mpicc
LINKER	= mpicc

# BINARY_NAME
TARGET	= dmr.out

# INCLUDE DIRECTORIES
INC_DIR	= inc
SRC_DIR	= src
OBJ_DIR	= obj
BIN_DIR	= bin
RES_DIR = result

# include / source / object files
INCLUDES := $(wildcard $(INC_DIR)/*.h)
SOURCES  := $(wildcard $(SRC_DIR)/*.c)
OBJECTS  := $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# compiling flags here
CFLAGS	= -Wall -fopenmp -I/usr/include/mpi $(INC_DIR:%=-I%)

# linking flags here
LFLAGS	= -Wall -lm

# linking libs
LDLIBS	= -fopenmp

# linking stage, but first he will call the compiler
$(BIN_DIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) $(LDLIBS) -o $@
	@echo "Linking complete!"

# compile stage
$(OBJECTS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

# make clean
.PHONY: clean
clean:
	@rm -f $(OBJ_DIR)/* $(BIN_DIR)/*
	@rm -f $(RES_DIR)/*
	@rm -f log.txt
	@echo "Cleanup complete!"
clean-result:
	@rm -f $(RES_DIR)/*
	@rm -f log.txt
	@echo "Result cleaned!"
