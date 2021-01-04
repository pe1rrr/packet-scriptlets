# Makefile for Turbo C / Turbo C++
# 
# 7PLUS  ASCII-Encoder/Decoder
#

CC = tcc
CLINK = tlink
CLFLAGS = /c /x
CCFLAGS = -ms -d -G -O -Z
CLIB = c:\tc\lib

.c.obj:
	$(CC) -c $(CCFLAGS) $<

#
#  7PLUS
#
7PLOBJS = 	7plus.obj\
	  	decode.obj\
                correct.obj\
                rebuild.obj\
		encode.obj\
		extract.obj\
		join.obj\
                utils.obj

7PLHDRS = 	7plus.h globals.h


7plus.exe : $(7PLOBJS)
	$(CLINK) $(CLFLAGS)	$(CLIB)\c0s.obj+@tc_dos.tl,7plus,,$(CLIB)\cs

7plus.obj	: 7plus.c	7plus.h

decode.obj	: decode.c	$(7PLHDRS)

correct.obj	: correct.c	$(7PLHDRS)

rebuild.obj	: rebuild.c	$(7PLHDRS)

encode.obj	: encode.c	$(7PLHDRS) 

extract.obj	: extract.c	$(7PLHDRS)

join.obj	: join.c	$(7PLHDRS)

utils.obj	: utils.c	$(7PLHDRS)
