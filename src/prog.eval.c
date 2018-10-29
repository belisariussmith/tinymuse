///////////////////////////////////////////////////////////////////////////////
// eval.c                                                      TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: eval.c,v 1.16 1993/10/18 01:14:50 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// Expression parsing module created by Lawrence Foard 
///////////////////////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#include "tinymuse.h"
#include "db.h"
#include "interface.h"
#include "config.h"
#include "externs.h"
#include "hash.h"

#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 10
#define MIN_HASH_VALUE 38
#define MAX_HASH_VALUE 676

// External declarations
extern char *type_to_name();

typedef struct fun FUN;
struct fun {char *name; void (*func)(); int nargs;};
static int lev = 0; // the in depth level which we're at. 

dbref match_thing(dbref player, char *name)
{
    init_match(player, name, NOTYPE);
    match_everything();

    return(noisy_match_result());
}

static void fun_rand(char *buff, char *args[10])
{
    int mod = atoi(args[0]);

    if (mod < 1)
    {
        mod = 1;
    }

    sprintf(buff, "%d", (int)((random() & 65535) % mod));
}

static void fun_switch(char *buff, char *args[10], dbref player, dbref doer, int nargs)
{
    char thing[BUF_SIZE];
    char *ptrsrv[10];
    int i;

    if (nargs < 2)
    {
        strcpy(buff, "WRONG NUMBER OF ARGS");
        return;
    }

    for (i = 0; i < 10; i++)
    {
          ptrsrv[i] = wptr[i];
    }

    strcpy(thing, args[0]);

    for (i = 1; (i + 1) < nargs; i += 2)
    {
        if (wild_match(args[i], thing))
        {
            strcpy(buff, args[i+1]);

            for (i = 0; i < 10; i++)
            {
                wptr[i] = ptrsrv[i];
            }

            return;
        }
    }

    if (i < nargs)
    {
        strcpy(buff, args[i]);
    }
    else
    {
        *buff = '\0';
    }

    for (i = 0; i < 10; i++)
    {
        wptr[i] = ptrsrv[i];
    }

    return;
}

static void fun_attropts(char *buff, char *args[10], dbref player, dbref doer, int nargs)
{
    char buf[BUF_SIZE];
    ATTR *attrib;
    dbref thing;

    if (nargs > 2 || nargs < 1)
    {
        strcpy(buff, "WRONG NUMBER OF ARGS");
        return;
    }

    if (nargs == 2)
    {
        char *k;
        static char *g[1];

        sprintf(buf, "%s/%s", args[0], args[1]);
        k = buf;

        args = g;
        g[0] = k;
    }

    if (!parse_attrib(player, args[0], &thing, &attrib, 0))
    {
        strcpy(buff, "NO MATCH");
        return;
    }

    if (!can_see_atr(player, thing, attrib))
    {
        strcpy(buff, "Permission denied.");
        return;
    }

    *buf = '\0';
    buf[1] = '\0';

    if (attrib->flags & AF_WIZARD)
        strcat(buf, " wizard");
    if (attrib->flags & AF_UNIMP)
        strcat(buf, " unsaved");
    if (attrib->flags & AF_OSEE)
        strcat(buf, " osee");
    if (attrib->flags & AF_INHERIT)
        strcat(buf, " inherit");
    if (attrib->flags & AF_DARK)
        strcat(buf, " dark");
    if (attrib->flags & AF_DATE)
        strcat(buf, " date");
    if (attrib->flags & AF_LOCK)
        strcat(buf, " lock");
    if (attrib->flags & AF_FUNC)
        strcat(buf, " function");
    if (attrib->flags & AF_DBREF)
        strcat(buf, " dbref");
    if (attrib->flags & AF_NOMEM)
        strcat(buf, " nomem");
    if (attrib->flags & AF_HAVEN)
        strcat(buf, " haven");

    strcpy (buff, buf+1);
}

static void fun_foreach(char *buff, char *args[10], dbref privs, dbref doer)
{
    char *k;
    char buff1[BUF_SIZE];
    char *ptrsrv;
    int i = 0, j = 0;

    ptrsrv = wptr[0];

    if (!args[0] || !strcmp(args[0], ""))
    {
        buff[0] = '\0';
        return;
    }

    while((k=parse_up(&args[0], ' ')) && i < 1000)
    {
        wptr[0] = k;
        pronoun_substitute(buff1, doer, args[1], privs);

        for (j = strlen(db[doer].name) + 1; buff1[j] && i < 1000; i++, j++)
        {
            buff[i] = buff1[j];
        }

        buff[i++] = ' ';
        buff[i] = '\0';
    }

    if (i > 0)
    {
        buff[i-1] = '\0';
    }

    wptr[0] = ptrsrv;
}

static void fun_get(char *buff, char *args[10], dbref player, dbref doer, int nargs)
{
    char buf[BUF_SIZE];
    ATTR *attrib;
    dbref thing;

    if (nargs > 2 || nargs < 1)
    {
        strcpy(buff, "WRONG NUMBER OF ARGS");
        return;
    }

    if (nargs == 2)
    {
        char *k;
        static char *g[1];

        sprintf(buf, "%s/%s", args[0], args[1]);
        k = buf;
        args = g;
        g[0] = k;
    }

    if (!parse_attrib(player, args[0], &thing, &attrib, 0))
    {
        strcpy(buff, "NO MATCH");
        return;
    }

    if (can_see_atr (player, thing, attrib))
    {
        strcpy(buff, atr_get(thing, attrib));
    }
    else
    {
        strcpy(buff, "Permission denied");
    }

    //if (!controls(player, thing, POW_SEEATR) && !(attrib->flags & AF_OSEE))
    //{
    //    strcpy(buff, "Permission denied");
    //    return;
    //}
   
    //if (attrib->flags & AF_DARK && !controls(player, attrib->obj, POW_SECURITY))
    //{
    //    strcpy(buff, "Permission denied");
    //    return;
    //}

    //strcpy(buff,atr_get(thing,attrib));
}

static void fun_time(char *buff, char *args[10], dbref privs, dbref doer, int nargs)
{
    time_t cl;

    // use supplied x-value if one is given 
    // otherwise get the current x-value of time 
    if ( nargs == 2 )
    {
        cl = atol(args[1]);
    }
    else
    {
        cl = now;
    }

    if ( nargs == 0 )
    {
        strcpy(buff, mktm(cl, "D", privs));
    }
    else
    {
        strcpy(buff, mktm(cl, args[0], privs));
    }
}

static void fun_xtime(char *buff, char *args[], dbref privs, dbref doer, int nargs)
{
    time_t cl;

    if ( nargs == 0 )
    {
        cl = now;
    }
    else
    {
        cl = mkxtime(args[0], privs, (nargs > 1) ? args[1] : "");

        if ( cl == -1L )
        {
            strcpy(buff,"#-1");
            return;
        }
    }

    sprintf(buff, "%ld", cl);
}

int mem_usage(dbref objectId)
{
    ALIST *attributeListItem;
    ATRDEF *attribute;
    int sizeOfObject;

    // Begin with the size of an object
    sizeOfObject = sizeof(struct object);
    // Now increase by the object's name (+1 for buffer)
    sizeOfObject += strlen(db[objectId].name)+1;

    // Loop through the item's (A)ttribute lists
    for (attributeListItem = db[objectId].list; attributeListItem; attributeListItem = AL_NEXT(attributeListItem))
    {
        // Check for a real attribute list
        if (AL_TYPE(attributeListItem))
        {
            // Ignore DOOMSDAY, BYTESUSED, and IT attribute list types
            if (AL_TYPE(attributeListItem) != A_DOOMSDAY && AL_TYPE(attributeListItem) != A_BYTESUSED && AL_TYPE(attributeListItem) != A_IT)
            {
                // Increase by the size of the attribute list
                sizeOfObject += sizeof(ALIST);

                // Does the attribute list have a string name
                if (AL_STR(attributeListItem))
                {
                    // Attribute has a string name, increase by that size as well
                    sizeOfObject += strlen(AL_STR(attributeListItem));
                }
            }
        }
    }

    // Loop through the defined attributes
    for (attribute = db[objectId].atrdefs; attribute; attribute = attribute->next)
    {
        // Increase by the size an ATRDEF
        sizeOfObject += sizeof(ATRDEF);

        // Check to see if the attribute has been named
        if (attribute->a.name)
        {
            // If the aittibute has been named, then increase the size of the string
            sizeOfObject += strlen(attribute->a.name);
        }
    }

    return sizeOfObject;
}

static void fun_objmem(char *buff, char *args[10], dbref privs)
{
    dbref thing;
    thing = match_thing (privs, args[0]);

    if (thing == NOTHING || !controls (privs, thing, POW_STATS))
    {
        strcpy(buff,"#-1");
        return;
    }

    sprintf(buff, "%d", mem_usage(thing));
}

static void fun_playmem (char *buff, char *args[10], dbref privs)
{
    int tot = 0;
    dbref thing;
    dbref j;

    thing = match_thing (privs, args[0]);

    if (thing == NOTHING || !controls (privs, thing, POW_STATS) || !power (privs, POW_STATS))
    {
        strcpy(buff,"#-1");
        return;
    }

    for (j = 0; j < db_top; j++)
    {
        if (db[j].owner == thing)
        {
            tot += mem_usage (j);
        }
    }

    sprintf (buff,"%d",tot);
}

