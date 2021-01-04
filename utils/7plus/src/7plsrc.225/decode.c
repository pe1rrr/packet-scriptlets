#include "7plus.h"
#include "globals.h"

/*
*** First decode. If there already are CORs for that file, try correcting
*** afterwards.
***
 */
int control_decode (char *name)
{
  int i, j, cor_exists;
  char newname[MAXPATH];
  FILE *out;
  char filename[MAXPATH];

  j = cor_exists = 0;
  *_ext = EOS;

  fnsplit (name, _drive, _dir, _file, _ext);
  sprintf (newname, "%s%s%s.cor", _drive, _dir, _file);

  if (!test_exist (newname))
    cor_exists = 1;

  i = decode_file (name, cor_exists);

  /* Meta file already present, try if correction can be done using regular
     7PLUS parts */
  if (i == 19)
  {
    sprintf (newname, "%s%s%s.7mf", _drive, _dir, _file);
    j = correct_meta (newname, 0, cor_exists?1:0);
    if (j == 16)
      i = 11;
    else
      i = j;
  }

  if (i == 11 && cor_exists)
  {
    sprintf (newname, "%s%s%s.cor", _drive, _dir, _file);
    i = correct_meta (newname, 1, j==16?2:0);
  }

  /* write 7plus.fls (for server use) */
  sprintf (filename, "%s"_7PLUS_FLS, genpath);
  if (!i && fls)
  {
    if ((out = fopen (filename, OPEN_WRITE_TEXT)) == NULLFP)
      return (14);
    check_fn (idxptr->full_name)
    fprintf (out, "%s %s\n", idxptr->filename, idxptr->full_name);
    fclose (out);
  }
  else
    unlink (filename);

  return (i);
}

/*
*** decode a file. create error report, if errors detected.
***
***
 */
const char decoding[] = "decoding...";
const char *rebuilding[] = { "### rebuilding a line ###",
                             "*** rebuilding a line ***" };

