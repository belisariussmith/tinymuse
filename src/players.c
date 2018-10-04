///////////////////////////////////////////////////////////////////////////////
// player_list.c                                           TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: players.c,v 1.8 1993/09/07 18:24:42 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include <ctype.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "externs.h"

#define PLAYER_LIST_SIZE (1 << 12) // must be a power of 2 

#define DOWNCASE(x) to_lower(x)

static dbref hash_function_table[256];
static int hft_initialized = 0;

static struct pl_elt *player_list[PLAYER_LIST_SIZE];
static int pl_used = 0;

struct pl_elt
{
    dbref player;                  // pointer to player 
    // key is db[player].name
    struct pl_elt *next;
};

static void init_hft()
{
    int i;

    for (i = 0; i < 256; i++)
    {
        hash_function_table[i] = random() & (PLAYER_LIST_SIZE - 1);
    }

    hft_initialized = 1;
}

static dbref hash_function(char *string)
{
    dbref hash;

    if (!hft_initialized)
    {
         init_hft();
    } 

    hash = 0;

    for (; *string; string++)
    {
        hash ^= ((hash >> 1) ^ hash_function_table[(int)DOWNCASE(*string)]);
    }

    return(hash);
}

void clear_players()
{
    int i;
    struct pl_elt *e;
    struct pl_elt *next;

    for (i = 0; i < PLAYER_LIST_SIZE; i++)
    {
        if (pl_used)
        {
            for(e = player_list[i]; e; e = next)
            {
          	next = e->next;
          	free(e);
            }
        }

        player_list[i] = 0;

    }

    pl_used = 1;
}

///////////////////////////////////////////////////////////////////////////////
// add_player()
///////////////////////////////////////////////////////////////////////////////
//    Registers a new player inside the MUD
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void add_player(dbref player)
{
    dbref hash;
    struct pl_elt *e;

    if (!strchr(db[player].name, ' '))
    {
        hash              = hash_function(db[player].name);
        e                 = malloc(sizeof(struct pl_elt));
        e->player         = player;
        e->next           = player_list[hash];
        player_list[hash] = e;
    }

    if (*atr_get(player, A_ALIAS))
    {
        hash              = hash_function(atr_get(player, A_ALIAS));
        e                 = malloc(sizeof(struct pl_elt));
        e->player         = player;
        e->next           = player_list[hash];
        player_list[hash] = e;
    }
}

dbref lookup_player(char *name)
{
    struct pl_elt *e;
    dbref a;

    while (name[0] == '*') name++;

    for (e = player_list[hash_function(name)]; e; e = e->next)
    {
        if (!string_compare(db[e->player].name, name) || !string_compare(atr_get(e->player,A_ALIAS),name))
        {
            return e->player;
        }
    }

    if (name[0] == '#' && name[1])
    {
        a = atoi(name+1);

        if (a >= 0 && a < db_top)
        {
            return a;
        }
    }

    return NOTHING;
}

void delete_player(dbref player)
{
    dbref hash;
    struct pl_elt *prev;
    struct pl_elt *e;

    if (!strchr(db[player].name,' '))
    {
        hash = hash_function(db[player].name);

        if ((e = player_list[hash]) == 0)
        {
            return;
        }
        else if(e->player == player)
        {
            // it's the first one 
            player_list[hash] = e->next;
            free(e);
        }
        else
        {
            for (prev = e, e = e->next; e; prev = e, e = e->next)
            {
          	if (e->player == player)
                {
                    // got it 
                    prev->next = e->next;
                    free(e);
                    break;
        	}
            }
        }
    }

    if (*atr_get(player, A_ALIAS))
    {
        hash = hash_function(atr_get(player,A_ALIAS));

        if ((e = player_list[hash]) == 0)
        {
            return;
        }
        else if (e->player == player)
        {
            // it's the first one 
            player_list[hash] = e->next;
            free(e);
        }
        else
        {
            for (prev = e, e = e->next; e; prev = e, e = e->next)
            {
                if (e->player == player)
                {
                    // got it 
                    prev->next = e->next;
                    free(e);
                    break;
                }
            }
        }
    }
}
