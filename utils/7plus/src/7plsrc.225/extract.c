#include "7plus.h"
#include "globals.h"

/*
*** extract 7plus-files from log-file.
***
***
 */

const char list_top[] = "File         Length Lines\n"
                        "------------ ------ -----\n";

int extract_files (char *name, char *search)
{
  FILE *in, *out;
  char string[81], destnam[13], writenam[MAXPATH], dummi[20], *p, *q;

  int  i, part, file, /*filen,*/ err, errn, cor, corn, ret, lines, info, offset;
  ulong bytes, sum;

  out = NULLFP;
  file = /*filen =*/ err = errn = cor = corn = ret = lines = offset = 0;
  info = 1;
  bytes = sum = 0UL;

  if (search)
    strlwr (search);

  fprintf (o, "\n--------------------\n"
                "7PLUS file extractor\n"
                "--------------------\n");

  if ((in = fopen (name, OPEN_READ_BINARY)) == NULLFP)
  {
    fprintf (o, cant, name);
    return (2);
  }
  setvbuf (in, NULL, _IOFBF, buflen);

  fprintf (o, "\nScanning '%s' for 7PLUS-files.\n\n%s", name, list_top);

  while ((p = my_fgets (string, 80, in)) != NULL)
  {
    offset = 0;

    /* This is necessary, because of some strange BBS in the UK that keeps
       stripping the space in the first line. */
    if (!strncmp (p, " go_", 4))
      offset = 4;
/*    if (!strncmp (p, "go_", 3))
      offset = 3;
*/

    if (offset)
    { /* Beginning of a 7PLUS file found. */
      if (out)
      {
        fprintf (o, "%6lu %5d -\n", bytes, lines);
        sum += bytes;
        fclose (out);
        out = NULLFP;
      }
      *destnam = EOS;
      if (!strncmp (p+offset, "7+.", 3)) /* It's a code file */
      {
        /* Get filename from header. Create output filename. */
        sscanf (p+offset+3, "%d%s%s%s", &part, dummi, dummi, destnam);
        if ((q = strrchr (destnam, '.')) != NULL)
          *q = EOS;
        destnam[8] = EOS;
        if (strstr (p, "of 001"))
          sprintf (dummi, ".7PL");
        else
          sprintf (dummi, ".P%02x", part);
	strcat (destnam, dummi);
      }
      /* OK, then it could be an ERR or COR file.
         Careful! It could also be a marked textfile */
      if (!strncmp (p+offset, "text.", 5) &&
          (strstr (p, ".ERR") || strstr (p, ".COR")))
        sscanf (p+offset+6, "%12s", destnam);

      /* It could also be an info file accompanying the code file */
      if (!strncmp (p+offset, "info.", 5))
      {
        sscanf (p+offset+6, "%12s", destnam);
        info = 0;
      }
      strlwr (destnam);
      fnsplit (destnam, _drive, _dir, _file, _ext);
      build_DOS_name (_file, _ext);
      sprintf (destnam, "%s.%s", _file, _ext);
      check_fn (destnam);

      err = cor = 0;
      if (strstr (p, ".ERR"))
        err = 1;
      if (strstr (p, ".COR"))
        cor = 1;

      if (search && *destnam && !strstr (destnam, search))
      {
	fprintf (o, "%-12s ------ -----\n", destnam);
        *destnam = EOS;
      }
      if (*destnam) /* Open output file if 7PLUS file found. */
      {
	sprintf (writenam, "%s%s", genpath, destnam);

	/* create filename for output file */
	if (err && !test_exist (writenam))
	{
	  errn = 1;
	  do
	  {
	    if ((q = strrchr (destnam, '.')) != NULL)
	      *q = EOS;
	    sprintf (dummi, ".e%02x", errn++);
	    strcat (destnam, dummi);
	    sprintf (writenam, "%s%s", genpath, destnam);
	  }
	  while (!test_exist (writenam));
	}

	if (cor && !test_exist (writenam))
	{
	  corn = 1;
	  do
	  {
	    if ((q = strrchr (destnam, '.')) != NULL)
	      *q = EOS;
	    sprintf (dummi, ".c%02x", corn++);
	    strcat (destnam, dummi);
	    sprintf (writenam, "%s%s", genpath, destnam);
	  }
	  while (!test_exist (writenam));
	}

/*        if (!err && !cor && !test_exist (writenam))
        {
          filen = 0;
	  do
          {
            sprintf (dummi, "%d", ++filen);
            if ((strlen (_file) + strlen (dummi)) > 8)
            {
              strcpy (destnam, _file);
              strcpy (destnam + 8 - strlen (dummi), dummi);
              strcat (destnam, ".");
              strcat (destnam, _ext);
            }
            else
              sprintf (destnam, "%s%d.%s", _file, filen, _ext);
	    sprintf (writenam, "%s%s", genpath, destnam);
	  }
	  while (!test_exist (writenam));
        }
*/
        file++;

        i = test_file (in, writenam, 3, 12);
        if (i == 10)
          return (10);
        else
          if (i)
            fprintf (o, list_top);

        fprintf (o, "%-12s ", destnam);
        out = fopen (writenam, OPEN_WRITE_TEXT);
        #ifndef _AMIGA_
          setvbuf (out, NULL, _IOFBF, buflen);
        #endif
        bytes = 0UL;
        lines = 0;
      }
    }

    if (out)
    {
      /* End of file reached? */
      if (!strncmp (p, " stop_", 6))
      {
        info = 2; /* yes */
        if (!strncmp (p+6, "info.", 5))
        {
          lines--;
          *p = EOS;
        }
      }

      if (info)
      {
        if (offset == 3)
          fprintf (out, " ");

        ret = fprintf (out, "%s", p);

        lines++;

        if (ret == EOF)
        {
	  fprintf (o, "\007\nWrite error. Can't continue. Break.\n");
          exit (1);
        }

        #ifdef TWO_CHAR_SEP
         if (p[(int)ret-1] == '\n')
           ret++;
        #endif
        bytes += (ulong) ret;

        if (info == 2)
        {
	  fprintf (o, "%6lu %5d\n", bytes, lines);
          sum += bytes;
          fclose (out);
          out = NULLFP;
        }
      }
    }
    info = 1;
  }

  if (file)
  {
    fprintf (o, "------------ ====== -----\n"
		"      Total: %6lu\n", sum);
    fprintf (o, "\nAll done!\n");
  }
  else
    fprintf (o, "No %sfiles found....\n", search?"matching ":"");

  fclose (in);
  if (out)
    fclose (out);
  return (0);
}
