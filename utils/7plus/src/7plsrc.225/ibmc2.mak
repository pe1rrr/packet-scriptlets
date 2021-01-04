# IBM Developer's Workframe/2 Make File Creation run at 20:36:13 on 03/02/93

# Make File Creation run in directory:
#   E:\TC\7PLUS;

.SUFFIXES:

.SUFFIXES: .c

7plus.exe:  \
  7PLUS.OBJ \
  CORRECT.OBJ \
  DECODE.OBJ \
  ENCODE.OBJ \
  EXTRACT.OBJ \
  JOIN.OBJ \
  REBUILD.OBJ \
  UTILS.OBJ \
  IBMC2.MAK
   @REM @<<7PLUS.@0
     /NOI /NOE /PM:VIO +
     7PLUS.OBJ +
     CORRECT.OBJ +
     DECODE.OBJ +
     ENCODE.OBJ +
     EXTRACT.OBJ +
     JOIN.OBJ +
     REBUILD.OBJ +
     UTILS.OBJ
     7PLUS.EXE
     
     
     ;
<<
   LINK386.EXE @7PLUS.@0

{.}.c.obj:
   ICC.EXE /Ss /Oi- /O /Q /C .\$*.c

!include IBMC2.DEP