static void fun_mid(char *buff, char *args[10])
{
    int l   = atoi(args[1]);
    int len = atoi(args[2]);

    if ((l < 0) || (len < 0) || ((len + l) > 1000))
    {
        strcpy(buff,"OUT OF RANGE");
        return;
    }

    if (l < strlen(args[0]))
    {
        strcpy(buff, args[0]+l);
    }
    else
    {
        *buff = 0;
    }

    buff[len] = 0;
}


static void fun_add(char *buff, char *args[10])
{
    sprintf(buff, "%d", atoi(args[0]) + atoi(args[1]));
}

static void fun_band(char *buff, char *args[10])
{
    sprintf(buff, "%d", atoi(args[0]) & atoi(args[1]));
}

static void fun_bor(char *buff, char *args[10])
{
    sprintf(buff, "%d", atoi(args[0]) | atoi(args[1]));
}

static void fun_bxor(char *buff, char *args[10])
{
    sprintf(buff, "%d", atoi(args[0]) ^ atoi(args[1]));
}

static void fun_bnot(char *buff, char *args[10])
{
    sprintf(buff, "%d", ~atoi(args[0]));
}

static int istrue(char *str)
{
    return (((strcmp(str,"#-1") == 0) || (strcmp(str,"") == 0) ||
       (strcmp(str,"#-2") == 0) ||
       ((atoi(str) == 0) && isdigit(str[0]))) ? 0 : 1);
}

static void fun_land(char *buff, char *args[10])
{
    sprintf(buff, "%d", istrue(args[0]) && istrue(args[1]));
}

static void fun_lor(char *buff, char *args[10])
{
    sprintf(buff, "%d", istrue(args[0]) || istrue(args[1]));
}

static void fun_lxor(char *buff, char *args[10])
{
    sprintf(buff,"%d",(istrue(args[0]) == 0 ? (istrue(args[1]) == 0 ? 0 : 1) :
             (istrue(args[1]) == 0 ? 1 : 0)));
}

static void fun_lnot(char *buff, char *args[10])
{
    sprintf(buff, "%d", (istrue(args[0]) == 0 ? 1 : 0));
}

static void fun_truth(char *buff, char *args[10])
{
    sprintf(buff,"%d", (istrue(args[0]) ? 1 : 0));
}

static void fun_base(char *buff, char *args[10])
{
    int i, digit, decimal, neg, oldbase, newbase;
    char tmpbuf[BUF_SIZE];

    oldbase = atoi(args[1]);
    newbase = atoi(args[2]);

    if ((oldbase < 2) || (oldbase > 36) || (newbase < 2) || (newbase > 36))
    {
        strcpy(buff, "BASES MUST BE BETWEEN 2 AND 36");
        return;
    }

    neg = 0;

    if (args[0][0]=='-')
    {
        neg = 1;
        args[0][0] = '0';
    }

    decimal = 0;

    for (i = 0; args[0][i] != 0; i++)
    {

        decimal *= oldbase;

        if (('0' <= args[0][i]) && (args[0][i] <= '9'))
            digit = args[0][i] - '0';
        else if (('a' <= args[0][i]) && (args[0][i] <= 'z'))
            digit = args[0][i] + 10 - 'a';
        else if (('A' <= args[0][i]) && (args[0][i] <= 'Z'))
            digit = args[0][i] + 10  - 'A';
        else {
            strcpy(buff, "ILLEGAL DIGIT");
            return;
        }

        if (digit >= oldbase) {
            strcpy(buff, "DIGIT OUT OF RANGE");
            return;
        }
        decimal += digit;
    }

    strcpy(buff,"");
    strcpy(tmpbuf,"");

    i = 0;

    while (decimal > 0)
    {
        strcpy(tmpbuf,buff);
        digit = (decimal % newbase);

        if (digit < 10)
        {
            sprintf(buff,"%d%s", digit, tmpbuf);
        }
        else
        {
            sprintf(buff,"%c%s", digit+'a'-10, tmpbuf);
        }

        decimal /= newbase;
        i++;
    }

    if (neg)
    {
        strcpy(tmpbuf,buff);
        sprintf(buff,"-%s",tmpbuf);
    }
}

static void fun_sgn(char *buff, char *args[10])
{
    sprintf(buff,"%d",atoi(args[0]) > 0 ? 1 : atoi(args[0]) < 0 ? -1 : 0);
}

static void fun_sqrt(char *buff, char *args[10])
{
    extern double sqrt P((double));
    int k;

    k = atoi(args[0]);

    if (k < 0)
    {
        k = (-k);
    }

    sprintf(buff, "%d", (int)sqrt((double)k));
}

static void fun_abs(char *buff, char *args[10])
{
    extern int abs P((int));

    sprintf(buff, "%d", abs(atoi(args[0])));
}

static void fun_mul(char *buff, char *args[10])
{
    sprintf(buff, "%d", atoi(args[0])*atoi(args[1]));
}

static void fun_div(char *buff, char *args[10])
{
    int bot = atoi(args[1]);

    if (bot == 0)
    {
        bot = 1;
    }

    sprintf(buff, "%d", atoi(args[0])/bot);
}

static void fun_mod(char *buff, char *args[10])
{
    int bot = atoi(args[1]);

    if (bot == 0)
    {
        bot = 1;
    }

    sprintf(buff, "%d", atoi(args[0]) % bot);
}

// read first word from a string 
static void fun_first(char *buff, char *args[10])
{
    char *s = args[0];
    char *b;

    // get rid of leading space 
    while(*s && (*s == ' '))
    {
        s++;
    }

    b = s;

    while(*s && (*s != ' '))
    {
        s++;
    }

    *s++=0;
    strcpy(buff, b);
}

static void fun_rest(char *buff, char *args[10])
{
    char *s = args[0];

    // skip leading space 
    while(*s && (*s == ' '))
    {
        s++;
    }

    // skip firsts word 
    while(*s && (*s != ' '))
    {
        s++;
    }

    // skip leading space 
    while(*s && (*s == ' '))
    {
        s++;
    }

    strcpy(buff, s);
}

static void fun_flags(char *buff, char *args[10], dbref privs, dbref owner)
{
    dbref thing;
    int oldflags; // really kludgy. 

    init_match(privs, args[0], NOTYPE);
    match_me();
    match_here();
    match_neighbor();
    match_absolute();
    match_possession();
    match_player();
    thing = match_result();

    if ( thing == NOTHING || thing == AMBIGUOUS )
    {
        *buff = '\0';
        return;
    }

    //if ( ! controls(owner, thing, POW_FUNCTIONS) )
    //{
    //    *buff = '\0';
    //    return;
    //}

    oldflags = db[thing].flags;    // grr, this is kludgy. 

    if (!controls(privs,thing,POW_WHO) && !could_doit(privs, thing, A_LHIDE))
    {
        db[thing].flags &= ~(CONNECT);
    }

    strcpy(buff, unparse_flags(thing));
    db[thing].flags = oldflags;
}

static void fun_mtime(char *buff, char *args[10], dbref player)
{
    dbref thing;

    thing = match_thing(player, args[0]);

    if (thing == NOTHING)
    {
        strcpy(buff,"#-1");
    }
    else
    {
        sprintf(buff, "%ld", db[thing].mod_time);
    }
}

static void fun_ctime(char *buff, char *args[10], dbref player)
{
    dbref thing;

    thing = match_thing(player, args[0]);

    if (thing == NOTHING)
    {
        strcpy(buff, "#-1");
    }
    else
    {
        sprintf(buff, "%ld", db[thing].create_time);
    }
}


static void fun_credits(char *buff, char *args[10], dbref player)
{
    dbref who;

    init_match(player, args[0], TYPE_PLAYER);

    match_me();
    match_player();
    match_neighbor();
    match_absolute();

    if ( (who = match_result()) == NOTHING )
    {
        strcpy(buff, "#-1");
        return;
    }

    if (!power(player, POW_FUNCTIONS) && !controls(player, who, POW_FUNCTIONS) )
    {
        strcpy(buff, "#-1");
        return;
    }

    sprintf(buff, "%d", Pennies(who));
}

static void fun_quota_left(char *buff, char *args[10], dbref player)
{
    dbref who;

    init_match(player, args[0], TYPE_PLAYER);

    match_me();
    match_player();
    match_neighbor();
    match_absolute();

    if ( (who = match_result()) == NOTHING )
    {
        strcpy(buff, "#-1");
        return;
    }

    if ( ! controls(player, who,POW_FUNCTIONS) )
    {
        strcpy(buff, "#-1");
        return;
    }

    sprintf(buff, "%d", atoi(atr_get(who, A_RQUOTA)));
}

static void fun_quota(char *buff, char *args[10], dbref player)
{
    dbref who;

    init_match(player, args[0], TYPE_PLAYER);

    match_me();
    match_player();
    match_neighbor();
    match_absolute();

    if ( (who = match_result()) == NOTHING )
    {
        strcpy(buff, "#-1");
        return;
    }

    if ( ! controls(player, who, POW_FUNCTIONS) )
    {
        strcpy(buff, "#-1");
        return;
    }

    // count up all owned objects 
    //owned = -1;  * a player is never included in his own quota 
    //for ( thing = 0; thing < db_top; thing++ )
    //{
    //    if ( db[thing].owner == who )
    //    {
    //        if ((db[thing].flags & (TYPE_THING|GOING)) != (TYPE_THING|GOING))
    //        {
    //            ++owned;
    //        }
    //    }
    //}

    strcpy(buff, atr_get(who, A_QUOTA));

    //sprintf(buff, "%d", (owned + atoi(atr_get(who, A_RQUOTA))));
}

