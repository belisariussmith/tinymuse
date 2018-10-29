///////////////////////////////////////////////////////////////////////////////
// timer.c                                               TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: timer.c,v 1.9 1993/09/06 19:33:01 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// Subroutines for timed events 
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

// External declarations
extern char ccom[BUF_SIZE];
extern FILE *command_log;

static int alarm_triggered = FALSE;

static signal_type alarm_handler(int i)
{
    alarm_triggered = TRUE;
    signal(SIGALRM,alarm_handler);
#ifdef void_signal_type
    return;
#else
    return 0;
#endif
}

void init_timer()
{
    signal(SIGALRM, alarm_handler);
    signal(SIGHUP, alarm_handler);

    alarm(1);
}

void dispatch()
{         
    // this routine can be used to poll from interface.c 
    if (!alarm_triggered)
    {
        return;
    }

    alarm_triggered = 0;

    do_second();

    // Free list re ruction 
    {
        static int ticks= -1;

        if (ticks == -1)
        {
            ticks = fixup_interval;
        }

        if (!ticks--)
        {
            if (command_log)
            {
                fprintf(command_log, "Dbcking...");
                fflush(command_log);
            }
 
            ticks = fixup_interval;
            strcpy(ccom,"dbck");
            do_dbck(root);
 
            if (command_log)
            {
                fprintf(command_log, "...Done.\n");
                fflush(command_log);
            }
        }
    }
    // Database dump routines 
    {
        static int ticks = -1;
        extern long next_dump;

        if (ticks == -1)
        {
            ticks = dump_interval;
        }

        if (!ticks--)
        {
            if (command_log)
            {
                fprintf(command_log, "Dumping.\n");
                fflush(command_log);
            }

            ticks = dump_interval;
            strcpy(ccom, "dump");
            fork_and_dump();
            next_dump = now + dump_interval;
        }
    }
    // Stale mail deletion. 
    {
        static int ticks = -1;
        extern long next_mail_clear;

        if (ticks == -1)
        {
            ticks = old_mail_interval;
        }

        if (!ticks--)
        {
            if (command_log)
            {
                fprintf(command_log, "Deleting old mail.\n");
                fflush(command_log);
            }

            ticks = old_mail_interval;
            strcpy(ccom, "mail");
            clear_old_mail();
            next_mail_clear = now + old_mail_interval;
        }
    }

    {
        int i;

        for (i = 0; i < ((db_top / 300) + 1); i++) 
        {
            update_bytes();
        }
    }

    strcpy(ccom, "garbage");
    do_incremental(); // handle incremental garbage collection 
    run_topology();

    // reset alarm 
    alarm(1);
}
