// boolexp.c 
// $Id: boolexp.c,v 1.6 1993/08/16 01:56:52 nils Exp $ 

#include <ctype.h>

#include "tinymuse.h"
#include "db.h"
#include "match.h"
#include "externs.h"
#include "config.h"
#include "interface.h"

#define RIGHT_DELIMITER(x) ((x == AND_TOKEN) || (x == OR_TOKEN) || \
			    (x == ':') || (x == '.') || (x == ')') || \
			    (x == '=') || (!x))
#define LEFT_DELIMITER(x) ((x == NOT_TOKEN) || (x == '(') || (x == AT_TOKEN) \
			   || (x == IS_TOKEN) || (x == CARRY_TOKEN))

#define IS_TYPE    0
#define CARRY_TYPE 1
#define _TYPE      2

static int eval_boolexp1 P((dbref, char *, dbref));
static int eval_boolexp_OR P((char **));
static int eval_boolexp_AND P((char **));
static int eval_boolexp_REF P((char **));

dbref parse_player;
dbref parse_object;
dbref parse_zone;

static int bool_depth; // recursion protection 

#define FUN_CALL 1

static int get_word(char *d, char **s)
{
    int type = 0;

    while (!RIGHT_DELIMITER(**s))
    {
        if (**s == '[')
        {
            type = FUN_CALL;

            while (**s && (**s != ']'))
            {
                *d++ = *(*s)++;
            }
        }
        else
            *d++ = *(*s)++;
    }
    *d = '\0';

    return type;
}

static dbref match_dbref(dbref player, char *name)
{
    init_match(player, name, NOTYPE);
    match_everything();

    return noisy_match_result();
}

char *process_lock(dbref player, char *arg)
{
    static char buffer[MAX_CMD_BUF];
    char *s = arg;
    char *t = buffer;
    int type;
    dbref thing;

    while (*s)
    {
        while (LEFT_DELIMITER(*s))
        {
            *t++ = *s++;
        }

        // time to get a literal 
        type = get_word(t, &s);

        switch(*s)
        {
            case ':': // a built in attribute 

                if ((!builtin_atr_str(t)) && (type != FUN_CALL))
                {
                    send_message(player, "Warning: no such built in attribute '%s'", t);
                }

                t += strlen(t);
               *t++ = *s++;
                type = get_word(t, &s);
                t += strlen(t);
                break;
            case '.': // a user defined attribute 

                if ((thing = match_dbref(player, t)) < (dbref) 0)
                {
                    return NULL;
                }

                sprintf(t, "#%d", thing);
                t += strlen(t);
                *t++ = *s++;
                type = get_word(t, &s);

                if ((!atr_str(player,thing,t)) && (type != FUN_CALL))
                {
                    send_message(player, "Warning: no such attribute '%s' on #%d", t, thing);
                }

                t += strlen(t);

                if (*s != ':')
                {
                    send_message(player, "I don't understand that key.");
                    return NULL;
                }

                *t++ = *s++;
                type = get_word(t, &s);
                t += strlen(t);
                break;
            default:
                if (type != FUN_CALL)
                {
                    if ((thing = match_dbref(player, t)) == NOTHING)
                    {
                        return NULL;
                    }

                    sprintf(t, "#%d", thing);
                }

                t += strlen(t);
         }

         while (*s && RIGHT_DELIMITER(*s))
         {
             *t++ = *s++;
         }
    }
    *t = '\0';

    return buffer;
}

#undef FUN_CALL

char *unprocess_lock(dbref player, char *arg)
{
    static char buffer[MAX_CMD_BUF];
    char *s = arg;
    char *t = buffer;
    char *p;
    char save;
    dbref thing;

    while (*s)
    {
        if (*s == '#')
        {
            s++;

            for (p = s; isdigit(*s); s++)
            ;

            save = *s;
            *s++ = '\0';
            thing = (dbref) atoi(p);

            if (thing >= db_top || thing < 0)
            {
                thing = NOTHING;
            }

            sprintf(t, "%s", unparse_object(player, thing));
            t += strlen(t);

            if (save != '\0')
            {
                *t++ = save;
            }
            else
            {
                s--;
            }
        }
        else if (*s == '[')
        {
            while (*s && (*s != ']'))
            {
                *t++ = *s++;
            }

            if (!*s)
            {
                *t++ = *s++;
            }
        }
        else
        {
            *t++ = *s++;
        }
    }
    *t = '\0';

    return buffer;
}

static void eval_fun(char *buffer, char *str, dbref doer, dbref privs)
{
    char *s;

    for (s = str; *s;)
    {
        if (*s == '[')
        {
            s++;
            exec(&s, buffer, privs, doer, 0);
            buffer += strlen(buffer);

            if (*s)
            {
                s++;
            }
        }
        else
        {
            *buffer++ = *s++;
        }
    }

    *buffer = '\0';
}

