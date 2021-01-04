/* fuer OS-9:
  stat()
  fstat()
  utime()	--  modifkation date setzen
  isatty()      --  mte DG1ECN Do. 07.11.1991 17:16:40
**
** Update : Fr. 08.11.1991 19:56:59 -mte- openmode geaendert
*****************************************************************************
**
** NAME
**		fstat, stat - find file attributes
**
** USAGE
**		#include <stat.h>
**
**		int fstat( int fd , struct stat *statp);
**
**		int stat ( char *filem struct stat *statp);
**
** BESCHREIBUNG
**
**		fstat und stat fuellen Struktur mit den Fileattributen 
**		des gewuenschten Files.
**
** RETURNVALUE  -1 wenn File nicht gefunden oder statp nicht gueltig
**---------------------------------------------------------------------------
*****************************************************************************/
#include <stdio.h>
#include <direct.h>
#include <sgstat.h>
#include <types.h>			/* u_char,u_chort types etc.	*/
#include <time.h>
#include <stat.h>			/* keine OS-9 version			*/

#ifdef __STDC__
int stat ( register const char *path, register struct stat *buf)
#else
int stat (path, buf)
register char *path;
register struct stat *buf;
#endif
{
  register int fd, res;

  if( path == NULL )  /* */
  	return (-1);
  	
  do{
	if( *path != '/')
	{
		if( (fd = open (path, S_IEXEC )) >= 0)		/* Execution-Directory	*/
			break;
	
      	if( (fd = open (path, S_IEXEC|S_IFDIR )) >= 0)	/* Execution-Directory	*/
			break;
	}
	if( (fd = open (path, 0 )) >= 0)			/* Daten-Directory		*/
		break;

	if( (fd = open (path, S_IFDIR )) >=0 )		/* Daten-Directory		*/
		break;

	return (-1);
  }while(0);

  res = fstat (fd, buf);
  close (fd);
  return res;
}
int
#ifdef __STDC__
fstat ( register int fd, register struct stat *buf)
#else
fstat (fd, buf)
     register int fd;
     register struct stat *buf;
#endif
{
  struct sgbuf tmp_sg;
  struct fildes tmp_fd;
  struct tm tmp_tm;

  if( buf == 0 || (fd < 0 ) )			/* grobes Fehlverhalten abfangen */		
  	return (-1);	

  if (_gs_opt (fd, &tmp_sg) < 0)		/* Path-Optionen holen			*/
    return -1;							/* -1 = Fehler , errno			*/

  buf->st_mode  = tmp_sg.sg_class * 256;

	/* sg_class
	   0 = SCF,
	   1 = RBF,
	   2 = PIPE,
	   3 = SBF,
	   4 = NET
	*/   		

  if (tmp_sg.sg_class != 1)				/* if not RBF */
    {
      buf->st_dev = tmp_sg.sg_tbl;
      return 0;
    }

  buf->st_dev   = tmp_sg.sg_dvt;	/* RBF: Adresse der Device-Table-Entry */
  if (_gs_gfd (fd, &tmp_fd, sizeof(struct fildes)) < 0)
  {
	    return -1;
  } 
  buf->st_ino   = tmp_sg.sg_fdpsn;			/* RBF: file descriptor PSN		*/
  buf->st_nlink = tmp_fd.fd_link;
  buf->st_mode += tmp_fd.fd_att;

  buf->st_gid = tmp_fd.fd_own[0];			/* Group - ID					*/
  buf->st_uid = tmp_fd.fd_own[1];			/* User-Id						*/
  memcpy (&(buf->st_size), &(tmp_fd.fd_fsize[0]), 4);	/* File Size		*/

  tmp_tm.tm_sec   = 0;
  tmp_tm.tm_min   = tmp_fd.fd_date[4];
  tmp_tm.tm_hour  = tmp_fd.fd_date[3];
  tmp_tm.tm_mday  = tmp_fd.fd_date[2];
  tmp_tm.tm_mon   = tmp_fd.fd_date[1] - 1;
  tmp_tm.tm_year  = tmp_fd.fd_date[0];
  tmp_tm.tm_isdst = -1;
  if ((buf->st_atime = buf->st_mtime = mktime (&tmp_tm)) < 0)
    return -1;
    
  buf->st_mtime = buf->st_atime  ; /* ?? */

  tmp_tm.tm_min   = 0;
  tmp_tm.tm_hour  = 0;
  tmp_tm.tm_mday  = tmp_fd.fd_dcr[2];
  tmp_tm.tm_mon   = tmp_fd.fd_dcr[1] - 1;
  tmp_tm.tm_year  = tmp_fd.fd_dcr[0];
  tmp_tm.tm_isdst = -1;
  if ((buf->st_ctime = mktime (&tmp_tm)) < 0)
    return -1;

  return 0;
}

