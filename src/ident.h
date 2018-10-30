///////////////////////////////////////////////////////////////////////////////
// ident.h                                                      TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// Author: Peter Eriksson <pen@lysator.liu.se>
// Intruder: P�r Emanuelsson <pell@lysator.liu.se>
///////////////////////////////////////////////////////////////////////////////
#ifndef __IDENT_H__
#define __IDENT_H__

#include "externs.h"

#ifdef	__cplusplus
extern "C" {
#endif

// Sigh
#ifdef __STDC__
#  if __STDC__ == 1
#    define IS_STDC 1
#  endif
#endif

#ifdef __P
#  undef __P
#endif

#ifdef IS_STDC
#  define __P(AL)	                  AL
#else
#  define __P(AL)	                  ()
#endif

#ifdef IS_STDC
#  undef IS_STDC
#endif

//
// Sigh, GCC v2 complains when using undefined struct tags
// in function prototypes...
//
#if defined(__GNUC__) && !defined(INADDR_ANY)
#  define __STRUCT_IN_ADDR_P	void *
#else
#  define __STRUCT_IN_ADDR_P	struct in_addr *
#endif

#if defined(__GNUC__) && !defined(DST_NONE)
#  define __STRUCT_TIMEVAL_P	void *
#else
#  define __STRUCT_TIMEVAL_P	struct timeval *
#endif


#ifndef IDBUFSIZE
#  define IDBUFSIZE 2048
#endif

#ifndef IDPORT
#  define IDPORT	113
#endif

typedef struct
{
  int fd;
  char buf[IDBUFSIZE];
} ident_t;

typedef struct {
  int lport;                    // Local port
  int fport;                    // Far (remote) port
  char *identifier;             // Normally user name
  char *opsys;                  // OS
  char *charset;                // Charset (what did you expect?)
} IDENT;                        // For higher-level routines

// Low-level calls and macros
#define id_fileno(ID)	((ID)->fd)

extern ident_t * id_open(struct in_addr *laddr, struct in_addr *faddr, struct timeval *timeout);

extern int id_close(ident_t *id);

extern int id_query(ident_t *id, int lport, int fport, struct timeval *timeout);

extern int id_parse(ident_t *id, struct timeval *timeout, int *lport, int *fport, char **identifier, char **opsys, char **charset);

// High-level calls

extern IDENT *ident_lookup(int fd, int timeout);

extern char  *ident_id(int fd, int timeout);

extern IDENT *ident_query(__STRUCT_IN_ADDR_P laddr, __STRUCT_IN_ADDR_P raddr, int lport, int rport, int timeout);

extern void   ident_free(IDENT *id);

extern char  _id_version[];

#ifdef IN_LIBIDENT_SRC

extern char *_id_strdup(char *str);
extern char *_id_strtok(char *cp, char *cs, char *dc);

#endif

#ifdef	__cplusplus
}
#endif

#endif