static void fun_strlen(char *buff, char *args[10])
{
    sprintf(buff, "%d", (int)strlen(args[0]));
}

static void fun_comp(char *buff, char *args[10])
{
    int x;

    x = (atoi(args[0]) - atoi(args[1]));

    if ( x > 0 )
    {
        strcpy(buff, "1");
    }
    else if ( x < 0 )
    {
        strcpy(buff, "-1");
    }
    else
    {
        strcpy(buff, "0");
    }
}

static void fun_scomp(char *buff, char *args[10])
{
    int x;

    x = strcmp(args[0], args[1]);

    if ( x > 0 )
    {
        strcpy(buff, "1");
    }
    else if ( x < 0 )
    {
        strcpy(buff, "-1");
    }
    else
    {
        strcpy(buff, "0");
    }
}

// handle 0-9,va-vz,n,# 
static void fun_v(char *buff, char *args[10], dbref privs, dbref doer)
{
    int c;

    if (args[0][0] && args[0][1])
    {
        // not a null or 1-character string. 
        ATTR *attrib;

        if (!(attrib=atr_str(privs,privs,args[0])) || !can_see_atr(privs, privs, attrib))
        {
            *buff = '\0';
            return;
        }

        strcpy(buff,atr_get(privs,attrib));

        return;
    }

    switch(c = args[0][0])
    {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6':
        case '7': case '8': case '9':
            if (!wptr[c-'0'])
            {
                *buff = 0;
                return;
            }

            strcpy(buff,wptr[c-'0']);
            break;
        case 'v':
        case 'V':
            {
                int a;

                a = to_upper(args[0][1]);

                if (( a < 'A') || (a > 'Z'))
                {
                    *buff = 0;
                    return;
                }

                strcpy(buff, atr_get(privs, A_V[a-'A']));
            }
            break;
        case 'n':
        case 'N':
            strcpy(buff, safe_name(doer));
            break;
        case '#':
            sprintf(buff, "#%d", doer);
            break;
            // info about location and stuff 
            // objects # 
        case '!':
            sprintf(buff, "#%d", privs);
            break;
        case 'a':
        case 'A':
            if (power(doer, POW_BEEP))
            {
                sprintf(buff, "\a");
            }
            else
            {
                *buff = '\0';
            }
            break;
        case 'r':
        case 'R':
            sprintf(buff,"\n");
            break;
        case 't':
        case 'T':
            sprintf(buff,"\t");
            break;
        default:
            *buff = '\0';
            break;
    }
}

static void fun_s(char *buff, char *args[10], dbref privs, dbref doer)
{
    char buff1[BUF_SIZE];

    pronoun_substitute(buff1,doer,args[0],privs);
    strcpy(buff,buff1+strlen(db[doer].name)+1);
}

static void fun_s_with(char *buff, char *args[10], dbref privs, dbref doer, int nargs)
{
    char buff1[BUF_SIZE];
    char *tmp[10];
    int a;

    if (nargs < 1)
    {
        strcpy(buff, "#-1");
        return;
    }

    for (a = 0; a < 10; a++)
        tmp[a] = wptr[a];

    wptr[9] = 0;

    for (a = 1; a < 10; a++)
        wptr[a-1] = args[a];

    for (a = nargs; a < 10; a++)
        wptr[a-1] = 0;

    pronoun_substitute(buff1,doer,args[0],privs);
    strcpy(buff,buff1+strlen(db[doer].name)+1);

    for (a = 0; a < 10; a++)
        wptr[a] = tmp[a];
}

static void fun_s_as(char *buff, char *args[10], dbref privs, dbref doer)
{
    char buff1[BUF_SIZE];
    dbref newprivs;
    dbref newdoer;

    newdoer = match_thing(privs, args[1]);
    newprivs = match_thing(privs, args[2]);

    if (newdoer == NOTHING || newprivs == NOTHING)
    {
        strcpy(buff,"#-1");
        return;
    }

    if (!controls(privs, newprivs, POW_MODIFY))
    {
        strcpy(buff,"Permission denied.");
        return;
    }

    pronoun_substitute(buff1,newdoer, args[0], newprivs);
    strcpy(buff,buff1+strlen(db[newdoer].name)+1);
}

static void fun_s_as_with(char *buff, char *args[10], dbref privs, dbref doer, int nargs)
{
    char buff1[BUF_SIZE];
    char *tmp[10];
    int a;
    dbref newprivs;
    dbref newdoer;

    if (nargs < 3)
    {
        strcpy(buff, "#-1");
        return;
    }

    newdoer = match_thing(privs, args[1]);
    newprivs = match_thing(privs, args[2]);

    if (newdoer == NOTHING || newprivs == NOTHING)
    {
        strcpy(buff, "#-1");
        return;
    }

    if (!controls(privs, newprivs, POW_MODIFY))
    {
        strcpy(buff, "Permission denied.");
        return;
    }

    for (a=0; a<10; a++)
    {
        tmp[a] = wptr[a];
    }

    wptr[9] = wptr[8] = wptr[7] = 0;

    for (a = 3; a < 10; a++)
    {
        wptr[a-3] = args[a];
    }

    for (a = nargs; a < 10; a++)
    {
        wptr[a-3] = 0;
    }

    pronoun_substitute(buff1,newdoer, args[0], newprivs);
    strcpy(buff,buff1+strlen(db[newdoer].name)+1);

    for (a = 0; a < 10; a++)
    {
        wptr[a] = tmp[a];
    }
}

static void fun_num(char *buff, char *args[], dbref privs)
{
    sprintf(buff, "#%d", match_thing(privs, args[0]));
}

static void fun_con(char *buff, char *args[], dbref privs, dbref doer)
{
    dbref it = match_thing(privs, args[0]);

    if ((it != NOTHING) && (controls(privs,it,POW_FUNCTIONS) || (db[privs].location==it) || (it==doer)))
    {
        if (controls(privs, it, POW_EXAMINE))
        {
            sprintf(buff,"#%d",db[it].contents);
        }
        else
        {
            strcpy(buff, "\0");
        }

        return;
    }

    strcpy(buff,"#-1");

    return;
}

// return next exit that is ok to see 
static dbref next_exit(dbref player, dbref this)
{
    while ( (this!=NOTHING) &&
    (db[this].flags & DARK) && !controls(player,this,POW_FUNCTIONS) )
    {
        this = db[this].next;
    }

    return(this);
}

static void fun_exit(char *buff, char *args[], dbref privs, dbref doer)
{
    dbref it = match_thing(privs, args[0]);

    if ((it != NOTHING) && (controls(privs, it, POW_FUNCTIONS) || (db[privs].location == it) || (it == doer)))
    {
        sprintf(buff, "#%d", next_exit(privs, db[it].exits));
        return;
    }

    strcpy(buff,"#-1");

    return;
}

static void fun_next(char *buff, char *args[], dbref privs, dbref doer)
{
    dbref it = match_thing(privs, args[0]);

    if (it != NOTHING)
    {
        if (Typeof(it) != TYPE_EXIT)
        {
            if ( controls(privs,db[it].location,POW_FUNCTIONS) ||
              (db[it].location==doer) || (db[it].location==db[privs].location) )
            {
                sprintf(buff, "#%d", db[it].next);
                return;
             }
        }
        else
        {
            sprintf(buff, "#%d", next_exit(privs, db[it].next));
            return;
        }
    }

    strcpy(buff,"#-1");

    return;
}

static void fun_loc(char *buff, char *args[], dbref privs, dbref doer)
{
    dbref it = match_thing(privs, args[0]);

    if ((it!=NOTHING) &&
      (controls(privs,it,POW_FUNCTIONS) ||
       controls(privs,db[it].location,POW_FUNCTIONS) ||
       controls_a_zone(privs,it,POW_FUNCTIONS) ||
       power(privs,POW_FUNCTIONS) ||
       (it==doer) ||
       (Typeof(it) == TYPE_PLAYER && !(db[it].flags&DARK))))
    {
        sprintf(buff, "#%d", db[it].location);
        return;
    }

    strcpy(buff, "#-1");

    return;
}

static void fun_link(char *buff, char *args[], dbref privs, dbref doer)
{
    dbref it = match_thing(privs, args[0]);

    if ((it != NOTHING) &&
     (controls(privs,it,POW_FUNCTIONS) ||
      controls(privs,db[it].location,POW_FUNCTIONS) || (it==doer)))
    {
        sprintf(buff, "#%d", db[it].link);
        return;
    }

    strcpy(buff, "#-1");

    return;
}

static void fun_linkup(char *buff, char *args[10], dbref privs)
{
    dbref i;
    dbref it = match_thing(privs, args[0]);

    int len = 0;

    if (!(controls(privs, it, POW_FUNCTIONS) || controls(privs, db[it].location, POW_FUNCTIONS) || (it == privs)))
    {
        strcpy(buff, "#-1");
        return;
    }
    *buff = '\0';

    for (i = 0; i < db_top; i++)
    {
        if (db[i].link == it)
        {
            if(len)
            {
                static char smbuf[30];
                sprintf(smbuf, " #%d", i);

                if ((strlen(smbuf) + len) > 990)
                {
                    strcat(buff," #-1");
                    return;
                }
                strcat (buff, smbuf);
                len += strlen(smbuf);
            }
            else
            {
                sprintf(buff, "#%d", i);
                len = strlen(buff);
            }
        }
    }
}

