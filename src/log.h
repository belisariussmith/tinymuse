///////////////////////////////////////////////////////////////////////////////
// log.h                                                   TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: log.h,v 1.4 1993/12/19 17:59:43 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// extern definitions for logging things 
///////////////////////////////////////////////////////////////////////////////

#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>

struct log {
  FILE *fptr;
  int counter;
  char *filename;
  char *com_channel;
};

extern struct log important_log, sensitive_log, error_log, ioerr_log,
  io_log, gripe_log, root_log, cmds_log, suspect_log, register_log, guest_log;

#define log_important(str) muse_log(&important_log, (str))
#define log_sensitive(str) muse_log(&sensitive_log, (str))
#define log_error(str) muse_log(&error_log, (str))
#define log_ioerr(str) muse_log(&ioerr_log, (str))
#define log_io(str) muse_log(&io_log, (str))
#define log_gripe(str) muse_log(&gripe_log, (str))
#define log_is_root(str) muse_log(&root_log, (str))
#define log_cmds(str) muse_log(&cmds_log, (str))
#define log_suspect(str) muse_log(&suspect_log, (str))
#define log_register(str) muse_log(&register_log, (str))
#define log_guest(str) muse_log(&guest_log, (str))

extern void muse_log P((struct log *, char *));

#endif // __LOG_H
