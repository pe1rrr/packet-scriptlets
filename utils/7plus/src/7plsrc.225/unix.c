/*--------------------------------*\
| Additions for UNIX-compatibility |
\*--------------------------------*/
#include "7plus.h"

#ifdef __MWERKS__
  char *strdup (const char *s1)
  { char *s;

    s = malloc (strlen(s1) + 1);
    strcpy (s , s1);
    return (s);
  }
#endif /* __MWERKS__ */

#ifdef __unix__

 #ifdef __vax__
  char *strdup (const char *s1)
  { char *s;

    s = malloc (strlen(s1) + 1);
    strcpy (s , s1);
    return (s);
  }
 #endif /* __vax__ */

 #ifdef __i386__
  #define MAXCMD 1024

  #ifndef _HAVE_RENAME
   int rename (const char *s1, const char *s2)
   {
     char tmp[MAXCMD];

     (void) sprintf(tmp, "mv %s %s", s1, s2);
     return (system(tmp));
  }
  #endif

  #ifndef _HAVE_STRSTR
   /*
     strstr - public-domain implementation of standard C library function

     last edit:  02-Sep-1990  D A Gwyn

     This is an original implementation based on an idea by D M Sunday,
     essentially the "quick search" algorithm described in CACM V33 N8.
     Unlike Sunday's implementation, this one does not wander past the
     ends of the strings (which can cause malfunctions under certain
     circumstances), nor does it require the length of the searched
     text to be determined in advance.  There are numerous other subtle
     improvements too.  The code is intended to be fully portable, but in
     environments that do not conform to the C standard, you should check
     the sections below marked "configure as required".  There are also
     a few compilation options, as follows:

     #define ROBUST  to obtain sane behavior when invoked with a null
         pointer argument, at a miniscule cost in speed
     #define ZAP  to use memset() to zero the shift[] array; this may
         be faster in some implementations, but could fail on
         unusual architectures
     #define DEBUG  to enable assertions (bug detection)
   */
   #define ROBUST
   #define ZAP

   #ifdef __STDC__
    #include  <limits.h>    /* defines UCHAR_MAX */

    #ifdef ZAP
     typedef void  *pointer;
     extern pointer  memset( pointer, int, size_t );
    #endif

   #else  /* normal UNIX-like C environment assumed; configure as required: */

    typedef unsigned  size_t;  /* type of result of sizeof */

    #ifndef NULL
     #define  NULL    0    /* null pointer constant */
    #endif

    #define  UCHAR_MAX  255    /* largest value of unsigned char */
                               /* 255 @ 8 bits, 65535 @ 16 bits  */

    #ifdef ZAP
     typedef char  *pointer;
     extern pointer  memset();
    #endif

    #define const  /* nothing */

   #endif  /* __STDC__ */

   #ifndef DEBUG
    #define  NDEBUG
   #endif

   #include <assert.h>

   typedef const unsigned char  cuc;  /* char variety used in algorithm */

   #define EOS  '\0'      /* C string terminator */

   char *          /* returns -> leftmost occurrence,
                      or null pointer if not present */
   strstr( s1, s2 )
     const char  *s1;       /* -> string to be searched */
     const char  *s2;       /* -> search-pattern string */
   {
     register cuc  *t;      /* -> text character being tested */
     register cuc  *p;      /* -> pattern char being tested */
     register cuc  *tx;     /* -> possible start of match */
     register size_t  m;    /* -> length of pattern */
     register cuc  *top;    /* -> high water mark in text */
   #if UCHAR_MAX > 255      /* too large for auto allocation */
     static        /* not malloc()ed; that can fail! */
   #endif          /* else allocate shift[] on stack */
       size_t  shift[UCHAR_MAX + 1];  /* pattern shift table */

   #ifdef ROBUST        /* not required by C standard */
     if ( s1 == NULL || s2 == NULL )
       return NULL;    /* certainly, no match is found! */
   #endif

     /* Precompute shift intervals based on the pattern;
        the length of the pattern is determined as a side effect: */

   #ifdef ZAP
     (void)memset( (pointer)&shift[1], 0, UCHAR_MAX * sizeof(size_t) );
   #else
     {
     register unsigned char  c;

     c = UCHAR_MAX;
     do
       shift[c] = 0;
     while ( --c > 0 );
     }
   #endif /* ZAP */
     /* Note: shift[0] is undefined at this point (fixed later). */

     for ( m = 1, p = (cuc *)s2; *p != EOS; ++m, ++p )
       shift[(cuc)*p] = m;

     assert(s2[m - 1] == EOS);

     {
     register unsigned char  c;

     c = UCHAR_MAX;
     do
       shift[c] = m - shift[c];
     while ( --c > 0 );

     /* Note: shift[0] is still undefined at this point. */
     }

     shift[0] = --m;    /* shift[EOS]; important details! */

     assert(s2[m] == EOS);

     /* Try to find the pattern in the text string: */

     for ( top = tx = (cuc *)s1; ; tx += shift[*(top = t)] )
     {
       for ( t = tx, p = (cuc *)s2; ; ++t, ++p )
       {
         if ( *p == EOS )       /* entire pattern matched */
           return (char *)tx;

         if ( *p != *t )
           break;
       }

       if ( t < top )    /* idea due to ado@elsie.nci.nih.gov */
         t = top;  /* already scanned this far for EOS */

       do
       {
         assert(m > 0);
         assert(t - tx < m);

         if ( *t == EOS )
           return NULL;  /* no match */
       }
       while ( ++t - tx != m );  /* < */
     }
   }
  #endif /* ifndef _HAVE_STRSTR */
 #endif /* ifdef __i386__      */