int decode_file (char *name, int flag)
{
  FILE     *in, *out;
  int      part, _part, parts, _parts, _parts0;
  int      c_line, c_line2, c_line3, f_lines, blocklines;
  int      defect, rest, length, hcorrupted, ignored;
  uint     csequence, crc;
  long     binbytes, _binbytes, lines, rebuilt, k, line;
  ulong    ftimestamp;
  char     rline[81], *p, dummi[20], dummi2[81];
  char     inpath[MAXFPATH], indexfile[MAXPATH], metafile[MAXPATH];
  char     filename[13], srcname[MAXPATH], orgname[MAXFNAME];
  char     orgname2[66], destname[13], orgdestname[13];
  register int i, j;

  *orgname = EOS;
  out = NULLFP;
  hcorrupted = ignored = part = blocklines = parts = 0;
  ftimestamp = 0UL;
  binbytes = 0L;

  /* Isolate input-path and filename */
  fnsplit (name, _drive, _dir, _file, _ext);
  sprintf (inpath, "%s%s", _drive, _dir);
  if (*_ext)
    memmove (_ext, _ext +1, strlen (_ext));
  /*build_DOS_name (_file, _ext);*/

  /* Make up names for the meta- and indexfile */
  sprintf (metafile,  "%s%s.7mf", genpath, _file);
  sprintf (indexfile, "%s%s.7ix", genpath, _file);

  if (!test_exist (metafile))
    return (19);

  /* Initialize index-info */
  *idxptr->filename  = *idxptr->full_name  = EOS;
  idxptr->length     = idxptr->timestamp   = 0UL;
  idxptr->splitsize  = 0;
  for (i=0;i<4080;i++)
    idxptr->lines_ok[i] = 0UL;

  if (sysop)
  {
    parts = 257;
    if (*_ext)
    {
      if (sscanf (_ext, "p%x", &part) == 1)
      {
        sprintf (srcname, "%s%s.%s", inpath, _file, _ext);
        if (test_exist (srcname))
        {
          fprintf (o, cant, srcname);
          return (2);
        }
      }
      else
        part = 0;
    }
    else
    {
      part = 1;
      for (part = 1; part < 256; part++)
      {
        sprintf (srcname, "%s%s.p%02x",  inpath, _file, part);
        if (!test_exist (srcname))
          break;
      }
      if (part == 256)
        part = 0;
    }
  }

  if (!part)
  {
    part = 1;

    if (*_ext)
    {
      parts = 1;
      if (!stricmp (_ext, "p01"))
        parts = 2;
      sprintf (srcname, "%s%s.%s", inpath, _file, _ext);
      if (test_exist (srcname))
      {
        fprintf (o, cant, srcname);
        return (2);
      }
    }
    else
    {
      /* Find out, if it's a split file */
      parts = 2;
      sprintf (srcname, "%s%s.p01", inpath, _file);
      if (test_exist (srcname))
      {
        parts = 1;
        sprintf (srcname, "%s%s.7pl", inpath, _file);
        if (test_exist (srcname))
        {
          sprintf (srcname, "%s.7pl or %s.p01", _file, _file);
          fprintf (o, cant, srcname);
          return (2);
        }
      }
    }
  }
  /* Open input file */
  in = fopen (srcname, OPEN_READ_BINARY);

  /* Set I/O-buffering */
  setvbuf (in, NULL, _IOFBF, buflen);

  fprintf (o, "\n-----------\n"
                "Decoding...\n"
                "-----------\n\n");

  defect = _parts0 = rest = length = 0;
  lines = rebuilt = 0L;

  /* Loop for number of parts. */
  for (; part < parts +1; part++)
  {
    if (part == 256)
    {
      fprintf (o, "\007\nMore than 255 parts not allowed. Break.\n");
      kill_dest (in, out, metafile);
      return (8);
    }

    /* If more than 1 part, generate filename for messages and handling. */
    if (parts == 1)
      sprintf (filename, "%s.7pl", _file);
    else
      sprintf (filename, "%s.p%02x", _file, part);

    /* If we're already at part > 1, generate filename for next part. */
    if (part != 1 && parts != 257)
    {
      sprintf (srcname, "%s%s", inpath, filename);
      if ((in = fopen (srcname, OPEN_READ_BINARY)) == NULLFP)
      {
        if (sysop != 2)
        {
          fprintf (o, "\007\n'%s': Not found. Break.\n"
                  "\nYou must have all parts to be able to decode!\n"
                    "              ===\n"
                  "Get the missing files and try again.\n", srcname);

          kill_dest (in, out, metafile);
          return (2);
        }
        else
        {
          j = blocklines;
          line = (long) blocklines * 62;
          if (part == parts)
          {

            j = (int) (((binbytes + 61) % line) /62);

            if (!j)
             j = blocklines;

            line = binbytes % (blocklines * 62);
            if (!line)
              line = (long) blocklines * 62;
          }

          lines += j;

          for (; line; line--)
            my_putc ( 0, out);

          line = (long)(part-1) * blocklines;
          for (i = 0; i < j; i++, line++)
            idxptr->lines_ok[(int)(line>>5)] += 1UL << (int)(line&31);
          defect = 1;

          continue;
        }
      }
      setvbuf (in, NULL, _IOFBF, buflen);
    }

    /* Read, until starting line is found. */
    while ((p = my_fgets (rline, 80, in)) != NULL)
    {
      if (!strncmp (rline, " go_7+. ", 7))
        break;
    }
    /* p == NULL? then no starting line found. File no good. */
    if (!p)
    {
      fprintf (o, "\007'%s': 7PLUS-startline ", filename);
      fprintf (o, "not found. Break.\n");
      kill_dest (in, out, metafile);
      return (3);
    }

    if (!mcrc (rline, 0))
      rebuild (rline, 1);

    /* Check if file went trough 7bit channel */
    if (!strstr (rline, "\xb0\xb1"))
    {
      fprintf (o, "\007\n'%s':\nBit 8 has been stripped! Can't decode.\nPlease "
              "check all settings of your terminal and tnc regarding 8 bit "
              "transfer.\nYou will have to re-read '%s' from the mailbox\n"
              "after having corrected the settings.\n", filename, filename);
      kill_dest (in, out, metafile);
      return (9);
    }

    /* Get info from 7PLUS header */
    if(sscanf (rline+8, "%d %s %d %s %ld %s %s %s %s %s",
            &_part, dummi, &_parts, destname,
            &binbytes, dummi, dummi2, dummi, dummi, dummi) != 10)
      hcorrupted = 1;
    blocklines = get_hex (dummi2);

    if (!blocklines || !_part || !_parts || !binbytes)
    {
      fprintf (o, "'%s': Header is corrupted. Can't continue.\n", filename);
      kill_dest (in, out, metafile);
      return (5);
    }

    strlwr (destname);                  /* Convert to lower case */
    fnsplit (destname, dummi, dummi, orgdestname, dummi2);
    sprintf (destname, "%s%s", orgdestname, dummi2);
    strcpy (orgdestname, destname);
    check_fn (destname);

    /* Set number of lines in this file */
    f_lines = blocklines;
    if (_part == _parts)
      f_lines = (int) (((binbytes + 61) / 62) % blocklines);
    if (!f_lines)
      f_lines = blocklines;
    f_lines--;

    if (!mcrc(rline, 0))
      hcorrupted = 1;

    if (!hcorrupted)
    {
      if (_part == 1 || sysop == 1)
      {
        _parts0  = _parts;
        strcpy (idxptr->filename, orgdestname);
        idxptr->length  = binbytes;
        idxptr->splitsize = blocklines;
      }

      if (_parts0 != _parts        || stricmp(idxptr->filename, orgdestname) ||
          idxptr->length != binbytes || idxptr->splitsize != blocklines)
        hcorrupted = 1;
    }
    if (hcorrupted)
    {
      fprintf (o, "'%s': Header is corrupted. Can't continue.\n", filename);
      kill_dest (in, out, metafile);
      return(5);
    }
    /* If first part, process filename, calculate how many valid binary bytes
       are contained in last code line of last part, initialize index-info. */
    if (_part == 1 || sysop == 1)
    {

      *orgname2 = EOS;

      if (dummi[3] == '*' && _part == 1)
      {
        my_fgets (rline, 80, in);
        if (!mcrc (rline, 0))
        {
          fprintf (o, "\nExtended Filename corrupted. "
                  "Using filename from header.\n");
          strcpy (orgname2, idxptr->filename);
          check_fn (orgname2);
        }
        else
          sscanf (rline, "/%60[^/]", orgname2);
      }

      strcpy (orgname, idxptr->filename);
      if (!*orgname2)
        strcpy (orgname2, orgname);
      strcpy (idxptr->full_name, orgname2);

      if (_extended == '*' && *orgname2)
        strncpy (orgname, orgname2, (size_t)(MAXFNAME-1));
      check_fn (orgname);

      rest = (int) (binbytes % 62);
      if (!rest)
        rest = 62;
      parts = _parts; /* Set number of parts to decode. */
    }

    /* Current file does not contain expected part */
    if (_part != part)
    {
      fprintf (o, "\007'%s': File does not contain part %03d. Break.\n",
                                                               filename, part);
      kill_dest (in, out, metafile);
      return (4);
    }

    /* If first part, open metafile for writing. */
    if (part == 1 || sysop == 1)
    {
      if (sysop)
        sysop++;
      out = fopen (metafile, OPEN_WRITE_BINARY);
      setvbuf (out, NULL, _IOFBF, buflen); /* As always, bufferize */

      if (!no_tty)
        fprintf (o, "File         Pt# of# Errors Rebuilt   Status\n");

      lines = 0;

      if (part != 1)
      {
        for (line=0; line < (long) (part -1) * blocklines *62; line++)
          my_putc (0, out);
        line = 0L;
        for (k = 0; k < (long) (part -1) * blocklines; k++, line++)
          idxptr->lines_ok[(int)(line>>5)] += 1UL << (int)(line&31);
        lines = (long) (part -1) * blocklines;
        defect = 1;
      }
      progress (filename, part, parts, lines, 0, decoding);
    }

    c_line = c_line2 = -1;

    /* Now decode this part */
    do
    {
      /* Get a line from code file */
      p = my_fgets (rline, 80, in);

      /* If line starts with a space, check if it's the last line */
      if (p && *rline == ' ')
        if (!strncmp (rline, " stop_7+.", 8))
        {
          /* Get timestamp */
          if (!ftimestamp && strchr (rline, '['))
          {
            if (!mcrc (rline, 0))
            {
              progress (filename, part, parts, lines, rebuilt, rebuilding[0]);
              rebuild (rline, 2);
              progress (filename, part, parts, lines, rebuilt, decoding);
            }
            if (mcrc (rline, 0))
            {
              char dummy[30];

              if (sscanf (rline, " stop_7+. %s [%lx]",
                                                     dummy, &ftimestamp) != 2)
                ftimestamp = 0UL;
            }
            idxptr->timestamp = ftimestamp;
          }
          p = NULL; /* Last line, set end indicator */
        }
      if (p)
      {
        /* Calculate CRC */
        csequence = 0;
        for (i=0; i<64; i++)
          crc_calc (csequence, p[i]);
        csequence &= 0x3fff; /* strip calculated CRC to 14 bits */

        c_line3 = c_line;
        /* Get crc from code line */
        crc_n_lnum (&crc, &c_line, p);

        if (csequence != crc)
        {
          if (rebuilt &1)
            progress (filename, part, parts, lines, rebuilt, rebuilding[1]);
          else
            progress (filename, part, parts, lines, rebuilt, rebuilding[0]);

          if (!rebuild (p, 0))
          {
            ignored++; /* Incorrect CRC. Ignore line. */
            c_line = c_line3;
            continue;
          }

          rebuilt++;
          progress (filename, part, parts, lines, rebuilt, decoding);
          crc_n_lnum (&crc, &c_line, p);
        }

        /* Number of valid binary bytes in this line. If it's the last line
           of the last part, set it to the number precalculated earlier */
        length = 62;
        if (c_line == f_lines && part == parts)
          length = rest;
      }

      /* If file ends prematurely, set current line number to number of
         lines in this part, so that the missing lines can be protocolled. */
      if (!p && f_lines != c_line)
        c_line = f_lines+2;

      /* If current line number is greater than previous one -> ok */
      if (c_line > c_line2)
      {
        /* Difference is greater than 1, then line(s) must be missing. */
        if (c_line2 != c_line-1)
        {
          defect = 1;

          /* Loop for number of missing or corrupted lines */
          for (i = c_line2+1; i < c_line; i++)
          {
            progress (filename, part, parts, lines, rebuilt, decoding);
            lines++; /* Number of missing or corrupted lines. */

            j = 62;
            if (i == f_lines && part == parts)
              j = rest;

            line = (long)(part-1) * idxptr->splitsize +i;
            idxptr->lines_ok[(int)(line>>5)] += 1UL << (int)(line&31);

            /* Write fill-bytes into metafile */
            for (;j;j--)
              my_putc ( 0, out);

            if (i == f_lines)
            {
              length = 0;
              break;
            }
          }
        }
        decode_n_write (out, rline, length);

        c_line2 = c_line; /* Memorize current line number */
      }
    }
    while (p); /* Loop until current code file ends */

    progress (filename, part, parts, lines, rebuilt, decoding);

    fclose (in);
  }
  progress (filename, part-1, parts, lines, rebuilt, "done...");

#ifdef __MWERKS__
  if (!no_tty)
    fprintf (o, "\n");
#endif

  idxptr->lines_left = lines;

  /* Get size of metafile */
  _binbytes = ftell (out);

  if (out)
    fclose (out);

  if (defect) /* write index-file and error report */
    w_index_err (idxptr, _file, 0);
  else
  {
    if (_binbytes == binbytes)
    {
      sprintf (srcname, "%s%s", genpath, orgname);
      if (test_file (NULLFP, srcname, 2, MAXFNAME-1) == 10)
        return (10);
      replace (srcname, metafile, ftimestamp);
#ifdef __MWERKS__
      set_filetype (srcname);
#endif

#ifndef __MWERKS__
      if (!no_tty)
        fprintf (o, "\n");
#endif

      if (autokill)
        kill_em (_file, inpath, (parts==1)?"7pl":"p",
           "cor", "c", "err", "e", parts, 0);

      fprintf (o, "\nDecoding successful! '%s', %ld bytes.\n", srcname, binbytes);

      return (0);
    }
  }

#ifndef __MWERKS__
  if (!no_tty)
    fprintf (o, "\n");
#endif

  if (!flag || no_tty)
  {
    fprintf (o, "\nDecoding of '%s' not successful.\n", orgname);

    if (no_tty)
      fprintf (o, "\n%ld line%s corrupted, %ld line%s rebuilt.\n",
                     lines, (lines==1)?"":"s" , rebuilt, (rebuilt==1)?"":"s");

    if ((idxptr->lines_left > (idxptr->length/620L)) && !sysop)
      fprintf (o,
           "\nWARNING:\n"
             "========\n"
             "More than 10%% of all lines are corrupted! Are you sure, your "
             "communications\nprogramm is set correctly to handle 7PLUS files "
             "(character conversion ect..)?\nMaybe you didn't get parts of "
             "the files because of link failures?\nOf course, the cause may "
             "lie with the originating source...\n\n");
  }
  if (_binbytes != binbytes)
  {
    fprintf (o, "\nDecoded file has wrong length! Disk full?\n"
                  "This error should never have occured.....I hoped...\n");
    return (1);
  }
  else
    if (autokill)
      kill_em (_file, inpath, (parts==1)?"7pl":"p",
                                            NULL, NULL, NULL, NULL, parts, 0);

  return (11);
}


