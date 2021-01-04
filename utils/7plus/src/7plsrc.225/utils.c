#include "7plus.h"
#include "globals.h"

#if (defined (__OS2__) || defined (__WIN32__))
 #ifdef __BORLANDC__
  #include <utime.h>
  #define _FTIMEDEFINED
  #define BRLND_PUTC_BUG (short)
 #else
  #include <sys/utime.h>
 #endif
#endif

#if !defined (BRLND_PUTC_BUG)
 #define BRLND_PUTC_BUG
#endif

#ifdef __unix__
 #ifdef __M_XENIX__
  struct utimbuf {
   time_t actime;
   time_t modtime;
  };
  extern time_t mktime (struct tm *utm);
 #else
  #include <utime.h>
 #endif
#endif

#ifdef __MWERKS__
 #include <time.h>
#endif

#ifdef _AMIGA_
 struct utimbuf {
   time_t actime;
   time_t modtime;
 };
 extern time_t mktime (struct tm *utm);
#endif

const char no[] = NO, yes[] = YES, always[] = ALWAYS;

/*
*** get a line from file. don't care about type of line separator.
***
***
 */

char *my_fgets (char *string, register int n, FILE *rein)
{
  register int in, i;

  if (feof (rein))
    return (NULL);

  i = 0;
  while ((in = fgetc (rein)) != EOF)
  {
    if (in == 0x0d)
    {
      if ((in = fgetc (rein)) != 0x0a)
        if (in != EOF)
          ungetc (in, rein);
      in = LSEP;
    }
    string[i++] = (char) in;
    if (i == n || in == LSEP)
      break;
  }
  string[i] = EOS;
  return (string);
}

/*
***
*** Write a byte to file.
***
 */

int my_putc (int outchar, FILE *out)
{
  register int x;

  if ((x = putc (BRLND_PUTC_BUG(outchar & 0xff), out)) == EOF)
  {
    fprintf (o, "\007\nWrite error! Can't continue.\n");
    exit (1);
  }
  return (x);
}

/*
*** Get crc and line number from code line.
***
***
 */

void crc_n_lnum (uint *crc, int *linenumber, char *line)
{
  register ulong cs;

  cs = 0xb640L * decode[(byte)line[66]] +
       0xd8L   * decode[(byte)line[65]] +
                 decode[(byte)line[64]];

  *linenumber = (int) (cs >> 14);   /* upper 9 bits are the line number */
  *crc = (uint) (cs & 0x3fffL);     /* lower 14 bits are the CRC */
}

/*
*** Get crc2 from code line.
***
***
 */

void crc2 (uint *crc, char *line)
{

  *crc = 0xd8 * decode[(byte)line[68]] +
                decode[(byte)line[67]];
}

/*
*** Whip up 2nd CRC
***
***
 */

void add_crc2 (char *line)
{
  register uint crc;
  register int i;

  /* Whip up 2nd CRC */
  crc = 0;
  for (i=66;i>-1;i--)
    crc_calc (crc, line[i]);
  crc &= 0x7fff;

  i = 67;
  line[i++] = code[crc % 0xd8];
  line[i++] = code[crc / 0xd8];
  line[i] = EOS;
}

/*
*** mini-crc for header. safe enough...
***
***
 */

int mcrc (char *line, int flag)
{
  register int i, j;
  register uint crc;
  char test[3], *p;

  sprintf (test, "\xb0\xb1");

  if ((p = strstr (line, test)) == NULL)
    return (0);

  j = (int) (p - line) + 4;

  for (i=crc=0; i<j; i++)
    crc_calc (crc, line[i]);
  crc %= 216;
  if (!flag)
  {
    if (crc == (uint) decode[(byte)line[j]])
      return (1);
    else
      return (0);
  }
  else
    line[j] = code[(byte)crc];

  return (crc);
}

/*
*** read info from indexfile
***
***
 */

int read_index (FILE *ifile, struct m_index *idxptr)
{
  int j;
  ulong i, begin, end;

  #ifdef _HAVE_CHSIZE
   fseek (ifile, -4L, SEEK_END);
   fseek (ifile, (long) read_ulong (ifile), SEEK_SET);
  #endif

  /* clear index */
  for (j = 0;j < 4088; j++)
    idxptr->lines_ok[j] = 0UL;

  my_fgets (idxptr->full_name, 12, ifile);
  if (strcmp(idxptr->full_name, "7PLUS-index"LSEPS))
  {
    fprintf (o, "\007No index info found.\n");
    return (1);
  }
  my_fgets (idxptr->filename, 13, ifile);
  idxptr->filename[(int)strlen(idxptr->filename)-1] = EOS;
  my_fgets (idxptr->full_name, 257, ifile);
  idxptr->full_name[(int)strlen(idxptr->full_name)-1] = EOS;
  idxptr->timestamp = read_ulong (ifile);
  idxptr->splitsize = read_uint  (ifile);
  idxptr->lines_left= (long) read_ulong (ifile);

  /* convert defect list into bitvektor */
  while (1==1)
  {
    if ((begin = read_ulong (ifile)) == 0xffffffffUL)
      break;
    end = read_ulong (ifile);

    if ((end>>5) > (begin>>5))
      for (i = begin;i < ((begin |31UL) + 1UL); i++)
	idxptr->lines_ok[(int)(i>>5)] |= (1UL <<(i&31UL));
    else
    {
      for (i = begin;i < end; i++)
	idxptr->lines_ok[(int)(i>>5)] |= (1UL <<(i&31UL));
      continue;
    }

    for (i = (begin>>5) +1; i < (end>>5); i++)
      idxptr->lines_ok[(int)i] = 0xffffffffUL;

    if (end&31)
      for (i = end &0xffffffe0UL; i < end; i++)
	idxptr->lines_ok[(int)(i>>5)] |= (1UL <<(i&31UL));
  }
  idxptr->length = read_ulong (ifile);

  #ifdef _HAVE_CHSIZE
   fseek (ifile, 0L, SEEK_SET);
  #endif

  return (0);
}

