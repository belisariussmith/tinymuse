// pcmds.c 
// $Id: pcmds.c,v 1.5 1993/08/16 01:56:36 nils Exp $ 

#include  <stdio.h>
#include  <time.h>
#include <sys/file.h>
#include <sys/fcntl.h>

#include "tinymuse.h"
#include "credits.h"
  
#include "db.h"
#include "config.h"
#include "externs.h"
  
char *base_date    = BASE_DATE;
char *upgrade_date = UPGRADE_DATE;
int day_release    = DAY_RELEASE;

extern long muse_up_time;
extern long next_dump;
extern long next_mail_clear;

static char *get_version()
{
    static char buf[BUF_SIZE];
    static int abs_day = 0;

    if (abs_day == 0)
    {
        abs_day  = (atoi(&upgrade_date[6]) - 91) * 372;
        abs_day += (atoi(upgrade_date) - 1) * 31;
        abs_day += atoi(&upgrade_date[3]);
    
        abs_day -= (atoi(&base_date[6]) - 91) * 372;
        abs_day -= (atoi(base_date) - 1) * 31;
        abs_day -= atoi(&base_date[3]);
    }

    sprintf(buf, "%s.%d%s", BASE_VERSION, day_release-1
#ifdef MODIFIED
	  ,"M"
#else
#ifdef BETA
	  ," beta"
#else
#ifdef ALPHA
	  ," alpha"
#else
#ifdef FINAL
	  ," final"
#else
	  ,""
#endif
#endif
#endif
#endif
	  );
  return buf;
}

void do_version(dbref player)
{
    send_message(player, "%s Version Information:", muse_name);
    send_message(player, "   Last Code Upgrade: %s", UPGRADE_DATE);
    send_message(player, "   Version reference: %s", get_version());
    send_message(player, "   DB Format Version: v%d", DB_VERSION);
}

void do_uptime(dbref player)
{
    int index = 28;
    long a, b=0, c=0, d=0, t;
    static char pattern[] = "%d days, %d hrs, %d min and %d sec";
    char format[50];

    // get current time and total run time
    a = now - muse_up_time;

    // calc seconds
    t = a / 60L;

    if ( t > 0L )
    {
        b = a % 60L;
        a = t;

        if ( a > 0L )
        {
            index = 17;
        }

        // calc minutes 
        t = a / 60L;

        if ( t > 0 )
        {
            c = b;
            b = a % 60L;
            a = t;
            if ( a > 0 )
            {
                index = 9;
            }

            // calc hours and days 
            t = a / 24L;

            if ( t > 0 )
            {
                d = c;
                c = b;
                b = a % 24L;
                a = t;
                if ( a > 0 )
                {
                    index = 0;
                }
            }
        }
    }

    // rig up format for operation time output 
    sprintf(format, "%%s %s", &pattern[index]);

    send_message(player, "%s runtime stats:", muse_name);
    send_message(player, "    Muse boot time..: %s", mktm(muse_up_time, "D", player));
    send_message(player, "    Current time....: %s", mktm(now, "D", player));
    send_message(player, format, "    In operation for:", (int) a, (int) b, (int) c, (int) d);
    send_message(player, "");
    send_message(player, "    Next database @dump at: %s", mktm(next_dump, "D", player));
    send_message(player, "    Next old mail clear at: %s", mktm(next_mail_clear, "D", player));
}

// uptime stuff 
static int cpos=0;
static int qcmdcnt[60*5];
static int pcmdcnt[60*5];

void inc_qcmdc()
{
    qcmdcnt[cpos]++;
    pcmdcnt[cpos]--; // we're gunna get this again in process_command 
}

static void check_time()
{
    static struct timeval t,last;

    gettimeofday(&t, NULL);

    while (t.tv_sec != last.tv_sec)
    {
        if (last.tv_sec - t.tv_sec > 60*5 || t.tv_sec-last.tv_sec > 60*5)
        {
            last.tv_sec = t.tv_sec;
        }
        else
        {
            last.tv_sec++;
        }

        cpos++;

        if (cpos >= (60*5) )
        {
            cpos = 0;
        }

        qcmdcnt[cpos] = pcmdcnt[cpos];
    }
}

void inc_pcmdc()
{
    check_time();
    pcmdcnt[cpos]++;
}

void do_cmdav(dbref player)
{
    int len;
    send_message(player, "Seconds  Player cmds/s   Queue cmds/s    Tot cmds/s");

    for(len = 5; len != 0; len = ((len == 5) ? 30 : ((len == 30) ? (60*5) : 0)))
    {
        int i;
        int cnt;
        double pcmds=0, qcmds=0;
        char buf[BUF_SIZE];

        i = cpos-1;
        cnt = len;
        while (cnt)
        {
            if (i <= 0)
            {
                i = 60*5-1;
            }

            pcmds += pcmdcnt[i];
            qcmds += qcmdcnt[i];

            i--; cnt--;
        }      

        sprintf(buf,"%-8d %-14f  %-14f  %f", len, pcmds/len, qcmds/len, (pcmds+qcmds)/len);
        send_message(player, buf);
    }
}

void do_at(dbref player, char *arg1, char *arg2)
{
    dbref newloc, oldloc;
    static int depth = 0;
  
    oldloc = db[player].location;

    if ((Typeof(player) != TYPE_PLAYER && Typeof(player) != TYPE_THING) || oldloc == NOTHING || depth > 10)
    {
        send_message(player, perm_denied());
        return;
    }

    newloc = match_controlled(player, arg1, POW_TELEPORT);

    if (newloc == NOTHING)
    {
        return;
    }

    db[oldloc].contents = remove_first(db[oldloc].contents, player);
    PUSH(player, db[newloc].contents);
    db[player].location = newloc;
    depth++;
    process_command(player, arg2, player);
    depth--;

    // the world has changed, possibly. 
    newloc = db[player].location;
    db[newloc].contents = remove_first(db[newloc].contents, player);
    PUSH(player, db[oldloc].contents);
    db[player].location = oldloc;
}

dbref as_from = NOTHING;
dbref as_to = NOTHING;

void do_as(dbref player, char *arg1, char *arg2)
{
    dbref who;
    static int depth = 0;

    who = match_controlled(player, arg1, POW_MODIFY);

    if (who == NOTHING)
    {
        return;
    }

    if (depth > 0)
    {
        send_message(player, perm_denied());
        return;
    }

    as_from = who;
    as_to = player;
    depth++;
    process_command(who, arg2, player);
    depth--;
    as_to = as_from = NOTHING;
}