static void fun_class(char *buff, char *args[], dbref privs)
{
    dbref it = match_thing(privs, args[0]);

    if (it == NOTHING || !power(privs, POW_WHO))
    {
        *buff = '\0';
    }
    else
    {
        strcpy(buff,get_class(it));
    }
}

static void fun_is_a(char *buff, char *args[], dbref privs)
{
    dbref thing, parent;

    thing  = match_thing(privs, args[0]);
    parent = match_thing(privs, args[1]);

    if (thing == NOTHING || parent == NOTHING)
    {
        strcpy(buff, "#-1");
    }
    else
    {
        strcpy(buff, is_a(thing, parent) ? "1" : "0");
    }
}

static void fun_has(char *buff, char *args[], dbref privs)
{
    dbref user, obj;
    dbref i;

    user = match_thing(privs,args[0]);
    obj  = match_thing(privs,args[1]);

    if (obj == NOTHING || user == NOTHING)
    {
        strcpy(buff, "#-1");
    }
    else
    {
        strcpy(buff, "0");

        for (i = db[user].contents; i != NOTHING; i = db[i].next)
        {
            if (i == obj)
            {
                strcpy(buff, "1");
            }
        }
    }
}

static void fun_has_a(char *buff, char *args[], dbref privs)
{
    dbref user, parent;
    dbref i;

    user   = match_thing(privs,args[0]);
    parent = match_thing(privs,args[1]);

    if (parent == NOTHING || user == NOTHING)
    {
        strcpy(buff, "#-1");
    }
    else
    {
        strcpy(buff,"0");

        for (i = db[user].contents; i != NOTHING; i = db[i].next)
        {
            if (is_a(i,parent))
            {
                strcpy(buff,"1");
            }
        }
    }
}

static void fun_owner(char *buff, char *args[], dbref privs)
{
    dbref it = match_thing(privs, args[0]);

    if (it != NOTHING)
    {
        it = db[it].owner;
    }

    sprintf(buff, "#%d", it);
}

static void fun_name(char *buff, char *args[], dbref privs)
{
    dbref it = match_thing(privs, args[0]);

    if (it == NOTHING)
    {
        strcpy(buff, "");
    }
    else
    {
        if (Typeof(it) == TYPE_EXIT)
        {
            strcpy(buff, main_exit_name(it));
        }
        else
        {
            strcpy(buff, db[it].name);
        }
    }
}

static void fun_pos(char *buff, char *args[10])
{
    int i = 1;
    char *t, *u, *s = args[1];

    while ( *s )
    {
        u = s;
        t = args[0];

        while ( *t && *t == *u )
            ++t, ++u;

        if ( *t == '\0' )
        {
            sprintf(buff, "%d", i);
            return;
        }

        ++i, ++s;
    }

    strcpy(buff, "0");

    return;
}

static void fun_delete(char *buff, char *args[10])
{
    char *s = buff, *t = args[0];
    int i, l = atoi(args[1]), len = atoi(args[2]);
    int a0len = strlen(args[0]);

    if ( (l < 0) || (len < 0) || (len+l >= 1000) )
    {
        strcpy(buff,"OUT OF RANGE");
        return;
    }

    for (i = 0; i < l && *s; i++)
        *s++ = *t++;

    if (len+l >= a0len)
    {
        *s = '\0';
        return;
    }

    t += len;

    while((*s++ = *t++))
    ;

    return;
}

static void fun_remove(char *buff, char *args[10])
{
    char *s = buff, *t = args[0];
    int w = atoi(args[1]), num = atoi(args[2]), i;

    if (w < 1)
    {
        strcpy(buff, "OUT OF RANGE");
        return;
    }

    for (i = 1; i < w && *t; i++)
    {
        while(*t && *t != ' ') *s++ = *t++;
        while(*t && *t == ' ') *s++ = *t++;
    }

    for(i = 0; i < num && *t; i++)
    {
        while(*t && *t != ' ') t++;
        while(*t && *t == ' ') t++;
    }

    if (!*t)
    {
        if (s != buff)
        {
            s--;
        }

        *s = '\0';

        return;
    }

    while ((*s++ = *t++));

    return;
}

static void fun_match(char *buff, char *args[10])
{
    int a;
    int wcount = 1;
    char *s = args[0];
    char *ptrsrv[10];

    for ( a = 0; a < 10; a++)
    {
        ptrsrv[a] = wptr[a];
    }

    do
    {
        char *r;

        // trim off leading space 
        while ( *s && (*s == ' ') )
        {
            s++;
        }

        r=s;

        // scan to next space and truncate if necessary 
        while ( *s && (*s != ' ') )
        {
            s++;
        }

        if ( *s )
        {
            *s++ = 0;
        }

        if ( wild_match(args[1], r) )
        {
            sprintf(buff, "%d", wcount);
            for( a = 0; a < 10; a++ )
                wptr[a] = ptrsrv[a];
            return;
        }

        wcount++;
    }

    while(*s)
    ;

    strcpy(buff, "0");

    for (a = 0; a < 10; a++)
    {
        wptr[a] = ptrsrv[a];
    }
}

static void fun_extract(char *buff, char *args[10])
{
    int start = atoi(args[1]), len = atoi(args[2]);
    char *s = args[0], *r;

    if ((start < 1) || (len < 1))
    {
        *buff = 0;
        return;
    }

    start--;

    while(start && *s)
    {
        while(*s && (*s ==' '))
        {
            s++;
        }

        while(*s && (*s !=' '))
        {
            s++;
        }

        start--;
    }

    while(*s && (*s == ' '))
    {
        s++;
    }

    r = s;

    while(len && *s)
    {
        while(*s && (*s == ' '))
        {
            s++;
        }
        while(*s && (*s != ' '))
        {
            s++;
        }

        len--;
    }

    *s = 0;
    strcpy(buff, r);
}

static void fun_parents(char *buff, char *args[10], dbref privs)
{
    dbref it;
    int i;

    it = match_thing(privs, args[0]);

    if (it == NOTHING)
    {
        strcpy(buff, "#-1");
        return;
    }

    *buff = '\0';

    for (i = 0; db[it].parents && db[it].parents[i] != NOTHING; i++)
    {
        if ( controls(privs, it, POW_EXAMINE)
          || controls(privs, it, POW_FUNCTIONS)
          || controls(privs, db[it].parents[i], POW_EXAMINE)
          || controls(privs, db[it].parents[i], POW_FUNCTIONS))
        {
            if (*buff)
            {
                sprintf(buff+strlen(buff)," #%d",db[it].parents[i]);
            }
            else
            {
                sprintf(buff,"#%d",db[it].parents[i]);
            }
        }
    }
}

static void fun_children(char *buff, char *args[10], dbref privs)
{
    dbref it;
    int i;

    it = match_thing(privs, args[0]);

    if (it == NOTHING)
    {
        strcpy(buff, "#-1");
        return;
    }

    *buff = '\0';

    for (i = 0; db[it].children && db[it].children[i] != NOTHING; i++)
    {
        if (controls (privs, it, POW_EXAMINE) || controls(privs, it, POW_FUNCTIONS) || controls (privs, db[it].children[i], POW_EXAMINE) || controls(privs, db[it].children[i], POW_FUNCTIONS))
        {

            if (*buff)
            {
                sprintf(buff + strlen(buff), " #%d", db[it].children[i]);
                buff[990] = '\0';
            }
            else
            {

                sprintf(buff, "#%d", db[it].children[i]);
            }
        }
    }
}

static void fun_zone(char *buff, char *args[10], dbref privs)
{
    dbref thing;

    thing = match_thing (privs, args[0]);

    if (thing == NOTHING)
    {
        strcpy(buff, "#-1");
        return;
    }

    sprintf(buff, "#%d", db[thing].zone);
}

static void fun_getzone(char *buff, char *args[10], dbref privs)
{
    dbref thing;

    thing = match_thing (privs, args[0]);

    if (thing == NOTHING)
    {
        strcpy(buff, "#-1");
        return;
    }

    sprintf(buff, "#%d", get_zone_first(thing));
}

static void fun_wmatch(char *buff, char *args[10])
{
    char *string = args[0], *word = args[1], *s, *t;
    int count = 0, done = 0;

    for (s = string; *s && !done; s++)
    {
        count++;

        while (isspace(*s) && *s) s++;

        t = s;

        while (!isspace(*t) && *t) t++;

        done = (!*t) ? 1 : 0;
        *t = '\0';

        if (!string_compare(s, word))
        {
            sprintf(buff, "%d", count);
            return;
        }

        s = t;
    }

    sprintf(buff, "0");

    return;
}

static void fun_inzone(char *buff, char *args[10], dbref privs)
{
    dbref it = match_thing(privs, args[0]);
    dbref i;
    int len = 0;

    if (!controls(privs, it, POW_EXAMINE))
    {
        strcpy(buff, "#-1");
        return;
    }
    *buff = '\0';

    for (i = 0; i < db_top; i++)
    {
        if (Typeof(i) == TYPE_ROOM)
        {
            if (is_in_zone (i, it))
            {
                if(len)
                {
                    static char smbuf[30]; /* eek, i hope it's not gunna be this long */
                    sprintf(smbuf, " #%d", i);
                    if ((strlen(smbuf) + len) > 990)
                    {
                        strcat(buff, " #-1");
                        return;
                    }
                    strcat (buff, smbuf);
                    len += strlen(smbuf);
                }
                else
                {
                    sprintf(buff, "#%d", i);
                    len = strlen(buff);
                }
            }
        }
    }
}

