/*
 *  stat.h for OSK : 89/04/10 by A.Seyama
 *  -mte DG1ECN Do. 07.11.1991 17:27:14
 */

#ifdef S_IFMT
/*
 *  All definitions in modes.h are present in this file.
 *  Furthermore, some modification is made, so that including 
 *  both stat.h and modes.h may produces an incorrect result.
 *
 */
/* #error Can not include both stat.h and modes.h. */
#endif

#ifndef ctime
/*
 *  MicroWare's configuration of header files are not so good that
 *  including types.h does not give typedef of time_t.
 *  We must include time.h to get typedef of time_t.
 *
 */
#include <time.h>
#endif

/* file modes */
#define S_IFMT	0xff80
#define S_IFREG	0x0100	/* S_IFREG means RBF type device and not directory */
#define S_IFIFO 0x0200  /* PIPE */
#define S_IFCHR 0x0000  /* SCF */
#define S_IFDIR 0x0080  /* directory */
#define S_ISIZE	0x0020	/* set initial file size */

/* permissions */
#define	S_IPRM		0x007f	/* mask for permission bits */
#define	S_IREAD		0x0001	/* owner read */
#define	S_IWRITE	0x0002	/* owner write */
#define	S_IEXEC		0x0004	/* owner execute */
#define	S_IOREAD	0x0008	/* public read */
#define	S_IOWRITE	0x0010	/* public write */
#define	S_IOEXEC	0x0020	/* public execute */
#define	S_ISHARE	0x0040	/* sharable */

/* permissions (Un*x compatible) */
#define S_IRWXU 0x0007	/* owner read write execute */
#define S_IRUSR	0x0001	/* owner read */
#define S_IWUSR	0x0002	/* owner write */
#define S_IXUSR	0x0004	/* owner execute */
#define S_IRWXG 0x0038  /* public read write execute */
#define S_IRGRP	0x0008	/* public read */
#define S_IWGRP	0x0010	/* public write */
#define S_IXGRP	0x0020	/* public execute */
#define S_IRWXO 0x0038  /* public read write execute */
#define S_IROTH	0x0008	/* public read */
#define S_IWOTH	0x0010	/* public write */
#define S_IXOTH	0x0020	/* public execute */

struct stat {
  unsigned int st_dev;		/* sg_dvt   (RBF option area)
							   sg_tbl   (SCF option area)	*/
  unsigned int st_ino;		/* sg_fdpsn (option area)	*/
  u_short      st_mode;		/* sg_class (option area) in MSB
							   fd_att   (file descriptor) in LSB	*/
  short        st_nlink;	/* fd_link  (file descriptor)	*/
  unsigned int st_rdev;		/* only defined			*/
  u_short      st_uid;		/* fd_own   (file descriptor)	*/
  u_short      st_gid;		/* fd_own   (file descriptor)	*/
  unsigned int st_size;		/* fd_fsize (file descriptor)	*/
  time_t       st_atime;	/* fd_date  (file desctiptor)	*/
  time_t       st_mtime;	/* fd_date  (file desctiptor)	*/
  time_t       st_ctime;	/* fd_dcr   (file desctiptor)	*/
};

  struct utimbuf {
   time_t actime;
   time_t modtime;
  };


#ifdef __STDC__
int stat (const char *, struct stat *);
int fstat (int , struct stat *);
int utime  (const char *,struct utimbuf *);
int utimes (const char *,struct utimbuf *);
int isatty(int);
#else
extern stat ();
extern fstat ();
extern utime ();
extern utimes ();
extern isatty();
#endif
