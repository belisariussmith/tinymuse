// log.c 
// $Id: log.c,v 1.7 1993/12/19 17:59:51 nils Exp $ 

#include <stdio.h>
#include <time.h>    // Belisarius - fixes dereferncing pointer to incomplete type 'struct tm' error 

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "externs.h"
#include "log.h"

struct log
    important_log = { NULL, -1, "run/logs/important", "*log_imp"   },
    sensitive_log = { NULL, -1, "run/logs/sensitive", "*log_sens"  },
    error_log     = { NULL, -1, "run/logs/error",     "*log_err"   },
    ioerr_log     = { NULL, -1, "run/logs/ioerr",     "*log_ioerr" },
    io_log        = { NULL, -1, "run/logs/io",        "*log_io"    },
    gripe_log     = { NULL, -1, "run/logs/gripe",     "*log_gripe" },
    root_log      = { NULL, -1, "run/logs/root",      "*log_root"  }
;

struct log *logs[] = {
    &important_log,
    &sensitive_log,
    &error_log,
    &ioerr_log,
    &io_log,
    &gripe_log,
    &root_log,
    0
};

void muse_log(struct log *l, char *str)
{
    static char buf[BUF_SIZE*2];
    struct tm *bdown;

    if (l->com_channel)
    {
        sprintf(buf,"[%s] * %s", l->com_channel, str);
        com_send(l->com_channel, buf);
    }

    if (!l->fptr)
    {
        l->fptr = fopen(l->filename, "a");

        if (!l->fptr)
        {
            mkdir("logs", 0755);
            l->fptr = fopen(l->filename, "a");

            if (!l->fptr)
            {
                fprintf(stderr, "BLEEEP! couldn't open log file %s\n", l->filename);
                return;
            }
        }
    }

    bdown = localtime((time_t *)&now);
    fprintf(l->fptr, "%02d/%02d:%02d:%02d:%02d| %s\n", bdown->tm_mon+1, bdown->tm_mday, bdown->tm_hour, bdown->tm_min, bdown->tm_sec, str);
    fflush(l->fptr);

    if (l->counter-- < 0)
    {
        l->counter = 100;
        fclose(l->fptr);
        l->fptr = NULL;
    };
}

void close_logs()
{
    int i;

    for (i = 0; logs[i]; i++)
    {
        fclose(logs[i]->fptr);
    }
}
