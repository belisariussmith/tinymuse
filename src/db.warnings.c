// topology.c - check to make sure rooms and exits are the way they
// should be. 
// $Id: warnings.c,v 1.2 1993/08/22 18:09:34 nils Exp $ 

#include <stdio.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "externs.h"

static dbref current_object = NOTHING;

static void complain P((dbref, char *, char *));
static void ct_roomdesc P((dbref));
static void ct_onewayexit P((dbref));
static void ct_doubleexit P((dbref));
static void ct_exitmsgs P((dbref));
static void ct_exitdesc P((dbref));
static void ct_playdesc P((dbref));
static void ct_thngdesc P((dbref));
static void ct_thngmsgs P((dbref));
static void ct_exitnames P((dbref));
static void ct_nolinked P((dbref));
static void ct_serious P((dbref));
static void ct_normal P((dbref));
static void ct_extra P((dbref));
static void ct_all P((dbref));

///////////////////////////////////////////////////////////////////////////////
// lock_type()
///////////////////////////////////////////////////////////////////////////////
//       0 == unlocked
//       1 == locked
//       2 == sometimes (complicated)
///////////////////////////////////////////////////////////////////////////////
// Returns: (int) lock status/type
///////////////////////////////////////////////////////////////////////////////
static int lock_type(dbref i, char *str)
{
    char buf[BUF_SIZE];

    if (!*str)
    {
        return UNLOCKED;
    }

    sprintf(buf, "#%d&!#%d", db[i].owner, db[i].owner);
    if (!strcmp(str, buf))
    {
        return LOCKED;
    }

    sprintf(buf, "#%d", db[i].owner);
    if (!strcmp(str, buf))
    {
        return LOCKED;
    }

    sprintf(buf, "#%d", db[i].location);
    if (!strcmp(str, buf))
    {
        return LOCKED;
    }

    // too complicated. need both messages. 
    return COMPLICATED;
}

static void complain(dbref i, char *name, char *desc)
{
    char buf[BUF_SIZE];
    char bufname[BUF_SIZE];
    char *x = buf, *y;

    strcpy(buf, atr_get(i, A_WINHIBIT));

    while ((y = parse_up(&x, ' ')))
    {
        if (!string_compare(y, name) || !string_compare(y, "all"))
        {
            // user doesn't want to hear about
            // it for this object.
            return;
        }
    }

    sprintf(buf, "Warning '%s' for %s: %s",name,unparse_object(db[i].owner,i), desc);
    send_message(db[i].owner, buf);

    // notify interested parties on channels 
    sprintf(buf, "[warn_%s] * %s: %s", name, unparse_object(db[i].owner,i), desc);
    sprintf(bufname, "warn_%s", name);
    com_send(bufname, buf);
}
 
static void ct_roomdesc(dbref i)
{
    if ((Typeof(i) == TYPE_ROOM) && !*atr_get(i, A_DESC))
    {
        complain(i, "roomdesc", "room has no description");
    }
}

static void ct_onewayexit(dbref i)
{
    dbref j;

    if ((Typeof(i) != TYPE_EXIT) || (db[i].link == NOTHING) || (Typeof(db[i].link) != TYPE_ROOM) || db[i].link == db[i].location)
    {
        return;
    }

    for (j = db[db[i].link].exits; j != NOTHING; j = db[j].next)
    {
        if (db[j].link == db[i].location)
        {
            return;
        }
    }

    complain(i, "onewayexit", "exit has no return exit");
}

static void ct_doubleexit(dbref i)
{
    dbref j;

    int count = 0;

    if ((Typeof(i) != TYPE_EXIT) || (db[i].link == NOTHING) || (Typeof(db[i].link) != TYPE_ROOM) || db[i].location == db[i].link)
    {
        return;
    }

    for (j = db[db[i].link].exits; j != NOTHING; j = db[j].next)
    {
        if (db[j].link == db[i].location)
        {
            count++;
        }
    }

    if (count > 1)
    {
        complain(i, "doubleexit", "exit has multiple return exits");
    }
}