static void fun_zwho(char *buff, char *args[10], dbref who)
{
    dbref it = match_thing(who, args[0]);
    dbref i;
    int len = 0;

    if (!controls(who, it, POW_FUNCTIONS))
    {
        strcpy(buff, "#-1");
        return;
    }
    *buff = '\0';

    for (i = 0; i < db_top; i++)
    {
        if (Typeof(i) == TYPE_PLAYER)
        {
            if (is_in_zone(i, it))
            {
                if(len)
                {
                    static char smbuf[30]; /* eek, i hope it's not gunna be this long */
                    sprintf(smbuf, " #%d", i);

                    if ((strlen(smbuf) + len) > 990)
                    {
                        strcat(buff," #-1");
                        return;
                    }
                    strcat (buff, smbuf);
                    len += strlen(smbuf);
                }
                else
                {
                    sprintf(buff, "#%d", i);
                    len = strlen(buff);
                }
            }
        }
    }
}

static void fun_objlist(char *buff, char *args[10], dbref privs, dbref doer)
{
    char buf[BUF_SIZE];
    dbref it = match_thing(privs, args[0]);

    *buff = '\0';

    if (it == NOTHING)
    {
        return;
    }

    if (Typeof(it) != TYPE_EXIT)
    {
        if (!(controls(privs, db[it].location, POW_FUNCTIONS) ||
      (db[it].location == doer) ||
      (db[it].location == db[privs].location) ||
      (db[it].location == privs)))
        {
            return;
        }
    }

    while (it != NOTHING)
    {
        if (*buff)
        {
            sprintf(buf, "%s #%d",buff, it);
            strcpy(buff, buf);
        }
        else
        {
            sprintf(buff, "#%d", it);
        }

        it = (Typeof(it) == TYPE_EXIT) ? next_exit(privs, db[it].next) : db[it].next;
    }
}

static void fun_lzone(char *buff, char *args[10], dbref privs, dbref doer)
{
    char buf[BUF_SIZE];
    dbref it = match_thing(privs, args[0]);
    int depth=10;

    *buff = '\0';

    if (it == NOTHING)
    {
        return;
    }

    it = get_zone_first(it);

    while (it != NOTHING)
    {
        if (*buff)
        {
            sprintf(buf, "%s #%d", buff, it);
            strcpy(buff, buf);
        }
        else
        {
            sprintf(buff,"#%d", it);
        }

        it = get_zone_next(it);
        depth--;

        if (depth <= 0)
        {
            return;
        }
    }
}

static void fun_strcat(char *buff, char *args[10])
{
    char buf[BUF_SIZE];

    sprintf(buf, "%s%s", args[0], args[1]);
    strcpy(buff, buf);
}

static void fun_controls(char *buff, char *args[10], dbref privs)
{
    dbref player, object;
    ptype power;

    player = match_thing(privs, args[0]);
    object = match_thing(privs, args[1]);
    power  = name_to_pow(args[2]);

    if (player == NOTHING || object == NOTHING || !power)
    {
        strcpy(buff, "#-1");
        return;
    }

    sprintf(buff, "%d", controls(player, object, power));
}

static void fun_entrances(char *buff, char *args[10], dbref privs)
{
    char buf[BUF_SIZE];

    dbref it = match_thing(privs, args[0]);
    dbref i;
    int control_here;

    *buff = '\0';

    if (it == NOTHING)
    {
        strcpy(buff, "#-1");
    }
    else
    {
        control_here = controls(privs, it, POW_EXAMINE);

        for (i = 0; i < db_top; i++)
        {
            if (Typeof(i) == TYPE_EXIT && db[i].link == it)
            {
                if (controls(privs, i, POW_FUNCTIONS) || controls(privs,i, POW_EXAMINE) || control_here)
                {
                    if (*buff)
                    {
                        sprintf("%s #%d", buff, i);
                        strcpy(buff, buf);
                    }
                    else
                    {
                        sprintf(buff,"#%d", i);
                    }
                }
            }
        }
    }
}

static void fun_fadd(char *buff, char *args[10])
{
    sprintf (buff, "%f", atof(args[0])+atof(args[1]));
}

static void fun_fsub(char *buff, char *args[10])
{
    sprintf (buff, "%f", atof(args[0])-atof(args[1]));
}

static void fun_sub(char *buff, char *args[10])
{
    sprintf (buff, "%d", atoi(args[0])-atoi(args[1]));
}

static void fun_fmul(char *buff, char *args[10])
{
    sprintf (buff, "%f", atof(args[0])*atof(args[1]));
}

static void fun_fdiv(char *buff, char *args[10])
{
    if (atof(args[1]) == 0)
    {
        sprintf (buff, "Error - division by 0.");
    }
    else
    {
        sprintf (buff, "%f", atof(args[0])/atof(args[1]));
    }
}

static void fun_fsgn(char *buff, char *args[10])
{
    if (atof(args[0]) < 0)
    {
        sprintf(buff, "-1");
    }
    else if (atof(args[0]) > 0)
    {
        sprintf(buff, "1");
    }
    else
    {
        strcpy(buff, "0");
    }
}

static void fun_fsqrt(char *buff, char *args[10])
{
    sprintf(buff, "%f", sqrt(atof(args[0])));
}

static void fun_fabs(char *buff, char *args[10])
{
    if (atof(args[0]) < 0)
    {
        sprintf(buff, "%f", -atof(args[0]));
    }
    else
    {
        strcpy(buff, args[0]);
    }
}

static void fun_fcomp(char *buff, char *args[10])
{
    char buf[90];
    char *k = buf;

    sprintf(buf, "%f", atof(args[0])-atof(args[1]));

    fun_fsgn(buff, &k);
}

static void fun_exp(char *buff, char *args[10])
{
    if (atof(args[0]) > 99 || atof(args[0]) < -99)
    {
        sprintf(buff, "Error - operand must be from -99 to 99.");
    }
    else
    {
        sprintf (buff,"%f", exp(atof(args[0])));
    }
}

static void fun_pow(char *buff, char *args[10])
{
    if (atof(args[0]) > 999 || atof(args[1]) < -999)
    {
        sprintf(buff, "Error - first operand must be from -999 to 999.");
    }
    else if (atof(args[1]) > 99 || atof(args[1]) < -99)
    {
        sprintf(buff, "Error - second operand must be from -99 to 99.");
    }
    else
    {
        sprintf(buff, "%f", pow(atof(args[0]), atof(args[1])));
    }
}

static void fun_log(char *buff, char *args[10])
{
    sprintf(buff, "%f", log10(atof(args[0])));
}

static void fun_ln(char *buff, char *args[10])
{
    sprintf(buff, "%f", log(atof(args[0])));
}

static void fun_arctan (char *buff, char *args[10])
{
    if (atof(args[0]) > 1 || atof(args[0]) < -1)
    {
        sprintf(buff, "Error - operand must be from -1 to 1.");
    }
    else
    {
        sprintf(buff, "%f", atan(atof(args[0])));
    }
}

static void fun_arccos(char *buff, char *args[10])
{
    if (atof(args[0]) > 1 || atof(args[0]) < -1)
    {
        sprintf(buff, "Error - operand must be from -1 to 1.");
    }
    else
    {
        sprintf(buff, "%f",acos(atof(args[0])));
    }
}

static void fun_arcsin(char *buff, char *args[10])
{
    if (atof(args[0]) > 1 || atof(args[0]) < -1)
    {
        sprintf(buff, "Error - operand must be from -1 to 1.");
    }
    else
    {
        sprintf(buff, "%f", asin(atof(args[0])));
    }
}

static void fun_tan(char *buff, char *args[10])
{
    if (atof(args[0]) > 1 || atof(args[0]) < -1)
    {
        sprintf(buff, "Error - operand must be from -1 to 1.");
    }
    else
    {
        sprintf(buff, "%f", tan(atof(args[0])));
    }
}

static void fun_cos(char *buff, char *args[10])
{
    if (atof(args[0]) > 1 || atof(args[0]) < -1)
    {
        sprintf(buff, "Error - operand must be from -1 to 1.");
    }
    else
    {
        sprintf(buff, "%f", cos(atof(args[0])));
    }
}

static void fun_sin(char *buff, char *args[10])
{
    if (atof(args[0]) > 1 || atof(args[0]) < -1)
    {
        sprintf(buff, "Error - operand must be from -1 to 1.");
    }
    else
    {
        sprintf(buff, "%f", sin(atof(args[0])));
    }
}

static void fun_gt(char *buff, char *args[10])
{
    sprintf(buff, "%d", (atof(args[0]) > atof(args[1])));
}

static void fun_lt(char *buff, char *args[10])
{
    sprintf(buff, "%d", (atof(args[0]) < atof(args[1])));
}

static void fun_gteq(char *buff, char *args[10])
{
    sprintf(buff, "%d", (atof(args[0]) >= atof(args[1])));
}

