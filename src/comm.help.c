///////////////////////////////////////////////////////////////////////////////
// help.c                                                        TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: help.c,v 1.11 1993/09/18 19:03:41 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// commands for giving help 
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "externs.h"
#include "help.h"
#include <sys/errno.h>

static void do_help P((dbref, char *, char *, char *, char *, ATTR *));

void do_text(dbref player, char *arg1, char *arg2, ATTR *trig)
{
    char indx[20];
    char text[20];

    if (strlen(arg1) < 1 || strlen(arg1) > 10 || strchr(arg1, '/') || strchr(arg1, '.'))
    {
        send_message(player, "illegal text file.");
        return;
    }

    sprintf(indx,"run/msgs/%sindx",arg1);
    sprintf(text,"run/msgs/%stext",arg1);

    do_help(player, arg2, arg1, indx, text, trig);
}

static void do_help(dbref player, char *arg1, char *deftopic, char *indxfile, char *textfile, ATTR *trigthis)
{
    int help_found;
    help_indx entry;
    FILE *file;
    char *p, line[LINE_SIZE+1];
    char buf[BUF_SIZE];
  
    if ( *arg1 == '\0' )
    {
        arg1 = deftopic;
    }
  
    if ( (file = fopen(indxfile, "r")) == NULL )
    {
        if ( (file = fopen(textfile, "r")) == NULL)
        {
            if (errno == ENOENT)
            {
               	// no such file or directory. no such text bundle. 
                send_message(player, "No such file or directory: %s",textfile);
               	return;
            }
            else
            {
                sprintf(buf, "%s: errno %d", textfile, errno);
                log_error(buf);

                send_message(player, "%s: error number %d", textfile, errno);
        	perror(textfile);

        	return;
            }
        }
        else
        {
            send_message(player, "%s isn't indexed.", textfile);
            fclose(file);
            return;
        }
    }

    while ( (help_found = fread(&entry, sizeof(help_indx), 1, file)) == 1 )
    {
        if ( string_prefix(entry.topic, arg1) )
        {
            break;
        }
    }

    fclose(file);

    if ( help_found <= 0 )
    {
        send_message(player, "No %s for '%s'.", deftopic, arg1);
        return;
    }
  
    if ( (file = fopen(textfile, "r")) == NULL )
    {
        send_message(player, "%s: sorry, temporarily not available.", deftopic);

        sprintf(buf, "can't open %s for reading", textfile);
        log_error(buf);

        return;
    }

    if ( fseek(file, entry.pos, 0) < 0L )
    {
        send_message(player, "%s: sorry, temporarily not available.", deftopic);

        sprintf(buf, "seek error in file %s", textfile);
        log_error(buf);

        fclose(file);

        return;
    }

    for (;;)
    {
        if ( fgets(line, LINE_SIZE, file) == NULL )
        {
            break;
        }

        if ( line[0] == '&' )
        {
            break;
        }

        for ( p = line; *p != '\0'; p++ )
        {
            if ( *p == '\n' )
            {
                *p = '\0';
            }
        }

        send_message(player, line);
    }

    fclose(file);

    if (trigthis)
    {
        dbref zon;
        wptr[0] = entry.topic;

        did_it(player, player, NULL, NULL, NULL, NULL, trigthis);

        DOZONE(zon, player)
        {
            did_it(player, zon, NULL, NULL, NULL, NULL, trigthis);
        }
    }
}
