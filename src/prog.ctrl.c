///////////////////////////////////////////////////////////////////////////////
// ctrl.c                                               TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: ctrl.c,v 1.7 1994/02/18 22:45:48 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// file containing programming commands.
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "config.h"
#include "externs.h"

void do_switch(dbref player, char *exp, char *argv[], dbref cause)
{
    char buff[BUF_SIZE];            
    char *ptrsrv[10];
    int any = 0;
    int a;
    int c;

    strcpy(buff,exp);

    if (!argv[1])
    {
        return;
    }

    for (c = 0; c < 10; c++)
    {
        ptrsrv[c] = wptr[c];
    }

    // now try a wild card match of buff with stuff in coms 
    for (a = 1; (a < (MAX_ARG-1)) && argv[a] && argv[a + 1]; a += 2)
    {
        if (wild_match(argv[a],buff))
        {
            any = 1;          

            for (c = 0; c < 10; c++)
            {
                wptr[c] = ptrsrv[c];
            }

            parse_que(player, argv[a+1], cause);
        }                              
    }

    for (c = 0; c < 10; c++)
    {
        wptr[c] = ptrsrv[c];
    }

    if ((a<MAX_ARG) && !any && argv[a])                              
    {
        parse_que(player, argv[a], cause);
    }
}

void do_foreach(dbref player, char *list, char *command, dbref cause)
{
    char buff[BUF_SIZE*2];
    char *ptrsrv[10];
    char *s = buff;
    char *k;
    int c;

    strcpy(buff, list);

    for (c = 0; c < 10; c++)
    {
        ptrsrv[c] = wptr[c];
    }

    for (c = 0;c < 10; c++)
    {
        wptr[c] = "";
    }

    while ((k = parse_up(&s, ' ')))
    {
        wptr[0] = k;
        parse_que(player, command, cause);
    }

    for (c = 0; c < 10; c++)
    {
        wptr[c] = ptrsrv[c];
    }
}

void do_trigger(dbref player, char *object, char *argv[])
{
    dbref thing;
    ATTR *attrib;
    int a;

    if (!parse_attrib(player, object, &thing, &attrib, POW_SEEATR))
    {
        send_message(player, "No match.");
        return;
    }

    if (!controls(player, thing, POW_MODIFY))
    {
        send_message(player, perm_denied());
        return;
    }

    if ( thing == root )
    {
        send_message(player, "root says, \"thou shalt not trigger me.\"");
        return;
    }

    for (a = 0; a < 10; a++)
    {
        wptr[a] = argv[a + 1];
    }

    did_it(player, thing, NULL, 0, NULL, 0, attrib);

    if (!(db[player].flags & QUIET))
    {
        send_message(player, "%s - Triggered.", db[thing].name);
    }
}

void do_trigger_as (dbref player, char *object, char *argv[])
{
    dbref thing;
    dbref cause;
    ATTR *attrib;
    int a;

    if (!argv[1] || !*argv[1] || ((cause = match_thing(player, argv[1])) == NOTHING))
    {
        return;
    }

    if (!parse_attrib(player, object, &thing, &attrib, POW_SEEATR))
    {
        send_message(player, "no match... getting desperate yet?");
        return;
    }

    if (!controls(player,thing,POW_MODIFY))
    {
        send_message(player, perm_denied());
        return;
    }

    if ( thing == root )
    {
        send_message(player, "root says, \"thou shalt not trigger me.\"");
        return;
    }

    for (a = 0; a < 9; a++)
    {
        wptr[a] = argv[a + 2];
    }

    did_it(cause, thing, NULL, 0, NULL, 0, attrib);

    if (!(db[player].flags & QUIET))
    {
        send_message(player, "%s - Triggered.",db[thing].name);
    }
}

void do_decompile(dbref player, char *arg1, char *arg2)
{
    char buf[BUF_SIZE];
    char *s;
    dbref obj;
    ALIST *a;
    ATRDEF *k;

    obj = match_thing (player, arg1);

    if (obj == NOTHING)
    {
        return;
    }

    if ((!controls(player, obj, POW_SEEATR) || !controls(player, obj, POW_EXAMINE)) && !(db[obj].flags & SEE_OK))
    {
        send_message(player, perm_denied());
        return;
    }

    s = flag_description(obj);

    if ((s = strchr(s, ':')) && (s = strchr(++s, ':')))
    {
        char *g;
        s += 2;

        while ((g = parse_up(&s,' ')))
        {
            send_message(player, "@set %s=%s", (*arg2) ? arg2 : arg1, g);
        }
    }

    if (db[obj].atrdefs)
    {
        for (k = db[obj].atrdefs; k; k = k->next)
        {
            sprintf(buf, "@defattr %s/%s%s", (*arg2) ? arg2 : arg1, k->a.name, (k->a.flags) ? "=" : "");

            if (k->a.flags & AF_WIZARD)
                strcat(buf, " wizard");
            if (k->a.flags & AF_UNIMP)
                strcat(buf, " unsaved");
            if (k->a.flags & AF_OSEE)
                strcat(buf, " osee");
            if (k->a.flags & AF_INHERIT)
                strcat(buf, " inherit");
            if (k->a.flags & AF_DARK)
                strcat(buf, " dark");
            if (k->a.flags & AF_DATE)
                strcat(buf, " date");
            if (k->a.flags & AF_LOCK)
                strcat(buf, " lock");
            if (k->a.flags & AF_FUNC)
                strcat(buf, " function");

            send_message(player, buf);
        }
    }

    for (a = db[obj].list; a; a = AL_NEXT(a))
    {
        if (AL_TYPE(a))
        {
            if (!(AL_TYPE(a)->flags & AF_UNIMP))
            {
                if (can_see_atr(player, obj, AL_TYPE(a)))
                {
                    //if ( AL_TYPE(a)->obj == NOTHING )
                    sprintf(buf, "%s", unparse_attr(AL_TYPE(a),0));
                    //else {
                    //  sprintf(buf, "#%d", obj);
                    //  sprintf(buf, "%s%s", db[obj].name, unparse_attr(AL_TYPE(a),0)+strlen(buf));
                    //} 
                    send_message(player, "@nset %s=%s:%s", (*arg2) ? arg2 : arg1, buf, (AL_STR(a)));
	        }
            }
        }
    }
}

void do_cycle(dbref player, char *arg1, char **argv)
{
    ATTR *attrib;
    dbref thing;
    char *curv;
    int i;
  
    if (!parse_attrib(player, arg1, &thing, &attrib, POW_SEEATR))
    {
        send_message(player, "No match.");
        return;
    }

    if (!argv[1])
    {
        send_message(player, "wha? quit that. you're confusing me.");
        return;
    }

    if (!can_set_atr(player,thing, attrib) || attrib == A_ALIAS)
    {
        send_message(player, perm_denied());
        return;
    }

    curv = atr_get (thing, attrib);

    for (i = 1; i < 10 && argv[i]; i++)
    {
        if (!string_compare(curv, argv[i]))
        {
            i++;

            if (!(db[player].flags & QUIET))
            {
                send_message(player,"Cycling...");
            }

            if (i < 10 && argv[i])
            {
                atr_add(thing, attrib, argv[i]);
            }
            else
            {
                atr_add(thing, attrib, argv[1]);
            }

            return;
        }
    }

    if (!(db[player].flags & QUIET))
    {
        send_message(player, "Defaulting to first in cycle...");
    }

    atr_add(thing, attrib, argv[1]);
}
