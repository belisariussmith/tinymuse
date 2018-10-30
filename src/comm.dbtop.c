///////////////////////////////////////////////////////////////////////////////
// dbtop.c 
///////////////////////////////////////////////////////////////////////////////
// $Id: dbtop.c,v 1.4 1993/08/16 01:56:33 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "db.h"
#include "externs.h"

static void shuffle P((int));

static int top[30];
static dbref topp[30];

static void dbtop_internal P((dbref, int (*)(), char *));

static int dt_mem(dbref x)
{
    if (db[x].owner != x)
    {
        return -1;
    }

    return atoi(atr_get(x, A_BYTESUSED));
 
    // Supposed to be used ? -- Belisarius
    //for (dbref i = 0; i < db_top; i++)
    //{
    //    if (db[i].owner == x)
    //    {
    //        tot += mem_usage(i);
    //    }
    //}
    //return tot; 
}

static int dt_cred(dbref x)
{
    return Pennies(x);
}

static int dt_cont(dbref x)
{
    int num = 0;

    if (Typeof(x) == TYPE_EXIT || db[x].contents == NOTHING)
    {
        return -1;
    }

    for (dbref l = db[x].contents; l != NOTHING; l = db[l].next)
    {
        num++;
    }

    return num;
}

static int dt_exits(dbref x)
{
    int num = 0;

    if (Typeof(x)!=TYPE_ROOM || Exits(x) == NOTHING)
    {
        return -1;
    }

    for(dbref l = Exits(x); l != NOTHING; l = db[l].next)
    {
        num++;
    }

    return num;
}

static int dt_quota(dbref x)
{
    if (Typeof(x) != TYPE_PLAYER)
    {
        return -1;
    }

    return atoi(atr_get(x, A_QUOTA));
    //return atoi(atr_get(x, A_RQUOTA))+dt_obj(x); // deprecated? --Belisarius
}

static int dt_obj(register dbref x)
{
    if (Typeof(x) != TYPE_PLAYER)
    {
        return -1;
    }

    // Deprecated? --Belisarius
    //
    //for (y = 0; y < db_top; y++)
    //{
    //    if (db[y].owner == x)
    //    {
    //        num++; 
    //    }
    //}

    return atoi(atr_get(x, A_QUOTA)) - atoi(atr_get(x, A_RQUOTA));
    //return num; // Deprecated? --Belisarius
}

static int dt_numdefs(dbref x)
{
    register ATRDEF *j;
    register int k = 0;

    for (j = db[x].atrdefs; j; j = j->next)
    {
        k++;
    }

    return k;
}

extern int dt_mail P((dbref)); // from mail.c

// dbref(x);
static int dt_connects(dbref x)
{
    return atoi(atr_get(x, A_CONNECTS));
}

void do_dbtop(dbref player, char *arg1)
{
    static struct dbtop_list
    {
        char *nam;
        int (*func) P((dbref));
    }
    *ptr, funcs[]=
    {
        {"numdefs",         dt_numdefs},
        {"credits",         dt_cred},
        {"contents",        dt_cont},
        {"exits",           dt_exits},
        {"quota",           dt_quota},
        {"objects",         dt_obj},
        {"memory",          dt_mem},
        {"mail",            dt_mail},
        {"connects",        dt_connects},
        {0,                 0}
    };

    int nm = 0;

    if (!*arg1)
    {
        arg1 = "foo! this shouldn't match anything";
    }

    for (ptr = funcs; ptr->nam; ptr++)
    {
        if (string_prefix(ptr->nam, arg1) || !strcmp(arg1, "all"))
        {
            nm++;
            //if(!power(player, POW_DBTOP))
            //{
            //    send_message(player, "@dbtop is a restricted command.");
            //    return;
            //}
            dbtop_internal(player, ptr->func, ptr->nam);
        }
    }

    if (nm == 0)
     {
        send_message(player, "Usage: @dbtop all|catagory");
        send_message(player, "catagories are:");

        for(ptr = funcs; ptr->nam; ptr++)
        {
            send_message(player, "  %s",ptr->nam);
        }
    }
}

static void dbtop_internal(dbref player, int (*calc)(), char *nam)
{
    int i, m, j;

    for(j = 0; j < 30; j++)
    {
        //
        // Belisarius
        // Goofy self assignment - likely an artifact
        //
        //top[j]  = top[j]  = (-1);
        //topp[j] = topp[j] = (dbref)0;
        top[j]  = (-1);
        topp[j] = (dbref)0;
    }

    send_message(player, "** %s:", nam);

    for (i = 0;i < db_top; i++)
    {
        m = (*calc)(i);

        if (m > top[28])
        {
            // put it somewhere 
            for (j = 28; (j > 0) && (top[j] < m); j--) ;  // (Belisarius) what is the point?

            j++;
            shuffle(j);
            top[j]  = m;
            topp[j] = i;
        }
    }

    for (j = 1; j < 27; j++)
    {
        send_message(player, "%2d) %s has %d %s", j, unparse_object(player, topp[j]), top[j], nam);
    }
}

static void shuffle(int jj)
{
    if (jj > 28)
    {
        return;
    }

    shuffle(jj + 1);

    top[jj+1]  = top[jj];
    topp[jj+1] = topp[jj];
}