static void fun_lteq(char *buff, char *args[10])
{
    sprintf(buff, "%d", (atof(args[0]) <= atof(args[1])));
}

static void fun_if(char *buff, char *args[10])
{
    if (istrue(args[0]))
    {
        sprintf(buff, "%s", args[1]);
    }
    else
    {
        *buff = '\0';
    }

    return;
}

static void fun_ifelse(char *buff, char *args[10])
{
    if (istrue(args[0]))
    {
        sprintf(buff, "%s", args[1]);
    }
    else
    {
        sprintf(buff, "%s", args[2]);
    }

    return;
}

static void fun_rmatch(char *buff, char *args[10], dbref privs, dbref doer)
{
    dbref who;

    who = match_thing (privs, args[0]);

    if (!controls (privs, who, POW_EXAMINE) && who != doer)
    {
        strcpy (buff,"#-1");
        send_message(privs, "Permission denied.");

        return;
    }

    init_match (who, args[1], NOTYPE);
    match_everything();
    sprintf (buff, "#%d", match_result());
}

static void fun_wcount(char *buff, char *args[10])
{
    char *p = args[0];
    int num = 0;

    while (*p)
    {
        while (*p && isspace(*p))
            p++;

        if (*p)
            num++;

        while (*p && !isspace(*p))
            p++;
    }

    sprintf(buff,"%d",num);

    return;
}

// MERLMOD 
static void fun_lwho (char *buff, dbref privs, dbref owner)
{
    char buf[BUF_SIZE];
    char buf2[BUF_SIZE];
    struct descriptor_data *d;

    if ( Typeof(owner) != TYPE_PLAYER && ! payfor(owner, 50) )
    {
        send_message(owner, "You don't have enough pennies.");
        return;
    }

    *buf = '\0';

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == CONNECTED && d->player > 0)
        {
            if ((controls(owner, d->player, POW_WHO)) || could_doit(owner, d->player, A_LHIDE))
            {
                if (*buf)
                {
                    sprintf(buf2, "%s #%d",buf, d->player);
                    strcpy(buf, buf2);
                }
                else
                {
                    sprintf(buf, "#%d", d->player);
                }
            }
        }
    }

    strcpy(buff, buf);
}

static void fun_spc(char *buff, char *args[10], dbref dumm1, dbref dumm2)
{
    char tbuf1[BUF_SIZE];
    int i;
    int s = atoi(args[0]);

    if (s <= 0 )
    {
        strcpy(buff, "");
        return;
    }

    if (s > 950)
    {
        s = 950;
    }

    for (i = 0; i < s; i++)
    {
        tbuf1[i] = ' ';
    }

    tbuf1[i] = '\0';
    strcpy(buff, tbuf1);
}


///////////////////////////////////////////////////////////////////////////////
// do_flip()
///////////////////////////////////////////////////////////////////////////////
//     A utility function to reverse a string.
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
static void do_flip(char *s, char *r)
{
    char *p;

    p = strlen(s) + r;

    for (*p-- = '\0'; *s; p--, s++)
    {
        *p = *s;
    }
}

static void fun_flip(char *buff, char *args[10], dbref dumm1, dbref dumm2)
{
    do_flip(args[0], buff);
}

static void fun_lnum(char *buff, char *args[10], dbref dumm1, dbref dumm2)
{
    int x, i;

    x = atoi(args[0]);

    if ((x > 250) || (x < 0))
    {
        strcpy(buff, "#-1 Number Out Of Range");
        return;
    }
    else
    {
        strcpy(buff,"0");

        for (i = 1; i < x; i++)
        {
            sprintf(buff, "%s %d", buff, i);
        }
    }
}


static void fun_string(char *buf, char *args[10], dbref privs)
{
    char *letter;
    int num;
    int i;

    *buf   = '\0';
    num    = atoi(args[1]);
    letter = args[0];

    if (((num*strlen(letter)) <= 0) || ((num*strlen(letter)) > 950))
    {
        strcpy(buf, "#-1 Out Of Range");
        return;
    }

    *buf = '\0';

    for (i = 0; i < num; i++)
        strcat(buf,letter);
}

static void fun_ljust(char *buff, char *args[10], dbref privs)
{
    char tbuf1[BUF_SIZE];
    char buf[BUF_SIZE];
    char *text;
    int leader;
    int i;

    int number = atoi(args[1]);

    if (number <= 0 || number > 950)
    {
        sprintf(buff, "#-1 Number out of range.");
        return;
    }

    text   = args[0];
    leader = number-strlen(text);

    if (leader <= 0)
    {
        strcpy(buff, text);
        buff[number]=0;
        return;
    }

    if (leader > 950)
    {
        leader = 950;
    }

    for (i = 0; i < leader; i++)
    {
        tbuf1[i] = ' ';
    }

    tbuf1[i] = '\0';
    sprintf(buf, "%s%s", text, tbuf1);
    strcpy(buff, buf);
}

static void fun_rjust(char *buff, char *args[10], dbref privs)
{
    char tbuf1[BUF_SIZE];
    char buf[BUF_SIZE];
    int number = atoi(args[1]);
    char *text;
    int leader;
    int i;

    if (number <= 0 || number > 950)
    {
        sprintf(buff, "#-1 Number out of range.");

        return;
    }

    text   = args[0];
    leader = number-strlen(text);

    if (leader <= 0)
    {
        strcpy(buff, text);
        buff[number] = 0;

        return;
    }

    if (leader > 950)
    {
        leader = 950 - 1;
    }

    for (i = 0; i < leader; i++)
    {
        tbuf1[i] = ' ';
    }

    tbuf1[i] = '\0';
    sprintf(buf, "%s%s", tbuf1, text);
    strcpy(buff, buf);
}

static void fun_lattrdef(char *buff, char *args[10], dbref privs)
{
    static char smbuf[100];
    ATRDEF *k;
    int len = 0;

    dbref it = match_thing(privs, args[0]);

    *buff = '\0';

    if ((db[it].atrdefs) && (controls(privs,it,POW_EXAMINE) || (db[it].flags & SEE_OK)))
    {
        for (k = db[it].atrdefs; k; k = k->next)
        {
            if (len)
            {
                sprintf(smbuf, " %s", k->a.name);

                if ((strlen(smbuf)+len) > 990)
                {
                    strcat(buff, " #-1");
                    return;
                }

                strcat(buff, smbuf);
                len += strlen(smbuf);
            }
            else
            {
                sprintf(buff, "%s", k->a.name);
                len = strlen(buff);
            }
        }
    }
}

static void fun_lattr(char *buff, char *args[10], dbref privs)
{
    char temp[BUF_SIZE];
    ALIST *a;

    dbref it = match_thing(privs, args[0]);

    int len = 0;

    *buff = '\0';

    if (db[it].list)
    {
        for (a = db[it].list; a; a = AL_NEXT(a))
        {
            if (AL_TYPE(a) && can_see_atr(privs, it, AL_TYPE(a)))
            {
                sprintf(temp, (*buff)?" %s":"%s", unparse_attr(AL_TYPE(a),0));

                if ((len + strlen(temp)) > 960)
                {
                    strcat(buff, " #-1");
                    return;
                }

                strcpy(buff+len,temp);

                len += strlen(temp);
            }
        }
    }
}

static void fun_type(char *buff, char *args[10], dbref privs, dbref dumm1)
{

    dbref it = match_thing(privs, args[0]);

    if (it == NOTHING)
    {
        strcpy(buff, "#-1");
        return;
    }

    strcpy(buff, type_to_name(Typeof(it)));
}

static void fun_idle (char *buff, char *args[10], dbref owner)
{
    struct descriptor_data *d;
    char buf[BUF_SIZE];

    dbref who = 0;

    if (!string_compare(args[0], "me"))
    {
        who = owner;
    }
    else
    {
        who = lookup_player(args[0]);
    }
    if ( !who )
    {
        sprintf(buff, "#-1");
        return;
    }

    sprintf(buf, "#-1");

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == CONNECTED && d->player > 0)
        {
            if ((controls(owner, d->player, POW_WHO)) || could_doit(owner, d->player, A_LHIDE))
            {
                if ( d->player == who )
                {
                    sprintf(buf,"%ld",(now - d->last_time));
                    break;
                }
            }
        }
    }

    if ( buf == NULL)
    {
        sprintf(buf, "#-1");
    }

    strcpy(buff, buf);
}

static void fun_onfor(char *buff, char *args[10], dbref owner)
{
    struct descriptor_data *d;
    char buf[BUF_SIZE];
    dbref who = 0;

    if ( !(string_compare(args[0], "me")))
    {
        who = owner;
    }
    else
    {
        who = lookup_player(args[0]);
    }

    if (!who)
    {
        sprintf(buff, "#-1");
        return;
    }

    sprintf(buf, "#-1");

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == CONNECTED && d->player > 0)
        {
            if ((controls(owner, d->player, POW_WHO)) || could_doit(owner, d->player, A_LHIDE))
            {
                if ( d->player == who )
                {
                    sprintf(buf, "%ld", (now - d->connected_at));
                    break;
                }
            }
        }
    }

    if ( buf == NULL)
    {
        sprintf(buf, "#-1");
    }

    strcpy(buff, buf);
}


