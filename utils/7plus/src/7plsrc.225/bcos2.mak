#=============================================================
#
#	7PLUS.MAK - Makefile for project 7plus.prj
#		Created on 03/25/93 at 00:11
#                  Borland C++ v1.0 (OS/2)
#
#=============================================================

.AUTODEPEND

.PATH.obj = .

#=============================================================
#		Translator Definitions
#=============================================================
CC = bcc +7PL_CFG
TASM = tasm.exe
TLIB = tlib.exe
TLINK = tlink
RC = brcc.exe
RB = rc.exe
SOURCEPATH= E:\TC\7PLUS
CPATH= D:\BCOS2
LIBPATH = $(CPATH)\LIB
INCLUDEPATH = $(CPATH)\INCLUDE;$(SOURCEPATH)


#=============================================================
#		Implicit Rules
#=============================================================
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

.asm.obj:
  $(TASM) -Mx $*.asm,$*.obj

.rc.res:
  $(RC) -r $*.rc

#=============================================================
#		List Macros
#=============================================================


EXE_DEPENDENCIES =  \
 utils.obj \
 rebuild.obj \
 join.obj \
 extract.obj \
 encode.obj \
 decode.obj \
 correct.obj \
 7plus.obj

#=============================================================
#		Explicit Rules
#=============================================================
7plus.exe: 7PL_CFG $(EXE_DEPENDENCIES)
  $(TLINK) /c /x /Toe /ap /L$(LIBPATH) @&&|
$(LIBPATH)\C02.OBJ+
utils.obj+
rebuild.obj+
join.obj+
extract.obj+
encode.obj+
decode.obj+
correct.obj+
7plus.obj
7plus
		# no map file
$(LIBPATH)\C2.LIB+
$(LIBPATH)\OS2.LIB

|


#=============================================================
#		Individual File Dependencies
#=============================================================
UTILS.obj: 7PL_CFG $(SOURCEPATH)\UTILS.C
	$(CC) -c $(SOURCEPATH)\UTILS.C

REBUILD.obj: 7PL_CFG $(SOURCEPATH)\REBUILD.C
	$(CC) -c $(SOURCEPATH)\REBUILD.C

JOIN.obj: 7PL_CFG $(SOURCEPATH)\JOIN.C
	$(CC) -c $(SOURCEPATH)\JOIN.C

EXTRACT.obj: 7PL_CFG $(SOURCEPATH)\EXTRACT.C
	$(CC) -c $(SOURCEPATH)\EXTRACT.C

ENCODE.obj: 7PL_CFG $(SOURCEPATH)\ENCODE.C
	$(CC) -c $(SOURCEPATH)\ENCODE.C

DECODE.obj: 7PL_CFG $(SOURCEPATH)\DECODE.C
	$(CC) -c $(SOURCEPATH)\DECODE.C

CORRECT.obj: 7PL_CFG $(SOURCEPATH)\CORRECT.C
	$(CC) -c $(SOURCEPATH)\CORRECT.C

7PLUS.obj: 7PL_CFG $(SOURCEPATH)\7PLUS.C
	$(CC) -c $(SOURCEPATH)\7PLUS.C

#=============================================================
#		Compiler Configuration File
#=============================================================
7PL_CFG: 7plus.mak
  copy &&|
-Oi
-Oz
-Ob
-Oe
-Oc
-L$(LIBPATH)
-I$(INCLUDEPATH)
-H=7plus.CSM
-vi-
-d
-k-
-O
-Ot
-w
-K
| 7PL_CFG