/*
*** write info to indexfile
***
***
 */

int write_index (FILE *ifile, struct m_index *idxptr, int flag)
{
  int j, part, prevpart, lines;
  ulong i, begin, end;

  prevpart = lines = 0;

  if (!flag)
  {
    /* Update index at end of meta file */
    #ifdef _HAVE_CHSIZE
     /* IBM C Set/2 fails, if fseeking to the end of a random file.
        Following output is not appended, but written to the beginning
        of the file, although the file length grows by the correct amount!
        After doing chzise first, it works. */
     chsize (fileno (ifile), (long) idxptr->length);
     fseek (ifile, (long) idxptr->length, SEEK_SET);
    #endif

    fprintf (ifile, "7PLUS-index\n");
    fprintf (ifile, "%s\n", idxptr->filename);
    fprintf (ifile, "%s\n", idxptr->full_name);
    write_ulong (ifile, idxptr->timestamp);
    write_uint  (ifile, idxptr->splitsize);
    write_ulong (ifile, (ulong) idxptr->lines_left);
  }
  /* convert bitvektor into defect list (gap list)*/
  i = 0UL;
  j = 0;
  while (1==1)
  {
    while (j < 4080 && !(idxptr->lines_ok[j] &(1UL <<(i & 31UL))))
    {
      if (!(i&31UL) && idxptr->lines_ok[j] == 0UL)
      {
        j++;
	i = ((ulong) j) <<5;
      }
      else
      {
        i++;
	j = (int) (i>>5);
      }
    }

    if (j == 4080)
      break;

    begin = i;

    do
    {
      if (!(i&31UL) && idxptr->lines_ok[j] == 0xffffffffUL)
      {
        j++;
	i = ((ulong) j) <<5;
      }
      else
      {
        i++;
	j = (int) (i>>5);
      }
    }
    while (j < 4080 && (idxptr->lines_ok[j] &(1UL <<(i & 31UL))));

    end = i;

    if (!flag)
    {
      write_ulong (ifile, begin);
      write_ulong (ifile, end);
    }
    else
    {
      /* Output body of ERR file */
      if (flag == 1)
      {
        for (i = begin; i < end; i++)
        {
          part = (int) (i / idxptr->splitsize +1);
          if (part != prevpart)
          {
            if (prevpart)
            {
              if (!(lines % 18) && lines)
                fprintf (ifile, delimit);
              lines = 0;
              fprintf (ifile, "FFF%s", delimit);
            }
            prevpart = part;
            fprintf (ifile, "%02X%s", part, delimit);
          }

          lines++; /* Number of missing or corrupted lines in this part. */
          fprintf (ifile, "%03X", (uint)(i % idxptr->splitsize));
          if (!(lines % 18) && lines)
          {
            fprintf (ifile, delimit);
            lines = 0;
          }
          else
            fprintf (ifile, " ");
        }
      }

      /* Create list of defective parts */
      if (flag == 2)
      {
        for (i = begin; i < end; i++)
        {
          part = (int) (i / idxptr->splitsize +1);
          if (part != prevpart)
          {
            prevpart = part;
            idxptr->lines_ok[4080 + (part>>5)] |= (1UL <<(part&31));
          }
        }
      }
    }
  }

  if (!flag)
  {
    write_ulong (ifile, 0xffffffffUL);
    write_ulong (ifile, idxptr->length);
  }
  else
  {
    if (flag == 1)
    {
      if (!(lines % 18) && lines)
        fprintf (ifile, delimit);
      fprintf (ifile, "FFF%s", delimit);
    }
  }
  return (0);
}

/*
*** Reading/writing unsigned long(32bit)/int(16)
***
***
 */

ulong read_ulong (FILE *in)
{
  ulong val;

  val =        (ulong) fgetc (in);
  val = val + ((ulong) fgetc (in) <<  8);
  val = val + ((ulong) fgetc (in) << 16);
  val = val + ((ulong) fgetc (in) << 24);

  return (val);
}

uint read_uint (FILE *in)
{
  uint val;

  val =        (uint) fgetc (in);
  val = val + ((uint) fgetc (in) << 8);

  return (val);
}

void write_ulong (FILE *out, ulong val)
{
  my_putc ((int) (val     &0xffUL), out);
  my_putc ((int)((val>>8 )&0xffUL), out);
  my_putc ((int)((val>>16)&0xffUL), out);
  my_putc ((int)((val>>24)&0xffUL), out);
}

void write_uint (FILE *out, uint val)
{
  my_putc ((int) (val    &0xffU), out);
  my_putc ((int)((val>>8)&0xffU), out);
}

/*
*** read a file, search for s1, calculate CRC until s2 is found.
*** flag == 1: compare calculated an read CRC.
*** flag == 0: insert CRC into file.
 */