static void fun_port(char *buff, char *args[10], dbref owner)
{
    struct descriptor_data *d;
    char buf[BUF_SIZE];

    dbref who = 0;

    if ( !(string_compare(args[0], "me")))
    {
        who = owner;
    }
    else
    {
        who=lookup_player(args[0]);
    }

    sprintf(buf,"#-1");

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == CONNECTED && d->player>0)
        {
            if ((controls(owner, d->player, POW_WHO)) || could_doit(owner, d->player, A_LHIDE))
            {
                if ( d->player == who )
                {
                    sprintf(buf, "%d", ntohs(d->address.sin_port));
                    break;
                }
            }
        }
    }

    strcpy(buff, buf);
}

static void fun_host(char *buff, char *args[10], dbref owner)
{
    struct descriptor_data *d;
    char buf[BUF_SIZE];

    dbref who = 0;

    if ( !(string_compare(args[0], "me")))
    {
        who = owner;
    }
    else
    {
        who = lookup_player(args[0]);
    }

    sprintf(buf,"#-1");

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == CONNECTED && d->player > 0)
        {
            if ((controls(owner, d->player, POW_WHO)) || could_doit(owner, d->player, A_LHIDE))
            {
                if ( d->player == who && controls(owner, who, POW_HOST))
                {
                    sprintf(buf, "%s", d->addr);
                    break;
                }
            }
        }
    }

    strcpy(buff, buf);
}


static void fun_tms(char *buff, char *args[10])
{
    int num = atoi(args[0]);

    if (num < 0 )
    {
        sprintf(buff, "#-1");
    }
    else
    {
        sprintf(buff, "%s", time_format_2(num));
    }
}

static void fun_tml(char *buff, char *args[10])
{
    int num = atoi(args[0]);

    if (num <0 )
    {
        sprintf(buff,"#-1");
    }
    else
    {
        sprintf(buff,"%s",time_format_1(num));
    }
}

static int udef_fun(char **str, char *buff, dbref privs, dbref doer)
{
    char obuff[BUF_SIZE], *args[10], *s;
    ATTR *attr = NULL;
    dbref tmp, defed_on = NOTHING;
    int a;

    // check for explicit redirection
    if ((buff[0] == '#') && ((s = strchr(buff,':')) != NULL))
    {
        *s = '\0';
        tmp = (dbref) atoi(buff + 1);
        *s++ = ':';

        if (tmp >= 0 && tmp < db_top && ((attr = atr_str(privs, tmp, s)) != NULL) && (attr->flags & AF_FUNC) && can_see_atr(privs, tmp, attr) && !(attr->flags & AF_HAVEN))
        {
            defed_on = tmp;
        }
    }
    else if (((attr = atr_str(privs, tmp = privs, buff)) != NULL) && (attr->flags & AF_FUNC) && !(attr->flags & AF_HAVEN))
    {
        // checked the object doing it
        defed_on = tmp;
    }
    else
    {
        // check that object's zone 
        DOZONE(tmp, privs)
        {
            if (((attr = atr_str(privs, tmp, buff)) != NULL) && (attr->flags & AF_FUNC) && !(attr->flags & AF_HAVEN))
            {
                defed_on = tmp;
                break;
            }
        }
    }

    if (defed_on != NOTHING)
    {
        char result[BUF_SIZE], ftext[BUF_SIZE], *saveptr[10];

        for (a = 0; a < 10; a++)
        {
            args[a] = "";
        }

        for (a = 0; (a < 10) && **str &&  (**str != ')'); a++)
        {
            if (**str == ',')
            {
                (*str)++;
            }

            exec(str, obuff, privs, doer, 1);
            strcpy(args[a] = newglurp(strlen(obuff)+1), obuff);
        }

        for (a = 0; a < 10; a++)
        {
            saveptr[a] = wptr[a];
            wptr[a]    = args[a];
        }

        if (**str) (*str)++;

        strcpy(ftext, atr_get(defed_on, attr));
        pronoun_substitute(result, doer, ftext, privs);

        for(--a; a >= 0; a--)
        {
            // na_unalloc(glurp, wptr[a]);       // Never unalloc thing from glurp, esp static strings
                                                 // glurp is freed at the beginning of each command anyways.
                                                 // at the for(a=0;a<10;a++) args[a]="";, it was assigning them all
                                                 // to "", then it was freeing them here. not good. -shkoo 
            wptr[a] = saveptr[a];
        }

        strcpy(buff, result + strlen(db[doer].name) + 1);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// function to split up a list given a seperator 
// note str will get hacked up 
char *parse_up(char **args, int delimit)
{
    char **str = args;
    int deep = 0;
    char *s= *str, *os= *str;

    if (!*s)
    {
        return(NULL);
    }

    while(*s && (*s!=delimit))
    {
        if (*s++ == '{')
        {
            deep = 1;

            while(deep && *s)
            {
                switch(*s++)
                {
                    case '{':
                        deep++;
                        break;
                    case '}':
                        deep--;
                        break;
                }
            }
        }
    }

    if (*s)
    {
        *s++ = 0;
    }

    *str = s;

    return(os);
}

///////////////////////////////////////////////////////////////////////////////
// C code produced by gperf version 2.1 (K&R C version)                      //
// # gperf -t -G -p -r -j 0 -k * -N lookup_funcs_gperf -H hash_funcs_gperf   //
///////////////////////////////////////////////////////////////////////////////

//
// 116 keywords
// 639 is the maximum key range
//

//     register char *str;
//     register unsigned int  len;
static int hash_funcs_gperf (register char *str, register unsigned int len)
{
    static unsigned short hash_table[] =
    {
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 676, 676, 676, 676, 676,
     676, 676, 676, 676, 676, 119, 676,  85,  30,  84,
     109,  75, 116,  36,  19, 126,  43,  40,  18, 125,
      95, 108,  43, 100,  56,  45,  18,   9, 103,  53,
      26,  14,  73, 676, 676, 676, 676, 676,
    };
    register int hval = len;

    switch (hval)
    {
        default:
        case 10:
            hval += hash_table[(int)(str[9])];
        case 9:
            hval += hash_table[(int)(str[8])];
        case 8:
            hval += hash_table[(int)(str[7])];
        case 7:
            hval += hash_table[(int)(str[6])];
        case 6:
            hval += hash_table[(int)(str[5])];
        case 5:
            hval += hash_table[(int)(str[4])];
        case 4:
            hval += hash_table[(int)(str[3])];
        case 3:
            hval += hash_table[(int)(str[2])];
        case 2:
            hval += hash_table[(int)(str[1])];
        case 1:
            hval += hash_table[(int)(str[0])];
    }

    return hval;
}

static struct fun wordlist[] =
{
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",},
      {"lt",     fun_lt,     2},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"s",       fun_s,      1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"gt",     fun_gt,     2},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",},
      {"sub",     fun_sub,    2},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"v",       fun_v,      1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",},
      {"ln",      fun_ln,     1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"truth",   fun_truth,  1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"get",     fun_get,   -1},
      {"",}, {"",}, {"",}, {"",}, {"",},
      {"ljust",   fun_ljust,  2},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"exp",     fun_exp,    1},
      {"",}, {"",}, {"",}, {"",},
      {"has",     fun_has,    2},
      {"",},
      {"type",    fun_type,   1},
      {"mul",     fun_mul,    2},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"abs",     fun_abs,    1},
      {"tml",     fun_tml,    1},
      {"log",     fun_log,    1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"spc",     fun_spc,    1},
      {"rjust",   fun_rjust,  2},
      {"",}, {"",},
      {"sgn",     fun_sgn,    1},
      {"",}, {"",}, {"",}, {"",}, {"",},
      {"lor",     fun_lor,    2},
      {"",}, {"",}, {"",}, {"",}, {"",},
      {"tms",     fun_tms,    1},
      {"",}, {"",},
      {"host",    fun_host,   1},
      {"",}, {"",},
      {"bor",     fun_bor,    2},
      {"rest",    fun_rest,   1},
      {"pos",     fun_pos,    2},
      {"lattr",   fun_lattr,  1},
      {"tan",     fun_tan,    1},
      {"lwho",    fun_lwho,   0},
      {"",},
      {"fsub",    fun_fsub,   2},
      {"",}, {"",},
      {"pow",     fun_pow,    2},
      {"",}, {"",}, {"",}, {"",},
      {"lxor",    fun_lxor,   2},
      {"loc",     fun_loc,    1},
      {"",},
      {"lteq",     fun_lteq,   2},
      {"",}, {"",},
      {"next",    fun_next,   1},
      {"",}, {"",}, {"",}, {"",},
      {"sqrt",    fun_sqrt,   1},
      {"bxor",    fun_bxor,   2},
      {"",}, {"",}, {"",}, {"",},
      {"port",    fun_port,   1},
      {"",}, {"",},
      {"num",     fun_num,    1},
      {"gteq",     fun_gteq,   2},
      {"",}, {"",}, {"",}, {"",}, {"",},
      {"base",    fun_base,   3},
      {"cos",     fun_cos,    1},
      {"",}, {"",},
      {"lnot",    fun_lnot,   1},
      {"if",      fun_if,     2},
      {"",}, {"",}, {"",}, {"",},
      {"exit",    fun_exit,   1},
      {"",},
      {"lnum",    fun_lnum,   1},
      {"",}, {"",}, {"",},
      {"bnot",    fun_bnot,   1},
      {"",},
      {"zwho",    fun_zwho,   1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",},
      {"sin",     fun_sin,    1},
      {"",}, {"",},
      {"fmul",    fun_fmul,   2},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"fabs",    fun_fabs,    1},
      {"",},
      {"class",   fun_class,  1},
      {"link",    fun_link,   1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"con",     fun_con,    1},
      {"",}, {"",}, {"",}, {"",}, {"",},
      {"fsgn",    fun_fsgn,    1},
      {"",},
      {"s_as",    fun_s_as,   3},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"flags",   fun_flags,  1},
      {"add",     fun_add,    2},
      {"flip",    fun_flip,   1},
      {"",}, {"",}, {"",},
      {"land",    fun_land,   2},
      {"strcat",  fun_strcat, 2},
      {"strlen",  fun_strlen, 1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"band",    fun_band,   2},
      {"",},
      {"quota",   fun_quota  ,1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"idle",    fun_idle,   1},
      {"",}, {"",}, {"",},
      {"match",   fun_match,  2},
      {"linkup",  fun_linkup, 1},
      {"",}, {"",},
      {"fsqrt",   fun_fsqrt,   1},
      {"div",     fun_div,    2},
      {"",}, {"",}, {"",},
      {"mod",     fun_mod,    2},
      {"",}, {"",},
      {"time",    fun_time,  -1},
      {"rand",    fun_rand,   1},
      {"",},
      {"switch",  fun_switch,-1},
      {"",}, {"",}, {"",},
      {"zone",    fun_zone,   1},
      {"",}, {"",},
      {"has_a",   fun_has_a,  2},
      {"",}, {"",}, {"",}, {"",},
      {"mid",     fun_mid,    3},
      {"comp",    fun_comp,   2},
      {"",},
      {"first",   fun_first,  1},
      {"",}, {"",},
      {"extract", fun_extract,3},
      {"",}, {"",}, {"",},
      {"wcount",  fun_wcount, 1},
      {"lzone",   fun_lzone,  1},
      {"xtime",   fun_xtime, -1},
      {"delete",  fun_delete, 3},
      {"",}, {"",},
      {"is_a",    fun_is_a,   2},
      {"",}, {"",},
      {"string",  fun_string, 2},
      {"",},
      {"name",    fun_name,   1},
      {"",},
      {"s_with",  fun_s_with, -1},
      {"",}, {"",}, {"",},
      {"wmatch",  fun_wmatch, 2},
      {"",},
      {"owner",   fun_owner,  1},
      {"rmatch",  fun_rmatch, 2},
      {"",},
      {"objlist",  fun_objlist, 1},
      {"",}, {"",}, {"",},
      {"attropts", fun_attropts, -1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",},
      {"scomp",   fun_scomp,  2},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",},
      {"fadd",    fun_fadd,   2},
      {"parents", fun_parents,1},
      {"",}, {"",}, {"",}, {"",},
      {"arctan",  fun_arctan, 1},
      {"",}, {"",}, {"",},
      {"ctime",   fun_ctime,  1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"fdiv",    fun_fdiv,   2},
      {"",}, {"",},
      {"ifelse",  fun_ifelse, 3},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"arccos",  fun_arccos, 1},
      {"",}, {"",}, {"",}, {"",}, {"",},
      {"mtime",   fun_mtime,  1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"fcomp",   fun_fcomp,  2},
      {"",}, {"",}, {"",}, {"",}, {"",},
      {"getzone", fun_getzone,1},
      {"onfor",   fun_onfor,  1},
      {"",}, {"",}, {"",},
      {"playmem", fun_playmem,1},
      {"",}, {"",}, {"",}, {"",},
      {"arcsin",  fun_arcsin, 1},
      {"",}, {"",}, {"",}, {"",}, {"",},
      {"lattrdef", fun_lattrdef,1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"objmem",  fun_objmem, 1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"credits", fun_credits,1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"controls",  fun_controls, 3},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"remove",  fun_remove, 3},
      {"",},
      {"foreach", fun_foreach,2},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},

      {"inzone",  fun_inzone, 1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",},
      {"children", fun_children,1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",},
      {"entrances",  fun_entrances, 1},
      {"s_as_with", fun_s_as_with,-1},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",}, {"",},
      {"",},
      {"quota_left", fun_quota_left,1},
};

void info_funcs(dbref player)
{
    int i;

    send_message(player, "builtin functions:");
    send_message(player, "%10s %s", "function", "nargs");

    for (i = 0; i < (sizeof(wordlist) / sizeof(*wordlist)); i++)
    {
        if (wordlist[i].name && wordlist[i].name[0])
        {
            if (wordlist[i].nargs == -1)
            {
                send_message(player, "%10s any", wordlist[i].name);
            }
            else
            {
                send_message(player, "%10s %d", wordlist[i].name, wordlist[i].nargs);
            }
        }
    }
}

struct fun *lookup_funcs_gperf (register char *str, register unsigned int len)
{
    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
        register int key = hash_funcs_gperf (str, len);

        if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
        {
            register char *s = wordlist[key].name;

            if (*s == *str && !strcmp (str + 1, s + 1))
            {
                return &wordlist[key];
            }
        }
    }

    return 0;
}

