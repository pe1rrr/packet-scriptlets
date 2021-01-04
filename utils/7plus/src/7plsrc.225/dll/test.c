/*----------------------------------------------------------------------------*/
/*                                                                            */
/*                                   TEST.C                                   */
/*                                                                            */
/*       Demonstrates how to load and link into a dynamic link library        */
/*                                                                            */
/*       This simple demonstration loads the 7plus dynamic link library       */
/*               and passes on to it the command line arguements              */
/*                      it is only a demo, use with care                      */
/*                                                                            */
/*                Example ...   TEST -SB 5000 "Long Name.zip"                 */
/*                                                                            */
/*       Long file names are supported in the 32 bit DLL, and should be       */
/*         surrounded with quotes, short file names don't need quotes         */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include <windows.h>

HINSTANCE hInst;

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpszArgs, int nWinMode)
{
HINSTANCE hinstDo7plus;
int (FAR *lpfnDo7plus) (char FAR*);

/*
 * The DLL in this example is called 7PLUSDLL.DLL
 */
hinstDo7plus = LoadLibrary("7PLUSDLL.DLL");

/*
 * Load DLL
 */
if (hinstDo7plus == NULL)
   {
   MessageBox(0,"Cannot load\n7PLUSDLL.DLL","Error",MB_ICONSTOP | MB_OK);
   goto end;
   }

/*
 * The entry point into the DLL is in this example called Do_7plus
 */
(FARPROC) lpfnDo7plus = GetProcAddress(hinstDo7plus,"Do_7plus");

if (lpfnDo7plus == NULL)
   {
   MessageBox(0,"Cannot Link DLL","Error",MB_ICONSTOP | MB_OK);
   goto end;
   }

/*
 * Execute the DLL passing it the command line arguements
 */
lpfnDo7plus((char FAR*) lpszArgs);

end:
FreeLibrary(hinstDo7plus);
return 0;
}



