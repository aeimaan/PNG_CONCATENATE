# Makefile, ECE252  
# Zoso Robati

CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 -Ilib# compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz -pthread -lcurl  # link with libz


OBJ_DIR = obj
SRC_DIR = lib
LIB_UTIL = $(OBJ_DIR)/shm_stack.o $(OBJ_DIR)/zutil.o $(OBJ_DIR)/crc.o $(OBJ_DIR)/lab_png.o $(OBJ_DIR)/catpng.o
SRCS   = catpng.c crc.c zutil.c lab_png.c paster2.c
OBJS_FINDPNG   = $(OBJ_DIR)/paster2.o $(LIB_UTIL) 

TARGETS= paster2

all: ${TARGETS}

paster2: $(OBJS_FINDPNG) 
	mkdir -p $(OBJ_DIR)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

$(OBJ_DIR)/paster2.o: paster2.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDLIBS)

.PHONY: clean
clean:
	rm -f *.d *.o $(OBJ_DIR)/*.d $(OBJ_DIR)/*.o  $(TARGETS) 