static void do_fun(char **str, char *buff, dbref privs, dbref doer)
{
    char obuff[BUF_SIZE];
    char buf[BUF_SIZE];
    char *args[10];
    FUN *fp;
    int a;

    // look for buff in flist 
    strcpy(obuff, buff);

    for (a = 0; obuff[a]; a++)
    {
        obuff[a] = to_lower(obuff[a]);
    }

    fp = lookup_funcs_gperf (obuff, strlen(obuff));

    strcpy(obuff, buff);

    if (!fp)
    {

        if (udef_fun(str, buff, privs, doer))
        {
            return;
        }
        // ambiguous if/else here. So else {} might belong
        // one level higher to (!fp) instead -- Belisarius
        else
        {
            int deep = 2;
            char *s = buff + strlen(obuff);

            strcpy(buff, obuff);
            *s++ = '(';

            while(**str && deep)
            {

                switch(*s++ = *(*str)++)
                {
                    case '(':
                        deep++;
                        break;
                    case ')':
                        deep--;
                        break;
                }
            }

            if (**str)
            {
                (*str)--;
                s--;
            }

            *s = 0;

            return;
        }
    }

    // now get the arguments to the function 
    for (a = 0;(a < 10) && **str &&  (**str != ')'); a++)
    {
        if (**str == ',')
        {

            (*str)++;
        }

        exec(str, obuff, privs, doer, 1);
        strcpy(args[a] = (char *)newglurp(strlen(obuff)+1), obuff);
    }

    if (**str)
    {
        (*str)++;
    }

    if ((fp->nargs!=-1) && (fp->nargs!=a))
    {
        sprintf(buf, "Function (%s) only expects %d arguments",  fp->name, fp->nargs);
        strcpy(buff, buf);
    }
    else
    {
        fp->func(buff, args, privs, doer, a);
    }
}

///////////////////////////////////////////////////////////////////////////////
// func_zerolev()
///////////////////////////////////////////////////////////////////////////////
//    A semi-do-nothing function called from process_command just in case this
// [ exec() ] goes bezerko.
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void func_zerolev()
{
    lev = 0;
}

///////////////////////////////////////////////////////////////////////////////
// exec()
///////////////////////////////////////////////////////////////////////////////
//    Execute a string expression, return result in buff 
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void exec(char **str, char *buff, dbref privs, dbref doer, int coma)
{
    char *s,*e=buff;

    lev += 2; // enter the func. 

    if (lev > 1000)
    {
        strcpy(buff, "Too many levels of recursion.");
        return;
    }

    *buff=0;

    // eat preceding space 
    for (s= *str;*s && isspace(*s);s++)
    ;

    // parse until (,],) or , 
    for (;*s;s++)
    {
        switch(*s)
        {
            case ',':            // comma in list of function arguments 
            case ')':            // end of arguments 
                if (!coma)
                {
                    goto cont;
                }
            case ']':            // end of expression 
                                 // eat trailing space 
                while((--e >= buff) && isspace(*e))
                ;

                e[1] = 0;
                *str = s;
                lev--;

                return;
            case '(':            // this is a function 
                while ((--e >= buff) && isspace(*e))
                ;

                e[1] = 0;
                *str = s+1;

                // if just ()'s by them self then it is quoted 
                if (*buff)
                    do_fun(str, buff, privs, doer);

                lev--;

                return;
            case '{':
                if (e == buff)
                {
                    int deep = 1;
                    e = buff;
                    s++;

                    while (deep && *s)
                    {
                        switch(*e++ = *s++)
                        {
                            case '{':
                                deep++;
                                break;
                            case '}':
                                deep--;
                                break;
                        }
                    }

                    if ((e > buff) && (e[-1] == '}'))
                    {
                        e--;
                    }

                    while ((--e >= buff) && isspace(*e))
                    ;

                    e[1] = 0;
                    *str = s;
                    lev--;

                    return;
                }
                else
                {
                    // otherwise is a quote in middle, search for other end 
                    int deep = 1;
                    *e++ = *s++;

                    while (deep && *s)
                    {
                        switch(*e++= *s++)
                        {
                            case '{':
                                deep++;
                                break;
                            case '}':
                                deep--;
                                break;
                        }
                    }

                    s--;
                }
                break;
            default:
                cont:
                *e++= *s;
                break;
        }
    }

    while ((--e >= buff) && isspace(*e))
    ;

    e[1] = 0;
    *str = s;
    lev--;

    return;
}