int crc_file (const char *file, const char *s1, const char *s2, int flag)
{
  char line[81];
  const char *p;
  uint crc, cs;
  int i, j, k;
  FILE *in;

  crc = cs = 0;

  if ((in = fopen (file, OPEN_RANDOM_BINARY)) == NULLFP)
  {
    fprintf (o, cant, file);
    return (2);
  }

  i = (int) strlen (s1);
  k = (int) strlen (s2);

  j = 1;

  do
  {
    if (my_fgets (line, 80, in) == NULL)
      break;

    j = strncmp (line, s1, i);
  }
  while (j);

  if (j)
    p = s1;
  else
  {
    p = s2;
    do
    {
      register int linelen = (int) strlen (line) -1;

      for (i=0;i<linelen;i++)
        crc_calc (crc, line[i]);

      /* unfortunately, OS9/68k uses CR as line seperator. Since the CRC
         is calculated over the entire file (including the line seps),
         the resulting CRC would differ */

      crc = crctab[crc>>8] ^ (((crc&255)<<8) | 0x0a);

      j = strncmp (line, s2, k);

      if (!j)
        continue;

      if (my_fgets (line, 80, in) == NULL)
        break;
    }
    while (j);
  }

  if (j)
  {
    fprintf (o, "\n\007Can't calculate CRC\nString '%s' not found in '%s'.\n"
            "Break.\n", p, file);
    return (7);
  }

  /* evaluate CRC */
  if (flag)
  {
    my_fgets (line, 80, in);
    fclose (in);
    if ((!line) || (strncmp ("CRC ", line, 4)))
    {
      fprintf (o, "\n'%s': no CRC found.\n(File may be corrupted or from version "
              "earlier than 7PLUS v1.5)\n", file);
      return (17);
    }
    cs = get_hex (line+4);
    if (cs == crc)
      return (0);

    fprintf (o, "\007\n'%s' is corrupted.\n", file);
    return (7);
  }

  /* insert CRC into file */
  fseek (in, 0L, SEEK_CUR);
  fprintf (in, "CRC %04X", crc);
  fclose (in);

  return (0);
}

/*
*** Copy a file.
***
***
 */

int copy_file (const char *to, const char *from, ulong timestamp)
{
  FILE *_from, *_to;
  int _char, status;

  status = 0;

  _from = fopen (from, OPEN_READ_BINARY);
  _to   = fopen (to,   OPEN_WRITE_BINARY);

  while ((_char = getc (_from)) != EOF)
   if ((status = putc (BRLND_PUTC_BUG(_char &0xff), _to)) == EOF)
      break;
  fclose (_from);

  if (status != EOF)
  {
    #if (defined(__MSDOS__) || defined(__TOS__))
     if (timestamp)
       set_filetime (_to, timestamp);

     fclose (_to);
    #else
     fclose (_to);

     if (timestamp)
       set_filetime (to, timestamp);
    #endif
  }
  else
  {
    fprintf (o, "\007\nFatal error. Can't write '%s'! Break.\n", to);
    exit (1);
  }
  return (0);
}

/*
*** Replace one file with another
***
***
 */

void replace (const char *old, const char *new, ulong timestamp)
{

  if (access (old, 2))
    chmod (old, S_IREAD | S_IWRITE);
  unlink (old);
  if (rename (new, old))
  {
    copy_file (old, new, timestamp);
    unlink (new);
  }
  else
  {
    if (timestamp)
    {
     #if (defined(__MSDOS__) || defined(__TOS__))
      FILE *_file;

      _file = fopen (old, OPEN_APPEND_BINARY);
      set_filetime (_file, timestamp);
      fclose (_file);
     #else
      set_filetime (old, timestamp);
     #endif
    }
  }
}

#ifdef __MWERKS__
/*
*** Check whether "string" is the suffix of the string "name".
***    Return 1 if true, else 0.
***    Ignore case (suffix must be lower case)
***
 */

int suffixcmp (char *string, const char *name)
{
  int slen, nlen, i, j;

  slen = (int) strlen(string);
  nlen = (int) strlen(name);
  for (i = slen-1, j = nlen-1; i >= 0; i--, j--)
  {
    if (j < 0)  return (0);      /* name is shorter than string */
    if (string[i] != tolower(name[j]))  return (0);
  }
  return (1);
}

/*
*** Set filetype and filecreator based on suffix
***     when running on Mac OS
***
 */

void set_filetype (const char *name)
{
  FInfo fi;
  int i;
  short rc;

  for (i = 0; i < NSUFFIX; i++)
  {
    if (suffixcmp (suffix_table[i].suffix, name))
    {
       rc = getfinfo(name, 0, &fi);
       fi.fdType = suffix_table[i].ftype;
       fi.fdCreator = suffix_table[i].fcreator;
       rc = setfinfo(name, 0, &fi);
       break;
    }
  }
}
#endif

/*
*** Kill all files that aren't needed any more
*** (Normally, kill_em will stop erasing if more than 10 consecutive files
*** are missing. This can be overridden by the -KA option)
 */

void kill_em (const char *name, const char *inpath, const char *one,
              const char *two, const char *three, const char *four,
	      const char *five, int _one, int no_lf)
{
  const char *p;
  char newname[MAXPATH];
  int i, j, k, l, len;

  k = l = 0;

  for (i = 0; i < 5; i++)
  {
    if (!i && no_lf != 2)
      fprintf (o, "\n");

    switch (i)
    {
      case 0:  p = one;
               break;
      case 1:  p = two;
               break;
      case 2:  p = three;
               break;
      case 3:  p = four;
               break;
      case 4:  p = five;
               break;
      default: p = NULLCP;
    }
    if (!p)
     break;

    len = (int) strlen(p);
    l = 0;

    for (j = 1; j <256; j++)
    {
      if (len == 3)
        sprintf (newname, "%s%s.%s", inpath, name, p);
      else
        sprintf (newname, "%s%s.%s%02x", inpath, name, p, j);

      k++;

      if (!unlink (newname))
      {
       l = 0;
       if (!no_tty)
      {
       set_autolf(0);
#ifdef __MWERKS__
       fprintf (o, "\rDeleting: %s", newname);
#else
       fprintf (o, "Deleting: %s\r", newname);
#endif
       fflush (o);
       set_autolf(1);
      }
     }
     else
	   l++;

     if ((l > 10) && i && (autokill != 2))
      break;

     if (len == 3)
      break;

     if (!i && _one && (j == _one))
      break;
    }
   }

   if (no_tty)
    fprintf (o, "Obsolete files deleted!\n");

   if (k && no_lf != 1)
    fprintf (o, "\n");
}

