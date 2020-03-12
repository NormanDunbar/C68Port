CC = gcc
SOURCES = c68port.c \
          keywords.c
HEADERS = c68port.h \
          keywords.h

DEBUG_FLAGS = -O0 -g -m32
CC_FLAGS= -O2 -m32 

all: release

release: $(SOURCES) 
	$(CC) -o C68Port $(CC_FLAGS) $?


$(SOURCES): $(HEADERS)

debug: $(SOURCES) 
	$(CC) -o C68Port $(DEBUG_FLAGS) $?
	