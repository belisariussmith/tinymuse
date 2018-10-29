///////////////////////////////////////////////////////////////////////////////
// match.c                                                    TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: match.c,v 1.5 1993/05/12 17:17:03 nils Exp $
///////////////////////////////////////////////////////////////////////////////
#include <ctype.h>                         // Routines for parsing arguments 

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "match.h"
#include "externs.h"

#define DOWNCASE(x) to_lower(x)

static dbref exact_match = NOTHING;        // holds result of exact match 
static int check_keys = 0;                 // if non-zero, check for keys 
static dbref last_match = NOTHING;         // holds result of last match 
static int match_count;                    // holds total number of inexact matches 
static dbref match_who;                    // player who is being matched around 
static char *match_name;                   // name to match 
static int preferred_type = NOTYPE;        // preferred type 
static dbref it;                           // the *IT*! 
static void store_it P((dbref));

///////////////////////////////////////////////////////////////////////////////
// is_prefix()
///////////////////////////////////////////////////////////////////////////////
//    This will check case insensitively if p is prefix for s 
///////////////////////////////////////////////////////////////////////////////
// Returns: Boolean
///////////////////////////////////////////////////////////////////////////////
static int is_prefix(char *p, char *s)
{
    if (!*s)
    {
        return FALSE;
    }

    while(*p)
    {
        if (!*s || (to_upper(*p++) != to_upper(*s++)))
        {
            return FALSE;
        }
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// pref_match()
///////////////////////////////////////////////////////////////////////////////
//    Checks a list for objects that are prefix's for string, && are controlled
// || link_ok 
///////////////////////////////////////////////////////////////////////////////
// Returns: (int) database reference obj id
///////////////////////////////////////////////////////////////////////////////
dbref pref_match(dbref player, dbref list, char *string)
{
    dbref lmatch = NOTHING;
    int   mlen   = 0;

    while (list != NOTHING)
    {
        if (is_prefix(db[list].name, string) && (db[list].flags & PUPPET) && (controls(player, list, POW_MODIFY) || (db[list].flags & LINK_OK)))
        {
            if (strlen(db[list].name)>mlen)
            {
                lmatch = list;
                mlen = strlen(db[list].name);
            }
        }

        list = db[list].next;
    }

    return(lmatch);
}

void init_match(dbref player, char *name, int type)
{
    exact_match    = last_match = NOTHING;
    match_count    = 0;
    match_who      = player;
    match_name     = color(name, REMOVE);
    check_keys     = 0;
    preferred_type = type;

    if ((!string_compare(name,"it")) && *atr_get(player,A_IT))
    {
        it = atoi(1+atr_get(player, A_IT));
    }
    else
    {
        it = NOTHING;
    }
}

void init_match_check_keys(dbref player, char *name, int type)
{
    init_match(player, name, type);
    check_keys = 1;
}

static dbref choose_thing(dbref thing1, dbref thing2)
{
    int has1;
    int has2;
  
    if (thing1 == NOTHING)
    {
        return thing2;
    }
    else if (thing2 == NOTHING)
    {
        return thing1;
    }
  
    if (preferred_type != NOTYPE)
    {
        if (Typeof(thing1) == preferred_type)
        {
            if (Typeof(thing2) != preferred_type)
            {
        	return thing1;
            }
        }
        else if (Typeof(thing2) == preferred_type)
        {
            return thing2;
        }
    }
  
    if (check_keys)
    {
        has1 = could_doit(match_who, thing1, A_LOCK);
        has2 = could_doit(match_who, thing2, A_LOCK);
    
        if (has1 && !has2)
        {
            return thing1;
        }
        else if (has2 && !has1)
        {
            return thing2;
        }
        // else fall through 
    }
  
    return (random() % 2) ? thing1 : thing2;
}

void match_player()
{
    dbref match;
    char *p;

    if (it != NOTHING && Typeof(it) == TYPE_PLAYER)
    {
        exact_match = it;
        return;
    }

    if (*match_name == LOOKUP_TOKEN)
    {
        for(p = match_name + 1; isspace(*p); p++)
        ;

        if ((match = lookup_player(p)) != NOTHING)
        {
            exact_match = match;
        }
    }
}

// returns nnn if name = #nnn, else NOTHING 
static dbref absolute_name()
{
    dbref match;
  
    if (*match_name == NUMBER_TOKEN)
    {
        match = parse_dbref(match_name+1);

        if (match < 0 || match >= db_top)
        {
            return NOTHING;
        }
        else
        {
            return match;
        }
    }
    else
    {
        return NOTHING;
    }
}

void match_absolute()
{
    dbref match;

    if (it != NOTHING)
    {
        exact_match = it;
        return;
    }

    if ((match = absolute_name()) != NOTHING)
    {
        exact_match = match;
    }
}

void match_me()
{
    if ((it != NOTHING) && (it == match_who))
    {
        exact_match = it;
        return;
    }

    if (!string_compare(match_name, "me"))
    {
        exact_match = match_who;
    }
}

void match_here()
{
    if (it != NOTHING && it == db[match_who].location)
    {
        exact_match = it;
        return;
    }

    if (db[match_who].location != NOTHING)
    {
        if (!string_compare(match_name, "here"))
        {
            exact_match = db[match_who].location;
        }
    }
}

static void match_list(dbref first)
{
    dbref absolute;
  
    absolute = absolute_name();
  
    DOLIST(first, first)
    {
        if (first == absolute  ||  first == it)
        {
            exact_match = first;
            return;
        }
        else if (!string_compare(db[first].name, match_name) ||( *atr_get(first,A_ALIAS) && !string_compare(match_name, atr_get(first,A_ALIAS))) )
        {
            // if there are multiple exact matches, randomly choose one 
            exact_match = choose_thing(exact_match, first);
        }
        else if (string_match(db[first].name, match_name))
        {
            if (match_count > 0 && !string_compare(db[last_match].name,db[first].name))
            {
                match_count = NONE;
            }

            last_match = first;
            match_count++;
        }
    }
}

void match_possession()
{
    match_list(db[match_who].contents);
}

void match_neighbor()
{
    dbref loc;
  
    if ((loc = db[match_who].location) != NOTHING)
    {
        match_list(db[loc].contents);
    }
}

void match_perfect()
{
    dbref loc;
    dbref first;
  
    if ((loc = db[match_who].location) != NOTHING)
    {
        first = db[loc].contents;
    }
    else
    {
        return;
    }

    DOLIST(first, first)
    {
        if (!strcmp(db[first].name, match_name) && ((Typeof(first) == preferred_type) || (preferred_type == NOTYPE)))
        {
            exact_match = first;
        }
    }
}

void match_exit()
{
    dbref loc;
    dbref exit;
    dbref absolute;
    char *match;
    char *p;

    if (Typeof(db[match_who].location)!=TYPE_ROOM && Typeof(db[match_who].location) !=TYPE_THING)
    {
        return;
    }

    if ((loc = db[match_who].location) != NOTHING)
    {
        absolute = absolute_name();
    
        DOLIST(exit, Exits(loc))
        {
            if (exit == absolute || exit == it)
            {
                exact_match = exit;
            }
            else
            {
                match = db[exit].name;
                while(*match)
                {
                    // check out this one 
                    for (p = match_name; (*p && DOWNCASE(*p) == DOWNCASE(*match) && *match != EXIT_DELIMITER); p++, match++)
                    ;
                   
                    // did we get it? 
                    if (*p == '\0')
                    {
                        // make sure there's nothing afterwards 
                        while (isspace(*match))
                        {
                            match++;
                        }

                        if (*match == '\0' || *match == EXIT_DELIMITER)
                        {
                            // we got it 
                            exact_match = choose_thing(exact_match, exit);
                            goto next_exit; // got this match 
                        }
                    }
                    // we didn't get it, find next match 
                    while (*match && *match++ != EXIT_DELIMITER)
                    ;

                    while (isspace(*match))
                    {
                        match++;
                    }
                }
            }

            next_exit:
            ;
        }
    }
}

void match_everything()
{
    match_exit();
    match_neighbor();
    match_possession();
    match_me();
    match_here();
    //if(power(player, TYPE_HONWIZ)) {
    match_absolute();
    match_player();
    //}
}

dbref match_result()
{
    if (exact_match != NOTHING)
    {
        store_it(exact_match);
        return exact_match;
    }
    else
    {
        switch(match_count)
        {
            case 0:
                return NOTHING;
            case 1:
                store_it(last_match);
                return last_match;
            default:
                return AMBIGUOUS;
        }
    }
}

// use this if you don't care about ambiguity 
dbref last_match_result()
{
    if (exact_match != NOTHING)
    {
        store_it(exact_match);
        return exact_match;
    }
    else
    {
        store_it(last_match);
        return last_match;
    }
}

dbref noisy_match_result()
{
    dbref match;
  
    switch(match = match_result())
    {
        case NOTHING:
            send_message(match_who, NOMATCH_PATT, match_name);
            return NOTHING;
        case AMBIGUOUS:
            send_message(match_who, "do you mean %s, %s, or %s?", match_name, match_name, match_name);
            return NOTHING;
        default:
            return match;
    }
}


static void store_it(dbref what)
{
    char buf[50];

    if (what == NOTHING)
    {
        return;
    }

    sprintf(buf,"#%d",what);
    atr_add(match_who,A_IT,buf);
}