/*
***
***
***
 */

void kill_dest (FILE *in, FILE *out, const char *name)
{
    if (out)
      fclose (out);
    if (in)
      fclose (in);
    if (*name)
      unlink (name);
}

/*
***
***  test if a file exists at all
***
 */

int test_exist (const char *filename)
{

  if (access (filename, 0))
    return (1);
  return (0);

/*  FILE *in;

  if ((in = fopen (filename, OPEN_READ_TEXT)) != NULLFP)
  {
    fclose (in);
    return (0);
  }
  return (1); */
}


/*
***  test if outputfile already exists. prompt for overwrite or
***  new name.
***
 */

int test_file (FILE *in, char *destnam, int flag, int namsize)
{
/* FILE *out; */
   int  i, ret;

   ret = 0;

   /* Loop as long as file with same name exists. */
   while (!access (destnam, 0))
   {
     if (noquery)
     {
       if (o == stdout)
         fprintf (o, "\007\nExisting '%s' overwritten with new file.\n", destnam);
       else
         fprintf (o, "Existing '%s' overwritten with new file.\n", destnam);
       return (ret);
     }

     if (flag > 1) /* autogenerate new filename */
     {
       char __drive[MAXDRIVE], __dir[MAXDIR], __file[MAXFILE], __ext[MAXEXT];
       char newnam[MAXPATH];
       int i = 1;
       int j, k;

       fnsplit (destnam, __drive, __dir, __file, __ext);

       if (flag == 3)
         k = 7;
       else
         k = MAXFILE-2;

       while (1==1)
       {
         sprintf (newnam, "%d", i);
         j = strlen (newnam);

         if (strlen (__file) > (size_t)(k-j))
           sprintf (newnam, "%s%s%*.*s$%d%s",
                                 __drive, __dir, k-j, k-j, __file, i, __ext);
         else
           sprintf (newnam, "%s%s%s$%d%s", __drive, __dir, __file, i, __ext);

         if (!access (newnam, 0))
         {
           /* File with new name already exists */
           i++;
           continue;
         }
         strcpy (destnam, newnam);
         return (0);
       } /* while (1==1) */
     } /* if */

     ret = 1;
     fprintf (o, "\007\nOutputfile '%s' already exists, overwrite? [y/n/a] ",
                                                                   destnam);
     fflush (o);
     do
     {
       i = getch();
       i = toupper(i);

       fflush (stdin);

       if (i == 'N')
       {
         if (flag)
         {
           #if (defined(_AMIGA_) || defined(__MWERKS__))
	    fprintf (o, "Enter new name (max %d chars)\n", namsize);
	    fprintf (o, "or press <CNTL>+C <RETURN> to break : ");
	    fflush (o);

            if (namsize == 12)
              strlwr (destnam);

            #ifdef _AMIGA_
              scanf("%s",destnam);
            #else
              gets(destnam);
            #endif

            destnam[namsize] = EOS;
           #else
	    fprintf (o, "%s\nEnter new name (max %d chars)\n", no, namsize);
	    fprintf (o, "or simply press ENTER to break : ");
	    fflush (o);

            if (namsize == 12)
              strlwr (destnam);

            i = getc(stdin);
            if(i != '\n')
            {
              ungetc(i, stdin);
              gets (destnam);
              fflush (stdin);
            }
            else
              *destnam = EOS;
            destnam[namsize] = EOS;
           #endif
         }
         else
           *destnam = EOS;

         if (!strlen (destnam))
         {
           if (!flag)
            fprintf (o, "%s\n", no);
            fprintf (o, "Break.\n");
           if (in)
             fclose (in);
           return (10);     /* Changed for Mac OS */
         }
         i = 0xff; /* indicate, that new name has been specified */
       }
     }
     while (i != 'Y' && i != 'A' && i != 0xff);

     if (i != 0xff)
     {
       if (i == 'A')
       {
         fprintf (o, "%s\n", always);
         noquery = 1;
       }
       else
	 fprintf (o, "%s\n", yes);
     }
#ifndef __MWERKS__        /* is this really necessary ? */
     fprintf (o, "\n");
#endif

/*   fclose (out); */

     if (i != 0xff)
       break;
   }

   return (ret);
}


/*
*** initialize decoding table
***
***
 */

void init_decodetab (void)
{
  register int i;
  register byte j;

  for (i = 0; i < 256; i++)
    decode[i] = 255;

  j = 0;
  for (i = 0x21; i < 0x2a; i++)
    decode[i] = j++;

  for (i = 0x2b; i < 0x7f; i++)
    decode[i] = j++;

  for (i = 0x80; i < 0x91; i++)
    decode[i] = j++;

  decode[0x92] = j++;

  for (i = 0x94; i < 0xfd; i++)
    decode[i] = j++;
}

/*
*** initialize encoding table
***
***
 */

void init_codetab (void)
{
  register byte i, j;

  j = 0;

  for (i = 0x21; i < 0x2a; i++, j++)
    code[j] = i;

  for (i = 0x2b; i < 0x7f; i++, j++)
    code[j] = i;

  for (i = 0x80; i < 0x91; i++, j++)
    code[j] = i;

  code[j++] = 0x92;

  for (i = 0x94; i < 0xfd; i++, j++)
    code[j] = i;
}

