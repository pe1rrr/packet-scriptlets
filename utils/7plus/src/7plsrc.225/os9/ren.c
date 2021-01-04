
/*
  Eine einfache implementierung der rename Funktion
  muesste eigentlich unter allen Betriebssystemen laufen
  wenn man mal vom overhead absieht
   Do. 17.12.1992 21:21:38 DG1ECN
    
*/
#include <stdio.h>
#include "../7plus.h"

int rename (register const char *old, register const char * new)
{
	char comb[2*MAXPATH];
	
	sprintf(comb,"rename %s %s",old,new);

	return system(comb);	
}