int eval_boolexp(dbref player, dbref object, char *key, dbref zone)
{
    static char keybuf[MAX_CMD_BUF];

    bool_depth = 0;

    parse_player = player;
    parse_object = object;
    parse_zone = zone;
    strcpy(keybuf, key);

    return eval_boolexp1(object, keybuf, zone);
}

static int eval_boolexp1(dbref object, char *key, dbref zone)
{
    char buffer[MAX_CMD_BUF]; 
    char *buf = buffer;

    if (!*key)
    {
       return TRUE;
    }

    if (++bool_depth > 10)
    {
        send_message(db[parse_object].owner, "Warning: recursion detected in %s lock.", unparse_object(db[parse_object].owner, object));
        return FALSE;
    }

    eval_fun(buffer, key, parse_player, object);
    return eval_boolexp_OR(&buf);
}

static int test_atr(char **buf, dbref player, int ind)
{
    ATTR *attr;
    int result;
    char *s, save;

    for (s = *buf;!RIGHT_DELIMITER(*s) || (*s == '.');s++)
    ;
  
    if (!*s || (*s != ':'))
    {
        return NOTHING;
    }

    *s++ = '\0';

    if (strchr(*buf,'.'))
    {
        attr = atr_str(player, NOTHING, *buf);
    }
    else
    {
        attr = builtin_atr_str(*buf);
    }

    if (  !(attr)
        || (attr->flags & AF_DARK)
        || ((ind) && !can_see_atr(parse_object, player, attr))
       )
    {
          return NOTHING;
    }

    for (*buf = s;*s && (*s != AND_TOKEN) && (*s != OR_TOKEN) && (*s != ')');s++)
    ;

    save = *s;
    *s = '\0';
    result = wild_match(*buf, atr_get(player, attr));
    *s = save;
    *buf = s;

    return result;
}

static int grab_num(char **buf)
{
    char *s, save;
    int num;

    for (s = *buf; *s && isdigit(*s); s++)
    ;

    save = *s;
    *s = '\0';
    num = atoi(*buf);
    *s = save;
    *buf = s;

    return num;
}

static dbref get_dbref(char **buf)
{
    char *s, save;
    dbref temp;

    for (s = *buf;!RIGHT_DELIMITER(*s) && (*s != '=');s++)
    ;

    save = *s;
    *s = '\0';
    init_match(parse_object, *buf, NOTYPE);
    match_everything();
    *s = save;
    *buf = s;
    temp = match_result();

    return temp;
}

static int eval_boolexp_OR(char **buf)
{
    int left;

    left = eval_boolexp_AND(buf);

    if (**buf == OR_TOKEN)
    {
        (*buf)++;
        return (eval_boolexp_OR(buf) || left);
    }
    else
    {
        return left;
    }
}

static int eval_boolexp_AND(char **buf)
{
    int left;

    left = eval_boolexp_REF(buf);

    if (**buf == AND_TOKEN)
    {
        (*buf)++;
        return (eval_boolexp_AND(buf) && left);
    }
    else
    {
        return left;
    }
}

static int eval_boolexp_REF(char **buf)
{
    dbref thing;
    int type, t;

    switch (**buf)
    {
        case '(':
            (*buf)++;
            t = eval_boolexp_OR(buf);

            if (**buf == ')')
            {
                (*buf)++;
            }

            return t;
        case NOT_TOKEN:
            (*buf)++;

            return !eval_boolexp_REF(buf);
        case AT_TOKEN:
            (*buf)++;

            if (**buf == '(')
            {
                // an indirect attribute 
                (*buf)++;
                thing = get_dbref(buf);

                if (**buf != '=')
                {
                    return eval_boolexp1(thing, atr_get(thing, A_LOCK), parse_zone);
                }

                (*buf)++;
                t = test_atr(buf,thing,1);

                if (**buf == ')')
                {
                    (*buf)++;
                }

                return (t == -1) ? FALSE : t;
            }
            thing = get_dbref(buf);

            if (can_see_atr(parse_object,thing,A_LOCK))
            {
                return eval_boolexp1(thing, atr_get(thing, A_LOCK), parse_zone);
            }
            else
            {
                return FALSE;
            }
        default:
            switch (**buf)
            {
                case IS_TOKEN:
                    type = IS_TYPE;
                    (*buf)++;
                    break;
                case CARRY_TOKEN:
                    type = CARRY_TYPE;
                    (*buf)++;
                    break;
                default:
                    type = _TYPE;
            }

            if (isdigit(**buf))
            {
                return grab_num(buf);
            }

            if ((t = test_atr(buf, parse_player, 0)) != -1)
            {
                return t;
            }

            if ((thing = get_dbref(buf)) < (dbref) 0)
            {
                return FALSE;
            }

            switch (type)
            {
                case IS_TYPE:
                    return (parse_player == thing) ? TRUE : FALSE;
                case CARRY_TYPE:
                    return member(thing, db[parse_player].contents);
                case _TYPE:
                    return ((parse_player == thing) || (member(thing, db[parse_player].contents)) || (parse_zone == thing));
                default:
                    // so gcc stops complaining 
                    return FALSE;
            }
    }
}