/*
*** Tnx to DC4OX.
***
*** calculate CRC-table
***
 */

void init_crctab (void)
{
  uint m, n, r, mask;

  static uint bitrmdrs[] = { 0x9188,0x48C4,0x2462,0x1231,
                             0x8108,0x4084,0x2042,0x1021 };

  for (n = 0; n < 256; ++n)
  {
    for (mask = 0x0080, r = 0, m = 0; m < 8; ++m, mask >>= 1)
      if (n & mask)
        r = bitrmdrs[m] ^ r;
    crctab[n] = r;
  }
}

/*
*** Create a MSDOS/ATARI compatible filename.
***
***
 */

void build_DOS_name (char *name, char *ext)
{

  strip (name);

  if (*ext)
    memmove (ext, ext +1, strlen (ext));

  strip (ext);

  /* truncate name and extension to 8/3 */
  name[8] = EOS;
  ext[3]  = EOS;
}

void strip (char *string)
{
  register int i;

  i = 0;

  strlwr (string);

  if (*string)
  {
    do
    {
      if (string[i] < 32 || string[i] > 126)
        string[i] = '_';

      if (strchr (" <>=,';:*?&[]{}|^()/.\\\"~+@", string[i]) != NULL)
        string[i] = '_';
    }
    while (string[++i]);

    string[i] = EOS;
  }
}

#if defined (__MSDOS__) || (__TOS__)
 /*
 *** Get file's timestamp and package it into a 32-bit word (MS_DOS-format)
 ***
 ***
  */
 ulong get_filetime (FILE *_file)
 {
  ulong    ftimestamp;

  if (getftime (fileno(_file), (struct ftime *)&ftimestamp) == EOF)
    fprintf (o, "\007\nCan't get file's timestamp!\n");

 #ifdef __TOS__
  ftimestamp = swapl(ftimestamp);
 #endif

  return (ftimestamp);
 }

 #define _SETFTIME_OK
 /*
 *** Set file's timestamp
 ***
 ***
  */
 void set_filetime (FILE *_file, ulong ftimestamp)
 {
 #ifdef __TOS__
  ftimestamp = swapl(ftimestamp);
 #endif

  if (setftime (fileno(_file), (struct ftime *)&ftimestamp) == EOF)
    fprintf (o, "\007\nCan't set file's timestamp!");
 }