/*
***
*** split up longs into 2 * 31 binary bytes and write to file.
***
 */
void decode_n_write (FILE *out, char *p, int length)
{
  static ulong after[16], *af;
  static int   i, j, k;

  /* Re-arrange data-characters to 2*8 longs containing 31 bits each.*/
  for (i=k=0; i<64; i++)
  {
    if ((i&3) == 3)
    {
      after[k] = 0L;
      for (j=i;j>(i-4);j--)
        after[k] = after[k] * 216L + decode[(byte)p[j]];
      k++;
    }
  }

  af = after;
  for (i=0; i<2; i++, af+=8)
  {
    /* Re-arrange to 2*8 longs containing 32 bits.
       7th and 15th long only contain 24 valid bits. */
    af[0] = (af[0] << 1) | (af[1] >> 30);
    af[1] = (af[1] << 2) | (af[2] >> 29);
    af[2] = (af[2] << 3) | (af[3] >> 28);
    af[3] = (af[3] << 4) | (af[4] >> 27);
    af[4] = (af[4] << 5) | (af[5] >> 26);
    af[5] = (af[5] << 6) | (af[6] >> 25);
    af[6] = (af[6] << 7) | (af[7] >> 24);
    af[7] = (af[7] << 8);
    for(j=0; j<8; j++)
    {
      for (k=24;k;k-=8)
      {
        if (!length)
          break;
        length--;
        my_putc ((int) (af[j] >> k), out);
      }
      if (j == 7 || !length)
        break;
      length--;
      my_putc ((int) af[j], out);
    }
  }
}

