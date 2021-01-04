#include "7plus.h"
#include "globals.h"

/*
*** Correct meta file using either a COR or a 7PL/P?? file
***
***
 */

#ifdef __MWERKS__
 const char processing[] = "\rProcessing '%s'. Missing lines left: %ld";
#else
 const char processing[] = "Processing '%s'. Missing lines left: %ld      \r";
#endif

const char inv_idxfile[] = "\007Invalid index info.\n";

int correct_meta (char *name, int itsacor, int quietmode)
{
  FILE     *rfile, *meta;
  char     inpath[MAXFPATH], indexfile[MAXPATH], metafile[MAXPATH];
  char     newname[MAXPATH], orgname[13], filename[13], dum[20];
  char     *p, line[81];
  int      i, j, corrpart, corrline, corrline2, splitsize, length;
  int      batchcor, num;
  uint     crc, csequence;
  long     binbytes, c_line, offset, oldoffset;
  ulong    ftimestamp;

  *indexfile = EOS;
  ftimestamp = 0UL;
  splitsize = corrpart = corrline = corrline2 = num = batchcor = j = 0;
  crc = csequence = 0U;
  binbytes = offset = 0L;
  oldoffset = -62L;

  /* Isolate input-path and filename */
  fnsplit (name, _drive, _dir, _file, _ext);
  sprintf (inpath, "%s%s", _drive, _dir);
  if (*_ext)
    memmove (_ext, _ext +1, strlen (_ext));
  /*build_DOS_name (_file, _ext);*/
  strcpy (newname, name);

  if (!stricmp ("7mf", _ext))
    batchcor = 1;

  check_fn (_file);
  strlwr (_file);

  sprintf (metafile , "%s%s.7mf", genpath, _file);

  #ifndef _HAVE_CHSIZE
   sprintf (indexfile, "%s%s.7ix", genpath, _file);

   /* Open index file */
   if ((meta = fopen (indexfile, OPEN_READ_BINARY)) == NULLFP)
   {
     fprintf (o, cant, indexfile);
     return (2);
   }
   /* read index info into struct idxptr */
   if (read_index (meta, idxptr))
   {
     fprintf (o, inv_idxfile);
     fclose (meta);
     return (7);
   }
   fclose (meta);
  #endif

  /* Open meta file (*.7mf) */
  if ((meta = fopen (metafile, OPEN_RANDOM_BINARY)) == NULLFP)
  {
    fprintf (o, cant, metafile);
    return (2);
  }

  #ifdef _HAVE_CHSIZE
   strcpy (indexfile, metafile);

   /* read index info into struct idxptr */
   if (read_index (meta, idxptr))
   {
     fprintf (o, inv_idxfile);
     fclose (meta);
     return (7);
   }
  #endif

  /* Create list of defective parts */
  if (batchcor)
    write_index (NULLFP, idxptr, 2);

  if (quietmode != 2)
    fprintf (o, "\n-------------\n"
                  "Correcting...\n"
                  "-------------\n\n");

  while (idxptr->lines_left)
  {
    if (itsacor)
    {
      if (num)
      {
        if (stricmp( _ext, "cor") && num == 1)
          break;

        if (num == 256 || (j > 9))
          break;

        sprintf (newname, "%s%s%s.c%02x", _drive, _dir, _file, num);
        if (test_exist (newname))
        {
          j++;
          continue;
        }
        j = 0;
      }
      num++;

      if ((i = crc_file (newname, "7PLUS corr", " P00:\n", 1)) != 0)
      {
        if (i == 7)
          continue;

        if (i != 17)
        {
          fclose (meta);
          return (i);
        }
        if (!force)
        {
          fprintf (o,
            "\n\007If you want to use this cor-file anyway, run 7PLUS "
                  "again\nwith the '-F' option (force usage). Bear in mind, "
                  "that this can\ncause irreparable damage to the metafile! "
                  "Use at own risk.\n");

          fclose (meta);
          return(7);
        }
      }
    }

    if (batchcor)
    {
      /* Find out which 7PLUS parts are required for the correction */
      for (; num < 256; num++)
      {
        if (idxptr->lines_ok[4080 + (num>>5)] & (1UL <<(num & 31)))
        {
          sprintf (newname, "%s%s%s.p%02x" , _drive, _dir, _file, num);
          if (!test_exist (newname))
          {
            /* Use batchcor to indicate that at least one file was found */
            if (batchcor == 1)
              batchcor++;
            num++;
            break;
          }
        }
      }
      if (num == 256)
        break;
    }

    /* Open COR-file */
    if ((rfile = fopen (newname, OPEN_READ_BINARY)) == NULLFP)
    {
      fprintf (o, cant, newname);
      return (2);
    }

    /* Find first line */
    while ((p = my_fgets (line, 80, rfile)) != NULLCP)
    {
      if (!strncmp (line, "7PLUS correction:", 17) && itsacor)
        break;
      if (!strncmp (line, " go_7+.", 7) && !itsacor)
        break;
    }
    /* if p == NULLCP) file has been completely read without finding
       the start of the COR info */
    if (!p)
    {
      fprintf (o, "\007\n'%s': invalid correction file. Break.\n", newname);
      fclose (rfile);
      continue;
    }

    if (itsacor)
    {
      /* Get info from COR-file */
      sscanf (line+18, "%12s %ld %s [%lx]",
                                    orgname, &binbytes, filename, &ftimestamp);
      splitsize = get_hex (filename);
    }
    else
    {
      if (!mcrc (line, 0))
        rebuild (line, 2);

      /* Get info from 7PLUS header */
      if(sscanf (line+8, "%d %s %s %s %ld %s %s %s %s %s",
                  &corrpart, indexfile, indexfile, orgname, &binbytes,
                  indexfile, dum, indexfile, indexfile, indexfile) == 10)
      {
        if (!mcrc(line, 0) || !corrpart || !dum)
          *orgname = EOS;

        splitsize = get_hex (dum);
        strlwr (orgname);
      }
      else
       *orgname = EOS;

      if (!*orgname)
      {
        fprintf (o, "\007\n'%s': Header is corrupted. Break.\n", newname);
        fclose (rfile);
        break;
      }
      else
      {
        if (corrpart == 1 && indexfile[3] == '*')
        {
          my_fgets (line, 80, rfile);
          if (mcrc (line, 0))
            sscanf (line, "/%60[^/]", idxptr->full_name);
        }
      }

      offset = ftell (rfile);
      fseek (rfile, -72L, SEEK_END);

      while ((p = my_fgets (line, 80, rfile)) != NULLCP)
        if (!strncmp (line, " stop_7+.", 9))
          break;

      ftimestamp = 0UL;

      if (p)
      {
        /* Get timespamp */
        if (strchr (line, '['))
        {
          if (!mcrc (line, 0))
            rebuild (line, 2);

          if (mcrc (line, 0))
            if (sscanf (line, " stop_7+. %s [%lx]",
                indexfile, &ftimestamp) != 2)
              ftimestamp = 0UL;
        }
      }

      fseek (rfile, offset, SEEK_SET);
    }

    if (stricmp (orgname, idxptr->filename) ||
        (binbytes && (binbytes != idxptr->length)))
    {
      fprintf (o, "\007\nCorrection file '%s.%s' and metafile '%s' do not "
              "refer\nto the same original file!\n", _file, _ext, metafile);
      fclose (rfile);
      continue;
    }
    if ((ftimestamp && idxptr->timestamp && (ftimestamp != idxptr->timestamp))
         && !force)
    {
      fprintf (o,
          "\007WARNING! The timestamps in the metafile and the correction "
              "file\n'%s.%s' differ!\nIf you still want to go ahead with the "
              "correction, call 7PLUS again\nwith the addition of the '-f' "
              "option (force usage).\nBear in mind, that this can cause "
              "irreparable damage to the metafile!\nUse at own risk.\n",
                                                                  _file, _ext);
      return (18);
    }

    if (!no_tty)
    {
      set_autolf(0);
      fprintf (o, processing, newname, idxptr->lines_left);
      fflush (o);
      set_autolf(1);
    }

    if (!splitsize)
      splitsize = idxptr->splitsize;

    while (1==1)
    {
      if ((p = my_fgets (line, 80, rfile)) == NULLCP)
        break;

      if (itsacor)
      {
        /* Read part and line number from COR file */
        if (p[0] == ' ' && p[1] == 'P')
        {
          if ((corrpart = get_hex (p+2)) == 0)
            break;
          my_fgets (p, 80, rfile);
          if (corrpart > 255)
            continue;
        }
        corrline = get_hex (p+2);
        my_fgets (p, 80, rfile);
        if (corrline > 511)
          continue;
      }

      /* Calculate CRC */
      csequence = 0;
      for (i=0; i<64; i++)
        crc_calc (csequence, p[i]);
      csequence &= 0x3fff; /* strip calculated CRC to 14 bits */

      /* If line is corrupted, try repairing it */
      if (csequence != crc)
      {
        if (!rebuild (p, 0))
          /* Incorrect CRC. Ignore line. */
          continue;
      }

      /* Read CRC and line number from (corrected) code line */
      crc_n_lnum (&crc, &corrline2, p);

      /*
      ** If it's not a COR, but a Pxx, then we have to trust the line number
      ** in the code line
      */
      if (!itsacor)
        corrline = corrline2;

      /* Calculated the absolute line number */
      c_line = ((long) (corrpart-1) * splitsize) + corrline;

      /* Check, if that line is needed */
      if (!(idxptr->lines_ok[(int)(c_line>>5)] & (1UL <<(c_line&31L))))
        continue;

      /* Is it really the right line? */
      if (corrline2 != corrline)
        continue;

      /* Calculate offset to metafile and position the r/w pointer there */
      offset = (long) c_line * 62UL;
      if (offset != (oldoffset + 62L) || !offset)
        fseek (meta, offset, SEEK_SET);
      oldoffset = offset;

      /* Calculate number of valid bytes in the line */
      length = 62;
      if (c_line == ((idxptr->length +61) /62)-1)
      {
        length = (int) (idxptr->length % 62);
        if (!length)
          length = 62;
      }

      /* Decode & insert the line into the metafile */
      decode_n_write (meta, p, length);

      /* Mark line as present */
      idxptr->lines_ok[(int)(c_line>>5)] &=
                                         (0xffffffffUL - (1UL <<(c_line&31L)));
      idxptr->lines_left--;

      if (!no_tty)
      {
        set_autolf(0);
        if (idxptr->lines_left %10 == 0)
          fprintf (o, processing, newname, idxptr->lines_left);
        fflush (o);
        set_autolf(1);
      }
    }

    set_autolf(0);
    fprintf (o, processing, newname, idxptr->lines_left);
    fflush (o);
    set_autolf(1);

    if (rfile)
      fclose (rfile);

    if (autokill && batchcor)
      unlink (newname);

    if (!itsacor && !batchcor)
      break;
  }

  if (batchcor == 1 && quietmode != 1)
    fprintf (o, "None of the required files have been found.\n");

  if (quietmode != 1)
    fprintf (o, "\n");

  #ifdef _HAVE_CHSIZE
   if (!idxptr->lines_left)
   {
     fseek (meta, 0L, SEEK_SET);
     chsize (fileno(meta), idxptr->length);
   }
  #endif
  fclose (meta);

#if (defined (__MSDOS__) || defined (__TOS__) || defined (__OS2__))
  #if defined (__BORLANDC__) && (__WIN32__)
   p = idxptr->full_name;
  #else
   p = idxptr->filename;
  #endif
#else
  p = idxptr->full_name;
#endif
  check_fn (p);

  if (idxptr->lines_left)
  {
    if (autokill)
    {
      if (itsacor)
        kill_em (_file, inpath, "cor", "c", NULL, NULL, NULL, 0, 0);
      else
        kill_em (_file, inpath, "7pl", "p", NULL, NULL, NULL, 0, 0);
    }
    w_index_err (idxptr, _file, 0);
    if (quietmode != 1)
      fprintf (o, "\nCorrection of '%s' not successful.\n", p);

    if ((no_tty || batchcor == 1) && quietmode != 1)
      fprintf (o, "\n%ld corrupted line%s left.\n", idxptr->lines_left,
                                               (idxptr->lines_left==1)?"":"s");
    return (16);
  }

  sprintf (newname, "%s%s", genpath, p);

  if (test_file (NULLFP, newname, 2, MAXFNAME-1) == 10)
    return (10);
  replace (newname, metafile, idxptr->timestamp);
#ifdef __MWERKS__
  set_filetype (newname);
#endif

  #ifndef _HAVE_CHSIZE
   sprintf (indexfile, "%s%s.7ix", genpath, _file);
   unlink (indexfile);
  #endif

  if (autokill)
  {
    kill_em (_file, inpath, "7pl", "p", "cor", "c", "err", 0, 1);
    kill_em (_file, inpath, "e", NULL, NULL, NULL, NULL, 0, 2);
  }
  fprintf (o, "\nCorrection successful! '%s', %ld bytes.\n",
                                                     newname, idxptr->length);

  return (0);
}

