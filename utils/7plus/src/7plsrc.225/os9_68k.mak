#
# Makefile for 7PLUS under OS-9
#  14.12.1992 

TARGET = 7plus

# define the object suffix of your system.
# for MessDOS you should define "obj".
O = r

# define Switches

SWITCHES = -D_HAVE_MKTIME -D_HAVE_GMTIME -DOSK


# choose compiler and linker and their flags
# GNU cc Version 1.42:

CC = gcc
LD = gcc


CFLAGS = -ansi -o68 -c -o $(RDIR)/$@  $(SWITCHES) -o68 \
          -T=/r0 -mnostack-check -fcombine-regs -fomit-frame-pointer \
          -fstrength-reduce -O  -W -Wunused -Wreturn-type -Wswitch \
          -Wpointer-arith -Wcast-qual -Wwrite-strings  -Wshadow

LDFLAGS = -cio -v -Q -x -e $(EDITION) -ob -g -s 30

# choose defines necessary for your system
DEFINES =

RDIR = RELS
R = $(RDIR)

ODIR = /dd/user/cmds

EDITION = 1

OBJS =  sevplus.$O decode.$O rebuild.$O  extract.$O \
    join.$O utils.$O unix.$O encode.$O correct.$O stat.$O ren.$O

ROBJS = $(R)/sevplus.$O $(R)/decode.$O $(R)/rebuild.$O $(R)/extract.$O \
    $(R)/join.$O $(R)/utils.$O $(R)/unix.$O $(R)/encode.$O $(R)/correct.$O \
    $(R)/stat.$(O) $R/ren.$O 

.c.r:
    $(CC) -c $(CFLAGS) $(DEFINES) $*.c

$(TARGET): $(OBJS) 
    $(LD) $(LDFLAGS) -o $(TARGET) $(ROBJS)

ren.$O : os9/ren.c
    $(CC) -c $(CFLAGS) $(DEFINES) os9/$*.c

stat.$(O) : os9/stat.c
    $(CC) -c $(CFLAGS) $(DEFINES) os9/$*.c

sevplus.$(O): sevplus.c 7plus.c 7plus.h

decode.$(O): decode.c 7plus.h globals.h

rebuild.$(O) : rebuild.c 7plus.h globals.h

encode.$(O) : encode.c 7plus.h globals.h

extract.$(O) : extract.c 7plus.h globals.h

join.$(O) : join.c 7plus.h globals.h

utils.$(O) : utils.c 7plus.h globals.h

unix.$(O) : unix.c 7plus.h globals.h

correct.$(O) : correct.c 7plus.h globals.h
