/** globals **/
extern FILE    *o;
extern uint    crctab[];
extern byte    decode[];
extern byte    code[];
extern byte    _extended;
extern size_t  buflen;
extern char    _drive[];
extern char    _dir[];
extern char    _file[];
extern char    _ext[];
extern char    spaces[];
extern char    *endstr;
extern char    *sendstr;
extern char    genpath[];
extern char    altname[];
extern char    delimit[];
extern char    range[];
extern char    cant[];
extern char    miss[];
extern char    notsame[];
extern char    def_format[];
extern int     noquery;
extern int     force;
extern int     fls;
extern int     autokill;
extern int     sysop;
extern int     simulate;
extern int     no_tty;
extern int     twolinesend;
extern struct m_index *idxptr;

#ifdef __MWERKS__
 extern struct suffix_index suffix_table[];
#endif

#ifdef __TOS__
  extern int   nowait;
#endif