/*
*** Write indexfile and error report
***
***
 */
void w_index_err (struct m_index *idxp, const char* localname, int flag)
{
  FILE *ifile;
  char filename[13];
  char filename2[MAXPATH];

  fnsplit (idxp->filename, NULL, NULL, filename, NULL);

  if (!flag)
  {
    if (localname != NULL)
      sprintf (filename2, "%s%s", genpath, localname);
    else
      sprintf (filename2, "%s%s", genpath, filename);
    check_fn (filename2);
    #ifndef _HAVE_CHSIZE
     strcat (filename2, ".7ix");
     ifile = fopen (filename2, OPEN_WRITE_BINARY);
    #else
     strcat (filename2, ".7mf");
     ifile = fopen (filename2, OPEN_APPEND_BINARY);
    #endif
    write_index (ifile, idxp, 0);
    fclose (ifile);
  }

  strcat (filename, ".err");

  if (localname != NULL)
    sprintf (filename2, "%s%s%s", genpath, localname, ".err");
  else
    sprintf (filename2, "%s%s", genpath, filename);

  check_fn (filename2);

  ifile = fopen (filename2, OPEN_WRITE_TEXT);
  strupr (filename);
  fprintf (ifile, " go_text. %s%s", filename, delimit);
  strcpy (filename, idxp->filename);
  strupr (filename);
  fprintf (ifile, "7PLUS error report: %s %03X", filename, idxp->splitsize);
  if (strcmp (idxp->full_name, idxp->filename))
    fprintf (ifile, " /%s/", idxp->full_name);
  fprintf (ifile, " %ld%s", idxp->length, delimit);
  write_index (ifile, idxp, 1);
  fprintf (ifile, "[%lX]%s00%s________%s stop_text.%s",
                          idxp->timestamp, delimit, delimit, delimit, delimit);
  if (endstr)
    fprintf (ifile, "%s%s", endstr, delimit);
  fclose (ifile);

  crc_file (filename2, "7P", "00\n", 0);
}