/* File access und modifikation   setzen */
int
#ifdef __STDC__
utime( register const char *file, register struct utimbuf * times)
#else
utime(file,times)
register char *file;
register struct utimbuf * times;
#endif
{
  struct sgbuf tmp_sg;
  struct fildes tmp_fd;
  struct tm *tmp_tm;
  register int fd, res=0;


  if ((fd = open (file, S_IWRITE | S_IREAD )) < 0
      && (fd = open (file, S_IFDIR | S_IWRITE | S_IREAD   )) < 0)
    return -1;


  if (_gs_opt (fd, &tmp_sg) >= 0)		/* kein zugriff auf file optionen ??*/
  {
	  if (tmp_sg.sg_class == 1)	/* if not RBF */
	  {
		  if (_gs_gfd (fd, &tmp_fd, sizeof(struct fildes)) >=0 )
		  {

			if( times->actime)
			{
			  tmp_tm = localtime( &times->actime);
			  tmp_fd.fd_date[4] = tmp_tm->tm_min;		/* minute last modified */
			  tmp_fd.fd_date[3] = tmp_tm->tm_hour;
			  tmp_fd.fd_date[2] = tmp_tm->tm_mday;
			  tmp_fd.fd_date[1] = tmp_tm->tm_mon + 1;
			  tmp_fd.fd_date[0] = tmp_tm->tm_year;
			}

			if( times->modtime)
			{
					/* creation date  */
		   
			  tmp_tm = localtime( &times->modtime);

			  tmp_fd.fd_dcr[2] = tmp_tm->tm_mday ;
			  tmp_fd.fd_dcr[1] = tmp_tm->tm_mon   +1;
			  tmp_fd.fd_dcr[0] = tmp_tm->tm_year  ;
			}

			if( _ss_pfd (fd, &tmp_fd)== -1)		/* date setzen */
			  res = -1;
	     }else res = -1;
	  }else res = -1;
  }else res = -1;

  close (fd);

  return res;
	
}
#ifdef __STDC__
int utimes(register const char *file, register struct utimbuf * times)
#else
int utimes(file,times)
register char *file;
register struct utimbuf * times;
#endif
{
		return utime(file,times);
}

/****************************************************************************
**
** isatty						Pruefen, ob Pfad zu einem SCF-Device fuehrt
**
** UEBERSICHT:
**				int isatty( int fd )
**
** BESCHREIBUNG:
**              Die Funktion isatty() liefert einen Returnwert der Ungleich
**				Null ist, wenn das Argument ein SCF-Pfad (Terminal) ist.
**				Andernfalls gibt die Funktion eine 0 zurueck.
**
** SIEHE AUCH:
**				_gs_opt()
**
*****************************************************************************/
#ifdef __STDC__
int isatty(register int fd)
#else
int isatty(fd)
register int fd;
#endif
{
#define TYP_SCF 0
#define TYP_RBF 1
#define TYP_PIPE 2
#define TYP_SBF 3
#define TYP_NET 4
  struct sgbuf tmp_sg;

  if (_gs_opt (fd, &tmp_sg) < 0)
    return -1;

  return( tmp_sg.sg_class == TYP_SCF);
  	
}/* End of isatty() */