#else /* it's not an MSDOS or Atari system */
 #ifndef _HAVE_GMTIME
  /*
  * mktime function from GNU C library V1.03; modified:
  * - expanded DEFUN and CONST macros from ansidecl.h
  * - inserted __isleap macro from time.h
  * - inserted __mon_lengths array and __offtime function from offtime.c
  * - inserted gmtime function from gmtime.c
  * - commented out call of localtime function
  * Be aware of the following copyright message for mktime !!!
  */

  /* Copyright (C) 1991 Free Software Foundation, Inc.
  This file is part of the GNU C Library.

  The GNU C Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  The GNU C Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with the GNU C Library; see the file COPYING.LIB.  If
  not, write to the Free Software Foundation, Inc., 675 Mass Ave,
  Cambridge, MA 02139, USA.  */


  /* How many days are in each month.  */
  const unsigned short int __mon_lengths[2][12] =
    {
      /* Normal years.  */
      { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
      /* Leap years.  */
      { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
    };

  #define  __isleap(year)  \
    ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

  #define  invalid()  return (time_t) -1


  #define  SECS_PER_HOUR  (60 * 60)
  #define  SECS_PER_DAY  (SECS_PER_HOUR * 24)

  /* Returns the `struct tm' representation of *T,
     offset OFFSET seconds east of UCT.  */
  struct tm *
  __offtime (const time_t *t, long int offset)
  {
    static struct tm tbuf;
    register long int days, rem;
    register int y;
    register const unsigned short int *ip;

    if (t == NULL)
      return NULL;

    days = *t / SECS_PER_DAY;
    rem = *t % SECS_PER_DAY;
    rem += offset;
    while (rem < 0)
    {
      rem += SECS_PER_DAY;
      --days;
    }
    while (rem >= SECS_PER_DAY)
    {
      rem -= SECS_PER_DAY;
      ++days;
    }
    tbuf.tm_hour = rem / SECS_PER_HOUR;
    rem %= SECS_PER_HOUR;
    tbuf.tm_min = rem / 60;
    tbuf.tm_sec = rem % 60;
    /* January 1, 1970 was a Thursday.  */
    tbuf.tm_wday = (4 + days) % 7;
    if (tbuf.tm_wday < 0)
      tbuf.tm_wday += 7;
    y = 1970;
    while (days >= (rem = __isleap(y) ? 366 : 365))
    {
      ++y;
      days -= rem;
    }
    while (days < 0)
    {
      --y;
      days += __isleap(y) ? 366 : 365;
    }
    tbuf.tm_year = y - 1900;
    tbuf.tm_yday = days;
    ip = __mon_lengths[__isleap(y)];
    for (y = 0; days >= ip[y]; ++y)
      days -= ip[y];
    tbuf.tm_mon = y;
    tbuf.tm_mday = days + 1;
    tbuf.tm_isdst = -1;

    return &tbuf;
  }


  /* Return the `struct tm' representation of *T in UTC.  */
  struct tm *
  gmtime (const time_t *t)
  {
    return __offtime(t, 0L);
  }
 #endif /* ifndef _HAVE_GMTIME */

 #ifndef _HAVE_MKTIME

  /* Return the `time_t' representation of TP and normalizes TP.
     Return (time_t) -1 if TP is not representable as a `time_t'.
     Note that 31 Dec 1969 23:59:59 is not representable
     because it is represented as (time_t) -1.  */
  time_t mktime (register struct tm *tp)
  {
    static struct tm min, max;
    static char init = 0;

    register time_t result;
    register time_t t;
    register int i;
    register const unsigned short *l;
    register struct tm *new;
    time_t end;

    if (tp == NULL)
    {
      errno = EINVAL;
      invalid();
    }

    if (!init)
    {
      init = 1;
      end = (time_t) LONG_MIN;
      new = gmtime(&end);
      if (new != NULL)
        min = *new;
      else
        min.tm_sec = min.tm_min = min.tm_hour =
      min.tm_mday = min.tm_mon = min.tm_year = INT_MIN;

      end = (time_t) LONG_MAX;
      new = gmtime(&end);
      if (new != NULL)
        max = *new;
      else
        max.tm_sec = max.tm_min = max.tm_hour =
      max.tm_mday = max.tm_mon = max.tm_year = INT_MAX;
    }

    /* Make all the elements of TP that we pay attention to
       be within the ranges of reasonable values for those things.  */
    #define  normalize(elt, min, max, nextelt)\
    while (tp->elt < min)                     \
    {                                         \
      --tp->nextelt;                          \
      tp->elt += max + 1;                     \
    }                                         \
    while (tp->elt > max)                     \
    {                                         \
      ++tp->nextelt;                          \
      tp->elt -= max + 1;                     \
    }

    normalize (tm_sec, 0, 59, tm_min);
    normalize (tm_min, 0, 59, tm_hour);
    normalize (tm_hour, 0, 24, tm_mday);

    /* Normalize the month first so we can use
       it to figure the range for the day.  */
    normalize (tm_mon, 0, 11, tm_year);
    normalize (tm_mday, 1, __mon_lengths[__isleap (tp->tm_year)][tp->tm_mon],
      tm_mon);

    /* Normalize the month again, since normalizing
       the day may have pushed it out of range.  */
    normalize (tm_mon, 0, 11, tm_year);

    /* Normalize the day again, because normalizing
       the month may have changed the range.  */
    normalize (tm_mday, 1, __mon_lengths[__isleap (tp->tm_year)][tp->tm_mon],
      tm_mon);

   /* Check for out-of-range values.  */
   #define  lowhigh(field, minmax, cmp)  (tp->field cmp minmax.field)
   #define  low(field)                   lowhigh(field, min, <)
   #define  high(field)                  lowhigh(field, max, >)
   #define  oor(field)                   (low(field) || high(field))
   #define  lowbound(field)              (tp->field == min.field)
   #define  highbound(field)             (tp->field == max.field)
   if (oor(tm_year))
     invalid();
   else
     if (lowbound(tm_year))
     {
       if (low(tm_mon))
         invalid();
       else
         if (lowbound(tm_mon))
         {
           if (low(tm_mday))
             invalid();
           else
             if (lowbound(tm_mday))
             {
               if (low(tm_hour))
                 invalid();
               else
                 if (lowbound(tm_hour))
                 {
                   if (low(tm_min))
                     invalid();
                   else
                     if (lowbound(tm_min))
                     {
                       if (low(tm_sec))
                       invalid();
                     }
                 }
             }
         }
     }
     else
       if (highbound(tm_year))
       {
         if (high(tm_mon))
           invalid();
         else
           if (highbound(tm_mon))
           {
             if (high(tm_mday))
               invalid();
             else
               if (highbound(tm_mday))
               {
                 if (high(tm_hour))
                   invalid();
                 else
                   if (highbound(tm_hour))
                   {
                     if (high(tm_min))
                       invalid();
                     else
                       if (highbound(tm_min))
                       {
                         if (high(tm_sec))
                         invalid();
                       }
                   }
               }
           }
       }
    t = 0;
    for (i = 1970; i > 1900 + tp->tm_year; --i)
      t -= __isleap(i) ? 366 : 365;
    for (i = 1970; i < 1900 + tp->tm_year; ++i)
      t += __isleap(i) ? 366 : 365;
    l = __mon_lengths[__isleap(1900 + tp->tm_year)];
    for (i = 0; i < tp->tm_mon; ++i)
      t += l[i];
    t += tp->tm_mday - 1;
    result = ((t * 60 * 60 * 24) +
             (tp->tm_hour * 60 * 60) +
             (tp->tm_min * 60) +
              tp->tm_sec);

    end = result;
   #if 0
    if (tp->tm_isdst < 0)
      new = localtime(&end);
    else
   #endif
      new = gmtime(&end);
    if (new == NULL)
      invalid();
    new->tm_isdst = tp->tm_isdst;
    *tp = *new;

    return result;
  }
 #endif /* ifndef _HAVE_MKTIME */

 #ifndef _FTIMEDEFINED
  /*
   * these functions have to convert a MS/DOS time to a UNIX time
   * and vice versa.
   * here comes the MS/DOS time structure
   */
  #ifdef _680X0_  /* use this struct on 680x0 systems */
   struct ftime
   {
     unsigned int ft_year  : 7; /* Year minus 1980 */
     unsigned int ft_month : 4;   /* 1..12 */
     unsigned int ft_day   : 5;   /* 1..31 */
     unsigned int ft_hour  : 5;   /* 0..23 */
     unsigned int ft_min   : 6;   /* 0..59 */
     unsigned int ft_tsec  : 5;   /* 0..59 /2 (!) */
   };
  #else  /* and this one on 80x86 systems */
   struct ftime
   {
     unsigned  ft_tsec  : 5;   /* 0..59 /2 (!) */
     unsigned  ft_min   : 6;   /* 0..59 */
     unsigned  ft_hour  : 5;   /* 0..23 */
     unsigned  ft_day   : 5;   /* 1..31 */
     unsigned  ft_month : 4;   /* 1..12 */
     unsigned  ft_year  : 7; /* Year minus 1980 */
   };
  #endif
 #endif /* _FTIMEDEFINED */

 /*
  * Get file's timestamp and package it into a 32-bit word (MS_DOS-format).
  * This function should work on any system (even on AMIGAs) :-)
  */
 ulong get_filetime (const char *filename)
 {
   struct ftime fti;
   ulong *retval = (ulong *) &fti;
   struct tm *utm;
   struct stat fst;

   *retval = 0UL;

   /* get file status */
   if (stat (filename, &fst) == 0)
   {
      /* get time of last modification and convert it to MS/DOS time */
     utm = localtime (&fst.st_mtime);

     if (utm)
     {
       #ifdef __EMX__
        /* The gmtime() implementation of EMX/GCC under OS2 already devides
           the seconds by 2.. strange!?  */
        fti.ft_tsec  = utm->tm_sec;
       #else
        fti.ft_tsec  = utm->tm_sec / 2;
       #endif
       fti.ft_min   = utm->tm_min;
       fti.ft_hour  = utm->tm_hour;
       fti.ft_day   = utm->tm_mday;
       fti.ft_month = utm->tm_mon + 1;
       fti.ft_year  = utm->tm_year - 80;

       return (*retval);
     }
   }

   /* error exit */
   fprintf (o, "\007\nCan't get file's timestamp!\n");
   return (*retval);
 }

 #ifdef _AMIGA_
  #define _SETFTIME_OK
  /*
   * Set file's timestamp
   * This function only works on AMIGA-systems
   */
  void set_filetime (const char *filename, ulong ftimestamp)
  {
    time_t atime;
    struct ftime *fti;
    struct tm utm;
    struct DateStamp fdate;

    /* convert MS/DOS ftimestamp to UNIX atime */
    fti = (struct ftime *) &ftimestamp;
    utm.tm_sec   = fti->ft_tsec * 2;
    utm.tm_min   = fti->ft_min;
    utm.tm_hour  = fti->ft_hour;
    utm.tm_mday  = fti->ft_day;
    utm.tm_mon   = fti->ft_month - 1;
    utm.tm_year  = fti->ft_year +80;
    utm.tm_wday  = utm.tm_yday  =  0;
    utm.tm_isdst = -1;
    atime = mktime (&utm);

    fdate.ds_Days = (atime/86400)-2922; /* 86400sec per Day + systimecorr.*/
    fdate.ds_Minute = (atime % 86400) / 60;
    fdate.ds_Tick = (atime % 60) * TICKS_PER_SECOND;

    SetFileDate((char *)filename,&fdate);

    return(1);
  }
 #endif /* _AMIGA_ */

 #if (defined (__unix__) || defined (__MWERKS__) || defined (OSK) || defined (__OS2__))
  #define _SETFTIME_OK

  /*
   * Set file's timestamp
   * This function only works on systems that have utime() available
   */
  void set_filetime (const char *filename, ulong ftimestamp)
  {
    time_t atime;
    struct utimbuf utim;
    struct ftime *fti;
    struct tm utm;

    /* convert MS/DOS ftimestamp to UNIX atime */
    fti = (struct ftime *) &ftimestamp;

    /* Now, setup struct utm with real data */
    utm.tm_sec   = fti->ft_tsec * 2;
    utm.tm_min   = fti->ft_min;
    utm.tm_hour  = fti->ft_hour;
    utm.tm_mday  = fti->ft_day;
    utm.tm_mon   = fti->ft_month - 1;
    utm.tm_year  = fti->ft_year + 80;
    utm.tm_wday  = utm.tm_yday  =  0;
    utm.tm_isdst = -1;

    atime = mktime(&utm);

    if (atime != -1)
    {
      /* set access time and modification time */
      utim.actime = atime;
      utim.modtime = atime;

      if (utime (filename, &utim) >= 0)
        return;
    }

    /* error exit */
    fprintf (o, "\007\nCan't set file's timestamp to: %s", ctime (&atime));
    return;
  }
 #endif /* __unix__/OSK/__OS2__ */

 #ifndef _SETFTIME_OK
  /*
   * Set file's timestamp
   *
   */
  void set_filetime (const char *filename, ulong ftimestamp)
  {
    /* error exit */
    fprintf (o, "\007\nset_filetime not (yet) implemented on this system!\n"
            "7PLUS should NOT be circulated until it is implemented!!\n"
            "Axel, DG1BBQ.\n");
    return;
  }
 #endif
#endif


/*
*** get_hex: some compilers have real big trouble when reading hex values from
***          a file with fscanf() that have leading zeros! e.g. 00A will be
***          read as two separate values (0 and A)! grr!!
***          get_hex skips all leading zeros to eliminate the problem.
***
 */

uint get_hex (char *hex)
{
  register int i = 0;
  uint   ret = 0;

  while (hex[i] == '0')
    i++;
  sscanf(hex+i, "%x", &ret);
  return (ret);
}


#ifndef _HAVE_FNSPLIT
/*
***       filenamesplit
***       (by DL1MEN, taken from SP-ST, modified for portability)
***
***       split filename up into drive, path, name and extension.
***
 */

void fnsplit(char *pth, char *dr, char *pa, char *fn, char *ft)
{
  char drv[MAXDRIVE], pat[MAXDIR], fna[MAXFILE], fty[MAXEXT], tmp[MAXPATH];
  char *p;

  strcpy(tmp,pth);

#if (defined (__MWERKS__) || defined (__linux__) || defined (__NETBSD__))
    /* Ignore drive on systems that don't have drives. */
    p = tmp;
    drv[0] = EOS;
#else
    if ((p = strchr(tmp,':')) != NULL)
    {
      *p++ = EOS;
      strcpy(drv,tmp);
    }
    else
    {
      p = tmp;
      drv[0] = EOS;
    }
#endif

  if ((pth = strrchr(p, PATHCHAR)) != NULL)
  {
    *pth++ = EOS;
    strcpy(pat,p);
  }
  else
  {
    pth = p;
    pat[0] = EOS;
  }
  if ((p = strrchr(pth,'.')) != NULL)
  {
    strcpy(fty,p);
    fty[MAXEXT-1] = EOS;
    *p = EOS;
  }
  else
    fty[0] = EOS;

  strcpy(fna,pth);
  fna[MAXFILE-1] = EOS;

  if (dr)
  {
    strcpy(dr,drv);
    if (drv[0])
      strcat(dr,":");
  }
  if (pa)
  {
    strcpy(pa,pat);
    if (pat[0])
      strcat(pa, PATHSEP);
  }
  if (fn)
    strcpy(fn,fna);
  if (ft)
   strcpy(ft,fty);
}
#endif /** ifndef _HAVE_FNSPLIT **/

#ifndef _HAVE_ICMP
/* The following functions are unfortunately not avialable on all compilers */

/*
*** strupr - convert string to upper case.
***
***
 */

char *strupr (char *string)
{
  char *strcnvt (char *string, int flag);

  return (strcnvt (string, 1));
}

/*
*** strlwr - convert string to lower case.
***
***
 */

char *strlwr (char *string)
{
  char *strcnvt (char *string, int flag);

  return (strcnvt (string, 0));
}

/*
*** strcnvt - convert string to upper (flag == 1) or lower (flag == 0) case.
***
***
 */

char *strcnvt (char *string, int flag)
{
  register int i = 0;

  while (string[i])
  {
    string[i] = (flag)?toupper (string[i]):tolower (string[i]);
    i++;
  }

  return (string);
}

/*
*** stricmp - same as strcmp(), but ignores case.
*** s1 and s2 are not modified.
***
 */

int stricmp (const char *s1, const char *s2)
{
  return (strnicmp (s1, s2, (size_t) 80));
}

/*
*** strnicmp - same as strncmp(), but ignores case.
*** s1 and s2 are not modified.
***
 */

int strnicmp (const char *s1, const char *s2, size_t n)
{
  char _s1[81], _s2[81];

  strncpy (_s1, s1, 80);
  strncpy (_s2, s2, 80);
  strupr (_s1);
  strupr (_s2);

  return (strncmp (_s1, _s2, n));
}
#endif /** ifndef _HAVE_ICMP **/

#ifndef _HAVE_GETCH

 #if defined(SYSV) || defined(__EMX__) || defined(__NetBSD__)/* use ioctl() */
  #define _IOCTL_
 #endif

#ifdef __MWERKS__
/*
*** getch - MacOS
*** Get one character from keyboard and empty keyboard buffer.
***
 */

 int getch (void)
 {
   int c;

   c = getchar();
   if (c == 0x0a)  return (c);
   while (getchar() != 0x0a);   /* anything will be ignored - wait for LF */
   return (c);
 }

#else
/*
*** getch - elsewhere
*** Disable keyboard buffering and echoing.
***
 */

 static int first = 1;

 int getch (void)
 {
   unsigned char c;
   int fd;

   fd = fileno (stdin);
   if (first)
#ifndef __linux__
   {
     first = 0;
     #ifdef _IOCTL_
     #ifdef __NetBSD__
      (void) ioctl(fd, TIOCGETA, (char *) &sg[OFF]);
     #else
      (void) ioctl(fd, TCGETA, (char *) &sg[OFF]);
     #endif
    #else
     (void) gtty(fd, &sg[OFF]);
    #endif
     sg[ON] = sg[OFF];

    #ifdef _IOCTL_
     #ifdef __EMX__
      /* IDEFAULT is a EMX/GCC extension. May not work elsewhere.. */
      sg[ON].c_lflag &= ~(IDEFAULT|ICANON|ECHO);
     #else
      sg[ON].c_lflag &= ~(ICANON|ECHO);
     #endif
     sg[ON].c_cc[VMIN] = 1;
     sg[ON].c_cc[VTIME] = 0;
    #else
     sg[ON].sg_flags &= ~(ECHO | CRMOD);
     sg[ON].sg_flags |= CBREAK;
    #endif
   }

  #ifdef _IOCTL_
   #ifdef __NetBSD__
    (void) ioctl(fd, TIOCSETAW, (char *) &sg[ON]);
   #else
    (void) ioctl(fd, TCSETAW, (char *) &sg[ON]);
   #endif
  #else
   (void) stty(fd, &sg[ON]);
  #endif
#endif /* __linux__ */

   read(fd, &c, 1);

#ifndef __linux__
  #ifdef _IOCTL_
   #ifdef __NetBSD__
    (void) ioctl(fd, TIOCSETAW, (char *) &sg[OFF]);
   #else
    (void) ioctl(fd, TCSETAW, (char *) &sg[OFF]);
   #endif
  #else
   (void) stty(fd, &sg[OFF]);
  #endif

#endif /* __linux__ */
   return (int) c;
 }

 #ifdef _IOCTL_
  #undef _IOCTL_
 #endif

#endif /** ifdef __MWERKS__ **/

#endif /** ifndef _HAVE_GETCH **/
