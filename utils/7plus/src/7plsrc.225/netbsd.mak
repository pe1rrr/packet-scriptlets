#
# Makefile for 7PLUS under NetBSD
#

CC = gcc
LD = gcc
CFLAGS = -O2 -Wall -pedantic -ansi
LDFLAGS = -s -ansi

INSTALL = install -c
INSTALL_PROGRAM = $(INSTALL) -s
MKDIR = mkdir -p

DEFINES = -D__unix__
TARGET = 7plus

ifndef PREFIX
  PREFIX = /usr/local
endif

SRCS =  7plus.c  encode.c  correct.c rebuild.c  decode.c \
	extract.c join.c  utils.c  unix.c

OBJS =  7plus.o encode.o correct.o rebuild.o decode.o \
	extract.o join.o utils.o unix.o

.c.o:
	$(CC) -c $(CFLAGS) $(SPECFLAGS) $(DEFINES) $*.c

all: $(OBJS)
	$(LD) $(LDFLAGS) $(SPECFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

install: all
	$(INSTALL_PROGRAM) 7plus $(PREFIX)/bin

depend:
	$(CC) -M $(SRCS) >depend.out

clean:
	rm -f $(OBJS)
	rm -f $(TARGET) core a.out depend.out

7plus.o : 7plus.c 7plus.h
encode.o : encode.c 7plus.h globals.h
rebuild.o : rebuild.c 7plus.h globals.h
decode.o : decode.c 7plus.h globals.h
correct.o : correct.c 7plus.h globals.h
extract.o : extract.c 7plus.h globals.h 
join.o : join.c 7plus.h globals.h 
utils.o : utils.c 7plus.h globals.h 
unix.o : unix.c 7plus.h
