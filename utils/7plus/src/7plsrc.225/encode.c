#include "7plus.h"
#include "globals.h"

/*
*** encode a file. split, if desired/needed. create correction file.
***
***
 */

int encode_file (char *name, long blocksize, char *search,
                 int join, char *head_foot)
{
  int      part, parts, first_part, last_part, blocklines, curline;
  int      correct, corrlines , corrpart, corrline, tune;
  uint     csequence;
  ulong    ftimestamp, ftstamp0 , after[16], *af, h;
  long     binbytes, binb0, position, size, blocksize2, corrsize;
  char     destname[MAXPATH], hdrname[MAXFNAME], filename[MAXPATH], corrname[13];
  char     orgcorrname[13], orgname[MAXFNAME];
  char     inpath[MAXFPATH], format_file[MAXPATH];
  char     line[81], line2[81], dummi[20], dummi2[20], *q, *r;
  char     go_buf[257], stop_buf[257], cor_head[129], searchbin[MAXPATH];

  FILE     *in, *out, *corr;
  register int i, j, k;

  correct = i = corrpart = corrlines = blocklines = 0;
  *destname = *hdrname = *filename = *corrname = *inpath = *orgname = EOS;
  *format_file = *_file = *go_buf = *stop_buf = *searchbin = EOS;
  out = corr = NULLFP;
  ftimestamp = ftstamp0 = 0UL;
  corrsize = blocksize2 = 0L;

  #ifdef TWO_CHAR_SEP
   tune = 1;
  #else
   tune = 0;
  #endif

  if (fls)
    if (!test_exist (_7PLUS_FLS))
      unlink (_7PLUS_FLS);

  if (search)
  {
    strcpy (searchbin, search);

    /* Get searchpath for unencoded original file. */
    fnsplit (search, _drive, _dir, _file, _ext);
    sprintf (inpath, "%s%s", _drive, _dir);
  }

  q = name;
  /* create correction file, if ext of input file is .ERR */
  if ((r = strrchr (q, '.')) != NULL)
  {
    if (!strnicmp (".err", r, 4))
    {
      /* OK, input file is an error file */
      correct = 1;
      if ((i = crc_file (q, "7PLUS error", "00\n", 1)) != 0)
        if (i != 17)
          return (i);

      corr = fopen (q, OPEN_READ_TEXT);

      /* Find starting line. */
      while ((r = my_fgets (line, 80, corr)) != NULL)
        if (!strncmp (line, "7PLUS ", 6))
          break;

      if (!r)
      {
        fprintf (o, "\007'%s': invalid error report. Break.\n", q);
        fclose (corr);
        return (7);
      }
      /* Get name, lines per part, and length of original
         file from error file.*/
      *orgname = EOS;
      binb0 = 0L;
      ftstamp0 = 0UL;
      sscanf (line+20, "%12s %s /%66[^/]/ %ld",
                          corrname, dummi, orgname, &binb0);
      corrlines  = get_hex (dummi);
      if (!*orgname)
      {
        binb0 = 0L;
        sscanf (line+20, "%s %s %ld", dummi, dummi, &binb0);
      }
      strlwr (corrname);
      strcpy (orgcorrname, corrname);
      strupr (orgcorrname);

      if (_extended != '*' || !*orgname)
      {
        strcpy (orgname, corrname);
        check_fn (corrname);
      }
      /* Build complete filename for original file */
      if (!*_file)
        strcat (inpath, orgname);
      else
      {
        strcat (inpath, _file);
        strcat (inpath, _ext);
      }
      q = inpath;
    }
  }

  if (!correct && search)
  {
    /* Setup output path for encoded files (if specified) */
    sprintf (searchbin, "%s%s%s%s", _drive, _dir, _file, _ext);
      if (searchbin[strlen(searchbin)-1] != PATHCHAR)
        strcat (searchbin, PATHSEP);
  }

  if ((in = fopen (q, OPEN_READ_BINARY)) == NULL)
  {
    fprintf (o, "\007'%s' not found. Break.\n", q);
    return (2);
  }

  /* read format information */
  if (head_foot)
  {
    /* try to find name.def, e.g. when encoding 'test.dat',
       look for 'test.def' */

    strcpy (format_file, head_foot);
    fnsplit (format_file, _drive, _dir, _file, _ext);
    if (*searchbin && !(*_drive || *_dir))
      sprintf (format_file, "%s%s%s", searchbin, _file, _ext);
    else
      if (head_foot == def_format)
      {
        strcpy (format_file, name);
        if ((r = strrchr (format_file, '.')) != NULL)
          *r = EOS;
        strcat (format_file, ".def");
        /* if default format file 'format.def' does not exist, use previously
           constructed name */
        if (!test_exist (format_file))
        {
          head_foot = format_file;
          join = 1; /* if name.def found, switch on join option */
        }
        else
          strcpy (format_file, head_foot);
      }
    if (read_tb (format_file, go_buf, stop_buf))
      return (2);
  }
  if (sendstr)
    sprintf (go_buf, "%s%s%%O %%P/%%Q%s", sendstr,
                                           twolinesend?delimit:" ",
                                           delimit);
  if (endstr)
    sprintf (stop_buf, "%s%s", endstr, delimit);

  /* Get file's timestamp */
  #if (defined(__MSDOS__) || defined(__TOS__))
   ftimestamp = get_filetime (in);
  #else
   ftimestamp = get_filetime (q);
  #endif

  /* determine size of original file. This could be done with filestat(),
     but it's not available on all compilers. */
  fseek (in, 0L, SEEK_END); /* position read pointer to end of file. */
  size = ftell (in);        /* get size. */
  fseek (in, 0L, SEEK_SET); /* reposition to beginning of file. */

  if (!size)
  {
    fprintf (o, "\nSorry. Can't encode files with zero filelength!\n");
    fclose (in);
    exit (20);
  }

  if (correct && binb0 && size != binb0)
  {
    fprintf (o, notsame, "error report");
    fclose (in);
    fclose (corr);
    return (15);
  }

  parts = 1;

  if (!correct)
  {
    /* Bufferize input, if we're encoding. */
    setvbuf (in, NULL, _IOFBF, buflen);

    /* if blocksize is greater then try to split into blocksize-50000 parts */
    if (blocksize > 50000L)
    {
      blocksize -= 50000L;
      /* calculate how many ascii-bytes per part are needed to get roughly
         equal filelengths. */
      blocksize = (((size + 61) / 62) + (blocksize - 1)) / blocksize;
      blocksize *= 62;
    }

    /* if blocksize is defined as zero or if it's bigger than the file,
       set it to filelength  */
    if (!blocksize || blocksize > size)
      blocksize = size;

    /* automatically split into 512 line parts, if file is bigger. */
    if (blocksize > (512 * 62))
    {
      blocksize = 512 * 62;
      fprintf (o, "Blocksize limited to 512 lines per file.\n");
    }
    /* how many lines do the parts contain? */
    blocklines = (int) ((blocksize + 61) / 62);

    /* how many parts result from that? */
    parts = (int) ((size + blocksize-1) / blocksize);

    blocksize2 = blocksize;

    if (parts > 255)
    {
      fprintf (o, "\007Not more than 255 parts allowed.\n"
        "Choose different blocksize. Break.\n");
      fclose (in);
      return (8);
    }
  }
  else
    if (blocksize > 50000L || blocksize == 138L*62L)
      blocksize = 9940L;
    else
      blocksize = (blocksize/62) *71;

  /* generate filenames */
  fnsplit (q, NULL, NULL, _file, _ext);
  sprintf (orgname, "%s%s", _file, _ext);
  if (strlen (orgname) > 60)
  {
    fprintf (o, "\n\007Filename of original file is too long (max 60 chars).\n"
                "Truncating \"%s\" to ", orgname);
    if (*_ext)
    {
      _ext[4] = EOS;
      _file[56] = EOS;
      sprintf (orgname, "%s%s", _file, _ext);
    }
    else
      orgname[60] = EOS;

    fprintf (o, "\"%s\"!", orgname);
  }

  build_DOS_name (_file, _ext);

  strcpy (destname, _file);

  sprintf (hdrname, "%s%s%s", _file, _ext[0]?".":"", _ext);
  strupr (hdrname);

  if (simulate && !correct)
  {
    if (!*altname)
      sprintf (filename, "%s%s", searchbin? searchbin:"", _7PLUS_FLS);
    else
      sprintf (filename, "%s.fls", altname);
    if ((out = fopen (filename, OPEN_WRITE_TEXT)) == NULLFP)
      return (14);
    fprintf (out, "%d %s\n", parts, destname);
    fprintf (o, "\nNumber of parts: %d, Filename of parts: \"%s\"\n", parts,
                                                                      destname);
    fclose (out);
    fclose (in);
    return (0);
  }

  if (!correct)
    fprintf (o, "\n-----------\n"
                  "Encoding...\n"
                  "-----------\n\n");
  else
    fprintf (o, "\n---------------------------\n"
                  "Creating correction file...\n"
                  "---------------------------\n\n");

  for (i = 1; (i < 256) && !range[i]; i++);
  first_part = i;

  for (i = 255; (i > 0) && !range[i]; i--);
  last_part = i;

  if (last_part >  parts)
    last_part = parts;

  if (first_part > parts)
  {
    fprintf (o, "\007Can't encode part %d of %d... You're pulling my leg!\n",
                     first_part, parts);
    return (1);
  }

  /* encode parts */
  for (part = 1; part<parts+1 ; part++)
  {
    if (!correct)
    {
      /* generate output filename. *.7PL, if unsplit. *.PXX if split.
         XX represents a two digit hex number. */
      set_autolf(0);

      if (parts == 1)
      {
        if (!*altname)
          sprintf (filename, "%s%s%s", searchbin? searchbin:"",
                        destname, ".7pl");
        else
          sprintf (filename, "%s%s", altname, ".7pl");

        if (!no_tty)
        #ifdef __MWERKS__
          fprintf (o, "'%s': Writing.\n", filename);
        #else
          fprintf (o, "'%s': Writing.\r", filename);
        #endif
      }
      else
      {
        if (!*altname)
          sprintf (filename, "%s%s.p%02x", searchbin? searchbin:"",
                          destname, part);
        else
          sprintf (filename, "%s.p%02x", altname, part);

        if (!no_tty && range[part])
        #ifdef __MWERKS__
          fprintf (o, "\r'%s': Writing part %03d of %03d.",
        #else
          fprintf (o, "'%s': Writing part %03d of %03d.\r",
        #endif
                        filename, part, parts);
      }

      fflush (o);
      set_autolf(1);

      if (join)
      {
        if (!*altname)
          sprintf (filename, "%s%s%s", searchbin? searchbin:"",
                        destname, ".upl");
        else
          sprintf (filename, "%s%s", altname, ".upl");
      }
      /* check, if output file already exists. */
      if ((join < 2) && range[part])
        if (test_file (out, filename, 0, 12) == 10)
         return (10);
    }
    else /* we're creating a correction file, set name accordingly. */
    {
      fnsplit (corrname, NULL, NULL, destname, NULL);
      sprintf (filename, "%s%s.cor", genpath, destname);
      if (*altname)
        sprintf (filename, "%s.cor", altname);
    }

    /* If -J is active, only open an output file in the first round */
    if (join < 2)
    {
      if (join)
        join++;
      if (range[part] || join || correct)
      {
        if ((out = fopen (filename, OPEN_WRITE_TEXT)) == NULL)
        {
          fprintf (o, "\n\n\007Write error. Break.\n");
          exit (1);
        }
        setvbuf (out, NULL, _IOFBF, buflen);
      }
      else
        out = NULL;
    }

    if (!correct)
    {
      if (!range[part])
      {
        fseek (in, blocksize2, SEEK_CUR);
        continue;
      }
      if (part == parts && parts > 1)
      {
        if (size % blocksize)
          blocksize = size % blocksize;
        blocksize = ((blocksize + 61 ) / 62) *62;
      }

      top_bottom (out, go_buf, orgname /*hdrname*/, "p", part, parts);

      /* output header */
      sprintf (line, " go_7+. %03d of %03d %-12s %07ld %04X %03X (7PLUS v2.2) "
                     "\xb0\xb1\xb2%c", part, parts, hdrname, size,
                     (uint)(((blocksize+61)/62) * 64), blocklines, _extended);

      mcrc (line, 1);
      add_crc2 (line);
      fprintf (out, "%s%s", line, delimit);

      if (part == 1 && _extended == '*')
      {
        sprintf (line, "///////////////////////////////////////////////////"
                       "///////////\xb0\xb1\xb2*");
        memcpy (line+1, orgname, strlen(orgname));
        mcrc (line, 1);
        add_crc2 (line);
        fprintf (out, "%s%s", line, delimit);
      }
    }
    else
    {
      /* output correction file header */
      fnsplit (orgcorrname, NULL, NULL, dummi2, NULL);
      strcat (dummi2, ".COR");
      strupr (dummi2);
      top_bottom (out, go_buf, dummi2, "c", part, part);

      sprintf (cor_head, " go_text. %s%s7PLUS correction: %s %ld %03X",
                                dummi2, delimit, orgcorrname, size, corrlines);
      if (ftimestamp)
      {
        sprintf (dummi, " [%lX]", ftimestamp);
        strcat (cor_head, dummi);
      }
      strcat (cor_head, delimit);
      corrsize += fprintf (out, "%s", cor_head) +tune;

      fscanf (corr, "%s", dummi2);
      corrpart = get_hex (dummi2);
      corrsize += fprintf (out, " P%02x:%s", corrpart, delimit) +tune;

      if (!no_tty)
      {
        set_autolf(0);
        #ifdef __MWERKS__
        fprintf (o, "\rCompiling: '%s'", filename);
        #else
        fprintf (o, "Compiling: '%s'\r", filename);
        #endif
        fflush (o);
        set_autolf(1);
      }
    }

    curline = j = 0;
    binbytes = 0L;

    /* get bytes from original file until it ends or blocksize is reached. */
    while ((!feof(in) && ((binbytes < blocksize) || parts == 1)) || correct)
    {
      csequence = 0;

      if (correct)
      {
        /* get number of part and number of line to put into correction file
           from error file */
        fscanf (corr, "%s", dummi2);
        corrline  = get_hex (dummi2);
        if (corrline  == 0xfff || corrsize > blocksize)
        {
          if (corrline  == 0xfff)
          {
            fscanf (corr, "%s", dummi2);
            corrpart = get_hex (dummi2);
            if (!corrpart)
            {
              sscanf (dummi2, "[%lX]", &ftstamp0);
              corrsize = (long) blocksize+1;
              fprintf (o, "\n");
            }
            else
            {
              fscanf (corr, "%s", dummi2);
              corrline  = get_hex (dummi2);
            }
          }
          if (corrsize > blocksize)
          {
            corrsize = 0L;

            /* if we were creating a correction file, complete it. */
            fprintf (out, " P00:%s________%s stop_text.%s",
                                                   delimit, delimit, delimit);


            sprintf (line, "%s.c%02x", destname, part);
            strupr (line);
            top_bottom (out, stop_buf, line, "c", part , part);

            if (!corrpart)
              break;

            if (ferror(out)) /* did any errors occur while writing? */
            {
              fprintf (o, "\n\007Write error. Break.\n");
              fclose (corr);
              fclose (out);
              fclose (in);
              return (1);
            }
            fclose (out);

            crc_file (filename, "7P", " P00:\n", 0);

            fnsplit (corrname, NULL, NULL, destname, NULL);
            check_fn (destname);
            if (!*altname)
              sprintf (filename, "%s%s.c%02x", genpath, destname, part++);
            else
              sprintf (filename, "%s.c%02x", altname, part++);

            out = fopen (filename, OPEN_WRITE_TEXT);
            setvbuf (out, NULL, _IOFBF, buflen);

            sprintf (line, "%s.c%02x", destname, part-1);
            strupr (line);
            top_bottom (out, go_buf, line, "c", part, part);

            corrsize += fprintf (out, "%s", cor_head) +tune;

            if (!no_tty)
            {
              set_autolf(0);
              #ifdef __MWERKS__
              fprintf (o, "\rCompiling: '%s'", filename);
              #else
              fprintf (o, "Compiling: '%s'\r", filename);
              #endif
              fflush (o);
              set_autolf(1);
            }
          }
          corrsize += fprintf (out, " P%02X:%s", corrpart, delimit) +tune;
        }
        curline = corrline ;
        /* calculate position in original file to get data from. */
        position =  (long)(corrpart-1) * 62 * (long)corrlines  +
                    62 * (long)corrline  ;
        /* position read pointer. */
        fseek (in, position, SEEK_SET);
        corrsize += fprintf (out, " L%03X:%s", corrline , delimit) +tune;
      }

      /* get two groups of 31 bytes and stuff them into 2 * 8 longs. */
      af = after;
      for (i=0; i<2; i++, af+=8)
      {
        /* Get 31 Bytes and put them into 8 longs. */
        for(j=0; j<8; j++)
        {
          af[j] = 0L;
          for (k=(j==7)?2:3; k>-1; k--)
          {
            if ((int)(h = fgetc (in)) == EOF)
            {
              if (!i && !j && k == 3)
                i = 255;
              h = 0L;
            }
            af[j] = (af[j] << 8) | h;
          }
        }
        /* Rearrange into 8 31bit values. */
        af[7] =  af[7]       | ((af[6] & 127L) << 24);
        af[6] = (af[6] >> 7) | ((af[5] & 63L ) << 25);
        af[5] = (af[5] >> 6) | ((af[4] & 31L ) << 26);
        af[4] = (af[4] >> 5) | ((af[3] & 15L ) << 27);
        af[3] = (af[3] >> 4) | ((af[2] & 7L  ) << 28);
        af[2] = (af[2] >> 3) | ((af[1] & 3L  ) << 29);
        af[1] = (af[1] >> 2) | ((af[0] & 1L  ) << 30);
        af[0] = (af[0] >> 1);
      }
      /* i is 256, then no bytes were read. End of file. */
      if (i == 256)
        break;

      binbytes += 62;

      /* write code line to output file. do radix216 conversion, crc
         calculation and ascii conversion as we go along. */
      for (i=j=0;i<16;i++)
      {
        line2[j++]  = code[(int)(after[i] % 0xd8L)];
        after[i]  /= 0xd8L;
        line2[j++]  = code[(int)(after[i] % 0xd8L)];
        after[i]  /= 0xd8L;
        line2[j++]  = code[(int)(after[i] % 0xd8L)];
        line2[j++]  = code[(int)(after[i] / 0xd8L)];
      }

      for (i=0;i<64;i++)
        crc_calc (csequence, line2[i]);

      /* package line number and crc into three radix216 bytes and add
         to code line. */
      after[0]   = (long)(curline & 0x1ff) << 14;
      after[0]  |= (csequence & 0x3fff);
      line2[j++] = code[(int) (after[0] % 0xd8L)];
      after[0]  /= 0xd8L;
      line2[j++] = code[(int) (after[0] % 0xd8L)];
      line2[j]   = code[(int) (after[0] / 0xd8L)];

      add_crc2 (line2);
      corrsize += fprintf (out, "%s", line2) +tune;

      /* conclude line with line separator. */
      corrsize += fprintf (out, delimit);
      curline++; /* increase line counter. */
    }

    if (!correct) /* put end indicator into output file. */
    {
      /* strupr (filename); */

      /* Add timestamp */
      sprintf (line, "                                                  "
                     "            \xb0\xb1\xb2\xdb");
      strupr (destname);
      if (parts > 1)
        sprintf (line2, " stop_7+. (%s.P%02X/%02X) [%lX]",
                                       destname, part, parts, ftimestamp);
      else
        sprintf (line2, " stop_7+. (%s.7PL) [%lX]", destname, ftimestamp);
      strlwr (destname);

      memcpy (line, line2, strlen(line2));
      mcrc (line, 1);
      add_crc2 (line);
      fprintf (out, "%s%s", line, delimit);
      top_bottom (out, stop_buf, hdrname, "p", part, parts);
    }
    else
    {
      if (no_tty)
      {
        fprintf (o, "Compiled correction file '");
        if (!*altname)
          fprintf (o, "%s.cor'", destname);
        else
          fprintf (o, "'%s.cor'", altname);

   if (part>1)
        {
          fprintf (o, " through '");
          if (!*altname)
            fprintf (o, "%s.c%02x'", destname, part-1);
          else
            fprintf (o, "%s.c%02x'", altname, part-1);
        }
        fprintf (o, ".\n");
      }
      if (ftstamp0 && ftstamp0 != ftimestamp)
        fprintf (o, "\007Warning: Timestamp in error report differs from the "
                    "original file!\n");
    }
    if (ferror(out)) /* did any errors occur while writing? */
    {
      fprintf (o, "\n\007Write error. Break.\n");
      fclose (in);
      fclose (out);
      return (1);
    }
    /* OK. This part is done. Close output file only when producing seperate
         parts */

    if (join && !*stop_buf)
      fprintf (out, "%s%s", delimit, delimit);

    if (!join || correct)
      if (out)
        fclose (out);

    if (correct) /* If creating a COR file, append CRC line */
      crc_file (filename, "7P", " P00:\n", 0);

    if ((part == last_part) || correct)
      part = 256;
  } /* end of for() */

  if (join)
    fclose (out);

  /* all parts done.
     tell user about action. */
  if (!correct)
  {
    if (no_tty)
    {
      fprintf (o, "Encoded ");
      if (!*altname)
        fprintf (o, "'%s%s", searchbin? searchbin:"", destname);
      else
        fprintf (o, "'%s", altname);
      fprintf (o, ".%s'", (parts==1)?"7pl":"p01");


      if (parts > 1)
      {
        fprintf (o, " through ");
        if (!*altname)
          fprintf (o, "'%s%s", searchbin? searchbin:"", destname);
        else
          fprintf (o, "'%s", altname);
        fprintf (o, ".p%02x'", parts);
      }
      fprintf (o, ".\n");
    }

    #ifdef __MWERKS__
     if (parts > 1)
       fprintf (o, "\n");
    #endif

    if (join)
    {

      fprintf (o, "\n\nCombined output written to ");
      if (!*altname)
        fprintf (o, "'%s%s%s'\n", searchbin? searchbin:"", destname, ".upl");
      else
        fprintf (o, "'%s%s'\n", altname, ".upl");
    }

    fprintf (o, "\n\nEncoding successful!\n");
    if (fls)
    {
      if (!*altname)
        sprintf (filename, "%s%s", searchbin? searchbin:"", _7PLUS_FLS);
      else
        sprintf (filename, "%s.fls", altname);
      if ((out = fopen (filename, OPEN_WRITE_TEXT)) == NULLFP)
        return (14);
      fnsplit (destname, NULL, NULL, _file, NULL);
      fprintf (out, "%d %s\n", parts, _file);
      fclose (out);
    }
  }
  fclose (in);
  return (0);
}

/*
*** Get info from header file
***
***
 */

int read_tb (char *name, char *go_top, char *go_bottom)
{
  FILE *rfile;
  int i, j, prev;
  char *p, *q, line [81], compare[10];

  j = prev = 0;

  if ((rfile = fopen (name, OPEN_READ_TEXT)) == NULLFP)
  {
    fprintf (o, cant, name);
    return (2);
  }

  q = go_top;
  strcpy (compare, "@@TOP\n");

  while (1 == 1)
  {
    p = my_fgets (line, 80, rfile);

    if (strnicmp (p, compare, strlen(compare)))
    {
      *q = EOS;
      j  = EOF;
    }
    else
    {
      i = 0;
      do
      {
        prev = j;

        q[i] = (char) (j = fgetc(rfile));

        #ifndef OSK  /* OSK uses CR as line sep. VERY strange.. Reminds me of
                        the good ol' C64 days ;-> */
        if (j == '\r')
          continue;
        #endif

        i++;
      }
      while (j != EOF && !(j == '@' && prev == '\n') && i < 256);

      q[i-2] = '\n';
      q[i-1] = EOS;
    }

    if (j != '@' && j != EOF)
      do
      {
        prev = j;
        j = fgetc(rfile);
      }
      while (j != EOF && !(j == '@' && prev == '\n'));

    if (j == '@')
      ungetc (j, rfile);

    if (j == EOF && q == go_bottom)
      break;

    if (q == go_top)
    {
      q = go_bottom;
      strcpy (compare, "@@BOTTOM\n");
    }
  }
  fclose (rfile);

  return (0);
}

/*
*** output head or foot
***
***
 */

int top_bottom (FILE *wfile, char *top_bot, char *orgname,
                char *type, int part, int parts)
{
  int i;
  char __file[MAXFNAME], __ext[MAXEXT], _type[2];

  fnsplit (orgname, NULL, NULL, _file, _ext);
  strcpy (__file, _file);
  strcpy (__ext , _ext );
  strcpy (_type , type);
  strlwr (_file);
  strlwr (_ext );

  i = 0;
  while (top_bot[i] != EOS)
  {
    if (top_bot[i] == '%')
    {
      i++;
      if (!top_bot[i])
        break;

      switch (top_bot[i])
      {
        case 'o': fprintf (wfile, "%s%s", _file, _ext);
                  break;
        case 'O': fprintf (wfile, "%s%s", __file, __ext);
                  break;
        case 'n': if (part == 1 && parts == 1)
                  {
                    if (*type == 'p')
                      fprintf (wfile, "%s.7pl", _file);
                    else
                      fprintf (wfile, "%s.cor", _file);
                  }
                  else
                  {
                    if (*type == 'c')
                      part -= 1;
                    strlwr(_type);
                    fprintf (wfile, "%s.%s%02x", _file, _type, part);
                  }
                  break;
        case 'N': if (part == 1 && parts == 1)
                  {
                    if (*type == 'p')
                      fprintf (wfile, "%s.7PL", _file);
                    else
                      fprintf (wfile, "%s.COR", _file);
                  }
                  else
                  {
                    if (*type == 'c')
                      part -= 1;
                    strupr (_type);
                    fprintf (wfile, "%s.%s%02X", _file, _type, part);
                  }
                  break;
        case 'p': fprintf (wfile, "%d", part);
                  break;
        case 'P': fprintf (wfile, "%02X", part);
                  break;
        case 'q': if (*type == 'p')
                    fprintf (wfile, "%d", parts);
                  else
                    fprintf (wfile, "X");
                  break;
        case 'Q': if (*type == 'p')
                    fprintf (wfile, "%02X", parts);
                  else
                    fprintf (wfile, "X");
                  break;
        case '%': fprintf (wfile, "%s", "%");
                  break;

        default : fprintf (wfile, "%%%c", top_bot[i]);
      }
      i++;
    }
    if (!top_bot[i])
      break;

    if (top_bot[i] == '\n')
      fprintf (wfile, "%s", delimit);
    else
      my_putc (top_bot[i], wfile);

    i++;
  }

  return (0);
}

/*
*** Evaluate specified range. Be as tollerant as possible.
*** Syntax: '-5,1-2,5,12-' means: parts 1,2,3,4,5 and 12-255.
*** Ranges not required to be in correct order.
*** Overlapping of ranges is allowed, but spaces are not.
 */
void get_range (char *rangestring)
{
  int i, start, end;
  char *p;
  char rstring[260];

  strcpy (rstring, rangestring);

  for (i = 0; i < 256; i++)
    range[i] = 0;

  /* get first range */
  p = strtok (rstring, ",");

  do
  {
    start = end = 0;

    if (*p == '-')
    {
      sscanf (p+1, "%i", &end);
      start = 1;
    }
    else
    {
      i = sscanf (p, "%i-%i", &start, &end);
      if (i == 2 && !end)
        end = 1;

      if (p[(int)strlen(p)-1] == '-')
        end = 255;
    }

    if (start > 255)
      start = 255;

    if (end > 255)
      end = 255;

    if (start == 0)
      start = 1;

    if (!end)
       end = start;

    if (end < start)
    {
      /* End lower than beginning. Trade places */
      i = start;
      start = end;
      end = i;
    }

    /* add range to array */
    for (i = start ;i <= end; i++)
      range[i] = 1;
    range[0] = 1;
  }
  while((p = strtok (NULL, ",")) != NULL); /* get next range */

  /* No range was defined. Set range to 1-255 */
  if (!range[0])
    for (i = 1; i < 256; i++)
      range[i] = 1;
}