static void ct_exitmsgs(dbref i)
{
    int lt;

    if ((Typeof(i) != TYPE_EXIT) || (db[i].flags & DARK))
    {
        return;
    }

    lt = lock_type(i, atr_get(i,A_LOCK));

    if ((lt != 1) && (!*atr_get(i, A_OSUCC) ||
                      !*atr_get(i, A_ODROP) ||
                      !*atr_get(i, A_SUCC)))
    {
        complain(i, "exitmsgs", "exit is missing one or more of osucc, odrop, succ");
    }

    if ((lt != 0) && (!*atr_get(i, A_OFAIL) ||
                      !*atr_get(i, A_FAIL)))
    {
        complain(i, "exitmsgs", "exit is missing one or more of fail, ofail");
    }
}

static void ct_exitdesc(dbref i)
{
    if ((Typeof(i) != TYPE_EXIT) || (db[i].flags & DARK))
    {
        return;
    }

    if (!*atr_get(i,A_DESC))
    {
        complain(i, "exitdesc", "exit is missing description");
    }
}

static void ct_playdesc(dbref i)
{
    if (Typeof(i) != TYPE_PLAYER)
    {
        return;
    }

    if (!*atr_get(i, A_DESC))
    {
        complain(i, "playdesc", "player is missing description");
    }
}

static void ct_thngdesc(dbref i)
{
    if ((Typeof(i) != TYPE_THING) || (db[i].location == db[i].owner))
    {
        return;
    }

    if (!*atr_get(i, A_DESC))
    {
        complain(i, "thngdesc", "thing is missing description");
    }
}

static void ct_thngmsgs(dbref i)
{
    int lt;

    if ((Typeof(i) != TYPE_THING) || (db[i].location == db[i].owner))
    {
        return;
    }

    lt = lock_type(i, atr_get(i, A_LOCK));

    if ((lt != 1) && (!*atr_get(i, A_OSUCC) ||
                      !*atr_get(i, A_ODROP) ||
                      !*atr_get(i, A_SUCC)  ||
                      !*atr_get(i, A_DROP)))
    {
        complain(i, "thngmsgs", "thing is missing one or more of osucc,odrop,succ,drop");
    }

    if ((lt != 0) && (!*atr_get(i, A_OFAIL) ||
                      !*atr_get(i, A_FAIL)))
    {
        complain(i, "thngmsgs", "thing is missing one or more of ofail,fail");
    }
}

static void ct_exitnames(dbref i)
{
    // soon to be written 
}

static void ct_nolinked(dbref i)
{
    if ((Typeof(i) == TYPE_EXIT) && (db[i].link == NOTHING))
    {
        complain(i, "nolinked", "exit is unlinked; anyone can steal it");
    }
}