/*
*** If an error report has been accidentally erased, it can be recreated
*** using the information in the indexfile, respectively metafile.
*** (depends on _CHSIZE_OK)
 */
int make_new_err (const char *name)
{
  FILE   *rfile;

  fprintf (o, "\n-----------------------\n"
                "Recreating error report\n"
                "-----------------------\n\n");

  /* Open meta file */
  if ((rfile = fopen (name, OPEN_READ_BINARY)) == NULLFP)
  {
    fprintf (o, cant, name);
    return (2);
  }

  /* read index info into struct idxptr */
  if (read_index (rfile, idxptr))
  {
    fprintf (o, "\007Invalid index info.\n");
    return (7);
  }

  fclose (rfile);

  w_index_err (idxptr, NULL, 1);

  fprintf (o, "Error report has been recreated from '%s'.\n", name);

  return (0);
}

/*
*** Progress indication
***
***
 */
void progress (const char *filename, int part, int of_parts, long errors,
                                             long rebuilt, const char *status)
{
  if (no_tty)
    return;

  set_autolf(0);

#ifdef __MWERKS__
  fprintf (o, "\r%-12s %3d %3d %6ld  %6ld   %-30s",
    filename, part, of_parts, errors, rebuilt, status);
#else
  fprintf (o, "%-12s %3d %3d %6ld  %6ld   %-30s\r",
    filename, part, of_parts, errors, rebuilt, status);
#endif

  fflush (o);
  set_autolf(1);
}