#endif /* ifdef __unix__      */


#ifdef OSK

 #include "7plus.h"


int setvbuf(FILE *stream, char *buf, int bufmode, size_t size)
{
  /* derzeit eine Dummyfunktion */

  return (0); /* Success */
}

char *strsave (const char *s1)
{ 
  register unsigned l = strlen(s1) + 1;
  register char *s = (char*)malloc (l);
  if(s ) memcpy (s , s1, l);
  return (s);
}

const char *strstr( const char *src, const char *sub )
{
  register int gefundene_position = findstr(1,src,sub);

  if(gefundene_position) 
    return (src+(gefundene_position-1));
  else
    return ( (const char *)0);	
}


/* Pruefen ob der angebotene Filename den Betriebssytem-Konventionen
   entspricht, gegebenenfalls entsprechend umbauen!! 
   Nicht zugelassene Zeichen werden gegen '_' ersetzt.
*/
void check_fn(register char * fn)
{
  register char tz;
  while((tz=*fn)!=0)
  {
    if (isalnum(tz) 
        || (tz == '.') 
        || (tz == '/')
        || (tz == '_') 
        || (tz == '$'))
    {;}
    else	
      *fn = '_';

    fn++;
  }
}

#include <sgstat.h>

static struct sgbuf original_buffer;
static struct sgbuf aktueller_zustand;

typedef enum { Unbekannt, Besetzt } Zustand;
static Zustand org_buff_zustand = Unbekannt;
#define SCF_CLASS 0
/*
       flag =0 == kein auto lf auf dem device
             1 ==      auto lf auf dem device
*/
void set_autolf(int flag)
{
  if(org_buff_zustand == Unbekannt )
  {
    if(_gs_opt(0,&original_buffer) != -1)
    {
      if( original_buffer.sg_class == SCF_CLASS)
      {
        memcpy(&aktueller_zustand,&original_buffer,sizeof(struct sgbuf));
        org_buff_zustand = Besetzt;
      }
    }
  }

  if( org_buff_zustand == Besetzt )  /* aktueller Zustand ebenfalls */
  {
    if( flag)
    {
      _ss_opt(0,&original_buffer);
    }
    else
    {
      aktueller_zustand.sg_alf   = 0;
      aktueller_zustand.sg_pause = 0;  /* switch page pause off */
      _ss_opt(0,&aktueller_zustand);
    }
  }
}  
#endif /*OSK*/