static void ct_security(dbref i)
{
    ALIST *j;
    char buf[BUF_SIZE];

    if (db[i].parents)
    {
        for (int j = 0; db[i].parents[j] != NOTHING; j++)
        {
            if ( controls(i, db[i].parents[j], POW_MODIFY) && !controls(db[i].parents[j], i, POW_MODIFY) && !(db[i].flags&HAVEN && !db[i].children))
            {
                complain(i, "security", buf);
                sprintf(buf, "wizbug may be inserted on parent %s", unparse_object(db[db[i].parents[j]].owner, db[i].parents[j]));
            }
        }
    }

    if (db[i].list && !*atr_get(i, A_ULOCK) && Typeof(i) != TYPE_PLAYER)
    {
        for (j = db[i].list; j; j = AL_NEXT(j))
        {
            if (AL_TYPE(j) && (*AL_STR(j) == '$' || *AL_STR(j) == '!'))
            {
                char *x = strchr(AL_STR(j), ':');

                if (!x)
                {
                    continue;
                }

                if (!strncmp(AL_STR(j), "$bork *:", 8))
                {
                    sprintf(buf, "i bet draco has a wizbug on attribute %s", unparse_attr(AL_TYPE(j), 0));
                    complain(i, "security", buf);
                    continue;
                }

                x++;

                if (*x == '/')
                {
                    for (x++; *x != '/' && *x; x++);
                    if (*x) x++;
                }

                if (!strncmp(x, "%0", 2) || !strncmp(x, "[v(0", 4))
                {
                    complain(i, "security", buf);
                    sprintf(buf, "wizbug may be present on attribute %s", unparse_attr(AL_TYPE(j), 0));
                    continue;
                }

                if (!strncmp(x, "@fo", 2))
                {
                    for (;*x && *x != '='; x++) ;

                    if (!*x)
                    {
                        continue;
                    }

                    x++;

                    if (!strncmp(x, "%0", 2) || !strncmp(x, "[v(0", 4))
                    {
                        complain(i, "security", buf);
                        sprintf(buf, "wizbug may be present on attribute %s", unparse_attr(AL_TYPE(j), 0));

                        continue;
                    }
                }
                else if (*x == '#')
                {
                    for (;*x && *x != ' '; x++) ;

                    if (!*x)
                    {
                        continue;
                    }

                    x++;

                    if (!strncmp(x, "%0", 2) || !strncmp(x, "[v(0", 4))
                    {
                        complain(i, "security", buf);
                        sprintf(buf, "wizbug may be present on attribute %s", unparse_attr(AL_TYPE(j), 0));
                        continue;
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Now the Groups 
///////////////////////////////////////////////////////////////////////////////

static void ct_serious(dbref i)
{
    // only display serious warnings
    ct_roomdesc(i);
    ct_nolinked(i);
    ct_security(i);
}

static void ct_normal(dbref i)
{
    ct_playdesc(i);
    ct_roomdesc(i);
    ct_onewayexit(i);
    ct_doubleexit(i);
    ct_exitnames(i);
    ct_nolinked(i);
    ct_security(i);
}

static void ct_extra(dbref i)
{
    ct_roomdesc(i);
    ct_onewayexit(i);
    ct_doubleexit(i);
    ct_playdesc(i);
    ct_exitmsgs(i);
    ct_thngdesc(i);
    ct_thngmsgs(i);
    ct_exitnames(i);
    ct_nolinked(i);
    ct_security(i);
}

static void ct_all(dbref i)
{
    ct_extra(i);
    ct_exitdesc(i);
}

struct tcheck_s
{
  char *name;
  void (*func) P((dbref));
} tchecks[] = {
    // group checks
    {"serious", ct_serious},
    {"normal",  ct_normal},
    {"extra",   ct_extra},
    {"all",     ct_all},

    // now the individual checks
    {"roomdesc",   ct_roomdesc},
    {"onewayexit", ct_onewayexit},
    {"doubleexit", ct_doubleexit},
    {"exitmsgs",   ct_exitmsgs},
    {"exitdesc",   ct_exitdesc},
    {"thngdesc",   ct_thngdesc},
    {"playdesc",   ct_thngdesc},
    {"thngmsgs",   ct_thngmsgs},
    {"exitnames",  ct_exitnames},
    {"nolinked",   ct_nolinked},
    {"security",   ct_security},
    {NULL, NULL}
};

static void check_topology_on(dbref i)
{
    char buf[BUF_SIZE];
    char *x = buf, *y;
  
    strcpy(buf, (*atr_get(db[i].owner, A_WARNINGS)) ? atr_get(db[i].owner, A_WARNINGS) : "normal");

    while ((y = parse_up(&x, ' ')))
    {
        int j;

        for (j = 0; tchecks[j].name; j++)
        {
            if (!string_compare(tchecks[j].name, y))
            {
                (*tchecks[j].func)(i) ;

                break;
            }
        }

        if (!tchecks[j].name && Typeof(i) == TYPE_PLAYER)
        {
            send_message(i, "Unknown warning: %s", y);
        }
    }
}

void run_topology()
{
    int ndone;

    for (ndone = 0; ndone < warning_chunk; ndone++)
    {
        current_object++;

        if (current_object >= db_top)
        {
            current_object = (dbref) 0;
        }

        if (!(db[current_object].flags & GOING))
        {
            if (db[db[current_object].owner].flags & CONNECT)
            {
              	check_topology_on(current_object);
            	ndone += warning_bonus;
            }
            else if (get_pow(db[current_object].owner, POW_MODIFY) != PW_NO)
            {
              	ct_security(current_object);
            	ndone += warning_bonus;
            }
        }
    }
}
