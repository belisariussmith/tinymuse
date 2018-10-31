////////////////////////////////////////////////////////////////////////////////
// predicates.c                                                 TinyMUSE (@)
////////////////////////////////////////////////////////////////////////////////
// $Id: predicates.c,v 1.25 1994/02/18 22:40:54 nils Exp $ 
////////////////////////////////////////////////////////////////////////////////
// Predicates for testing various conditions
////////////////////////////////////////////////////////////////////////////////
#include <ctype.h>
#include <stdarg.h>

#include "tinymuse.h"
#include "db.h"
#include "interface.h"
#include "config.h"
#define NO_PROTO_VARARGS
#include "externs.h"

static int ok_name P((char *));
static int group_depth = 0;
static int group_controls_int P((dbref,dbref));

int Level(dbref thing)
{
    if(db[thing].owner != thing)
    {
        if(db[thing].flags&INHERIT_POWERS)
        {
            return Level(db[thing].owner);
        }
        else
        {
            return CLASS_VISITOR;    // no special powers.
        }
    }

    if (db[thing].flags & PLAYER_MORTAL)
    {
        return CLASS_VISITOR;
    }

    return *db[thing].pows;
}

int Levnm(dbref thing)
{
    if (db[thing].flags & INHERIT_POWERS)
    {
        thing = db[thing].owner;
    }
    if (Typeof(thing)==TYPE_PLAYER)
    {
        return *db[thing].pows;
    }

    return Level(thing);
}

int power(dbref thing, int level_check)
{
    // mortal flag on player makes him mortal - no arguments 
    if ( (IS(thing, TYPE_PLAYER, PLAYER_MORTAL))
      && (level_check != POW_MEMBER)
       )
    {
        return 0;
    }

    return has_pow(thing, NOTHING, level_check);
}

int inf_mon(dbref thing)
{
  if (has_pow(db[thing].owner, NOTHING, POW_MONEY))
  {
      return TRUE;
  }

  return FALSE;
}

int can_link_to(dbref who, dbref where, int cutoff_level)
{
    return ( ( where >= 0 && where < db_top )
          && ( controls(who, where, cutoff_level) || (db[where].flags & LINK_OK))
           );
}

///////////////////////////////////////////////////////////////////////////////
// could_doit()
///////////////////////////////////////////////////////////////////////////////
//     Returns whether or not a player can do
///////////////////////////////////////////////////////////////////////////////
// Returns: Boolean (int)
///////////////////////////////////////////////////////////////////////////////
int could_doit(dbref player, dbref thing, ATTR *attr)
{
    // no if puppet tries to get key 
    if ((Typeof(player) == TYPE_THING) && IS(thing, TYPE_THING,THING_KEY))
    {
        return FALSE;
    }

    if ((Typeof(thing) == TYPE_EXIT) && (db[thing].link == NOTHING))
    {
        return FALSE;
    }

    if ( (Typeof(thing) == TYPE_PLAYER || Typeof(thing) == TYPE_THING)
      && (db[thing].location == NOTHING)
       )
    {
        return FALSE;
    }

    return(eval_boolexp (player, thing, atr_get(thing, attr),
               get_zone_first(player)));
}

void did_it(dbref player, dbref thing, ATTR *what, char *def, ATTR *owhat, char *odef, ATTR *awhat)
{
    char *d;
    char buff[BUF_SIZE];
    char buff2[BUF_SIZE];
    dbref loc = db[player].location;

    if (loc == NOTHING)
    {
        // No location, so nothing to do
        return;
    }

    // message to player
    if (what)
    {
        if (*(d = atr_get(thing, what)))
        {
            strcpy(buff2, d);
            pronoun_substitute(buff, player, buff2, thing);
            send_message(player,  buff + strlen(db[player].name) + 1);
        }
        else
        {
            if (def)
            {
                send_message(player, def);
            }
        }
    }

    if (!IS(get_room(player), TYPE_ROOM, ROOM_AUDITORIUM))
    {
        // message to neighbors 
        if (owhat)
        {
            if (*(d = atr_get(thing, owhat)) && !(db[thing].flags & HAVEN))
            {
                strcpy(buff2,d);
                pronoun_substitute(buff,player,buff2,thing);
                notify_in(loc,player,buff);
            }
            else
            {
                if (odef)
                {
                    sprintf(buff, "%s %s", db[player].name, odef);
                    notify_in(loc, player, buff);
                }
            }

            // do the action attribute 
            if (awhat && *(d = atr_get(thing, awhat)))
            {
                char *b;
                char dbuff[BUF_SIZE];
                strcpy(dbuff,d);
                d = dbuff;

                // check if object has # of charges 
                if (*(b = atr_get(thing, A_CHARGES)))
                {
                    int num = atoi(b);
                    char ch[100];

                    if (num)
                    {
                        sprintf(ch, "%d", num - 1);
                        atr_add(thing, A_CHARGES, ch);
                    }
                    else
                    {
                        if (!*(d = atr_get(thing, A_RUNOUT)))
                        {
                            return;
                        }
                    }
                    strcpy(dbuff, d);
                    d = dbuff;
                }
                //    pronoun_substitute(buff,player,d,thing);
                // parse the buffer and do the sub commands
                parse_que(thing, d, player);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// check_zone()
///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
// Returns: Boolean
///////////////////////////////////////////////////////////////////////////////
dbref check_zone(dbref player, dbref who, dbref where, int move_type)
{
    dbref old_zone, new_zone;
    int zonefail = 0;

    old_zone = get_zone_first(who);
    new_zone = get_zone_first(where);
    //if ((old_zone = get_zone(who)) == NOTHING) old_zone = db[0].zone;
    //if ((new_zone = get_zone(where)) == NOTHING) new_zone = db[0].zone;

    // Check for illegal home zone crossing. Applies to objects only. 
    if (move_type == MOVE_HOME)
    {
        // Always allow return to home. 
        return 1;
    }

    // safety in case root hasn't set the universal zone yet 
    if ((old_zone == NOTHING) || (new_zone == NOTHING))
    {
        return (dbref) 1;
    }

    // Check to see if the zones don't match 
    if (old_zone != new_zone)
    {
        // Check to see if 'who' can pass the zone leave-lock 
        if (move_type == 1 && !could_doit(who, old_zone, A_LLOCK) && !controls(player, old_zone, POW_TELEPORT))
        {
            did_it(who, old_zone, A_LFAIL, "You can't leave.", A_OLFAIL, NULL, A_ALFAIL);
                
            return (dbref) 0;
        }
        else
        {
            // Do not allow KEY objects to leave the zone. 
            if ((Typeof(who) != TYPE_PLAYER) && (db[new_zone].flags & THING_KEY))
            {
                zonefail = TRUE;
            }

            // Check for @tel or walk and check appropriate lock. 
            if (!eval_boolexp(who, new_zone,atr_get(new_zone,(move_type) ?A_ELOCK:A_LOCK),old_zone))
            {
                zonefail = TRUE;
            }
                         
            if (move_type == MOVE_TELEPORT)
            {
                // Make sure player can properly @tel in or out of zone 
                if (!(db[new_zone].flags & ENTER_OK))
                {
                    zonefail = TRUE;
                }

                // Anyone with POW_TEL can @tel anywhere, must be the last condition */
                if (power(player,POW_TELEPORT))
                {
                    zonefail = FALSE;
                }
            }

            // Next we evaluate if successfull/failed and do the verbs 

            if (zonefail == TRUE && move_type == MOVE_WALK)
            {
                // Failed walking attempt 
                did_it(who, new_zone, A_FAIL, "You can't go that way.", A_OFAIL, NULL, A_AFAIL);

                return (dbref) 0;
            }

            if (zonefail == 1 && move_type == MOVE_TELEPORT)
            {
                // Failed @tel attempt 
                did_it(who, new_zone, A_EFAIL, perm_denied(), A_OEFAIL, NULL, A_AEFAIL);
                return (dbref) 0;
            }
 
            if (zonefail == 0 && move_type == MOVE_WALK)
            {
                // Successfull walking attempt 
                did_it(who, new_zone, A_SUCC, NULL, A_OSUCC, NULL, A_ASUCC);
                return old_zone;
            }
 
            if (zonefail == 0 && move_type == MOVE_TELEPORT)
            {
                // Successfull @tel attempt 
                return(dbref) 1;
            }
        }
    }
    else
    {
        return (dbref)1;
    }

    return (dbref)1;
}

///////////////////////////////////////////////////////////////////////////////
// can_see()
///////////////////////////////////////////////////////////////////////////////
//     Returns whether or not the player can see the thing they're trying to
// look at.
///////////////////////////////////////////////////////////////////////////////
// Returns: Boolean
///////////////////////////////////////////////////////////////////////////////
int can_see(dbref player, dbref thing, int can_see_loc)
{
    if (player == thing || Typeof(thing) == TYPE_EXIT || ((Typeof(thing)==TYPE_PLAYER) && !IS(thing,TYPE_PLAYER,CONNECT)))
    {
        // 1) your own body isn't listed in a 'look'
        // 2) exits aren't listed in a 'look'
        // 3) unconnected (sleeping) players aren't listed in a 'look' 
        return FALSE;
    }
    else if (can_see_loc)
    {
        // if the room is lit, you can see any non-dark objects 
        return( ! Dark(thing));
    }
    else
    {
        // otherwise room is dark and you can't see a thing 
        if (IS(thing,TYPE_THING,THING_LIGHT) && controls(thing,db[thing].location,POW_MODIFY))
        {
            // LIGHT flag. can see it in a dark room. */
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

int can_set_atr(dbref who, dbref what, ATTR *atr)
{
    if (!can_see_atr (who, what, atr))
        return FALSE;
    if (atr->flags&AF_BUILTIN)
        return FALSE;
    if ((atr == A_QUOTA || atr == A_RQUOTA) && ! power (who, POW_SECURITY))
        return FALSE;
    if ((atr == A_PENNIES) && !power (who, POW_MONEY))
        return FALSE;
    if (!controls (who, what, POW_MODIFY))
        return FALSE;
    if (atr->flags&AF_WIZARD && atr->obj == NOTHING && !power(who,POW_WATTR))
        return FALSE;
    if (atr->flags&AF_WIZARD && atr->obj != NOTHING && !controls(who, atr->obj, POW_WATTR))
        return FALSE;

    // Looks like they can!
    return TRUE;
}

int can_see_atr(dbref who, dbref what, ATTR *atr)
{
    if (atr == A_PASS && !is_root(who))
        return FALSE;
    if (!(atr->flags&AF_OSEE) && !controls(who, what, POW_SEEATR) && !(db[what].flags&SEE_OK))
        return FALSE;
    if (atr->flags&AF_DARK && atr->obj == NOTHING && !power(who, POW_SECURITY))
        return FALSE;
    if (atr->flags&AF_DARK && atr->obj != NOTHING && !controls(who, atr->obj, POW_SEEATR))
        return FALSE;

    // Looks like they can!
    return TRUE;
}

static int group_controls(dbref who, dbref what)
{
    group_depth = 0;
    return group_controls_int(who,what);
}

static int group_controls_int(dbref who, dbref what)
{
    char buf[BUF_SIZE];
    char *s = buf;
    char *z;

    group_depth++;

    if (group_depth > 20)
    {
        return FALSE;
    }

    if (who == what)
    {
        return TRUE;
    }
    else if (*atr_get(what, A_USERS))
    {
        strcpy(buf, atr_get(what, A_USERS));

        while ((z = parse_up(&s,' ')))
        {
            int i;

            if (*z != '#')
                continue;

            i = atoi(z + 1);

            if (i >= 0 && i < db_top && Typeof(i) == TYPE_PLAYER)
            {
                if (group_controls_int(who,i))
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

int controls_a_zone(dbref who, dbref what, int cutoff_level)
{
    dbref zon;

    DOZONE(zon, what)
    {
        if (controls (who, zon, cutoff_level))
        {
            return TRUE;
        }
    }

    return FALSE;
}

int controls(dbref who, dbref what, int cutoff_level)
{
    // valid thing to control?
    if (what == NOTHING)
    {
        return has_pow(who, what, cutoff_level);
    }
    if ( what < 0 || what >= db_top )
    {
        return FALSE;
    }

    if ((cutoff_level == POW_EXAMINE || cutoff_level == POW_SEEATR) && (db[what].flags&SEE_OK))
    {
        return TRUE;
    }

    // owners (and their stuff) control the owner's stuff 
    //if ( db[who].owner == db[what].owner )
    //{
    //    return TRUE;
    //}

    if (db[who].owner == db[what].owner || group_controls(db[who].owner,db[what].owner))
    {
        // the owners match, check ipow 
        if(db[who].owner == who)
        {
            return TRUE; // it's the player 
        }
        else if(db[who].flags&INHERIT_POWERS)
        {
            return TRUE;
        }
        else if((db[what].flags&INHERIT_POWERS || db[what].owner == what) && (!db[db[what].owner].pows || (*db[db[what].owner].pows)>CLASS_CITIZEN))
        {
            return FALSE; // non inherit powers things don't control inherit stuff 
        }
        else
        {
            return TRUE; // the target isn't inherit  
        }
    }

    //if (( db[who].owner == db[what].owner ) &&
    //((db[who].flags&INHERIT_POWERS) || !(db[what].flags&INHERIT_POWERS)
    //|| Typeof(who)==TYPE_PLAYER ||                                      // or it's a player -- dont need I.
    //!power(db[who].owner,TYPE_OFFICIAL)))                               // or it's no special powers
    //{
    //    return TRUE;
    //}

    if (db[what].flags & INHERIT_POWERS)
    {
        what = db[what].owner;
    }

    // root controls all 
    if ( who == root )
    {
        return TRUE;
    }

    // and root don't listen to anyone! 
    if ( (what == root) || (db[what].owner == root) )
    {
        return FALSE;
    }

    if (has_pow(who, what, cutoff_level))
    {
        return TRUE;
    }

    return FALSE;
}

dbref def_owner(dbref who)
{
    if (!*atr_get(who,A_DEFOWN))
    {
        return db[who].owner;
    }
    else
    {
        dbref i;
        i = match_thing(who,atr_get(who,A_DEFOWN));

        if (i == NOTHING || Typeof(i) != TYPE_PLAYER)
        {
            return db[who].owner;
        }

        if (!controls(who, i, POW_MODIFY))
        {
            send_message(who, "You don't control %s, so you can't make things owned by %s.", unparse_object(who, i), unparse_object(who, i));
            return db[who].owner;
        }

        return db[i].owner;
    }
}

int can_link(dbref who, dbref what, int cutoff_level)
{
    return ((Typeof(what) == TYPE_EXIT && db[what].location == NOTHING)
     || controls(who, what, cutoff_level));
}

int can_pay_fees(dbref who, int credits, int quota)
{
    // can't charge credits till we've verified building quota 
    if ( ! Guest(db[who].owner) && Pennies(db[who].owner) < credits && !has_pow(db[who].owner, NOTHING, POW_MONEY))
    {
        send_message(who, "With what credits?");
        return FALSE;
    }

    // check building quota 
    if ( ! pay_quota(who, quota) )
    {
        send_message(who, "With what quota?");
        return FALSE;
    }

    // charge credits 
    payfor(who, credits);

    return TRUE;
}

void giveto(dbref who, int pennies)
{
    int old_amount;

    // wizards don't need pennies 
    if (has_pow(db[who].owner, NOTHING, POW_MONEY))
    {
        return;
    }

    who        = db[who].owner;
    old_amount = Pennies(who);

    if (old_amount + pennies < NONE)
    {
        if ((old_amount > NONE) && (pennies > NONE))
        {
            s_Pennies(who,(((unsigned)-2)/2));
        }
        else
        {
            s_Pennies(who, NONE);
        }
    }
    else
    {
        s_Pennies(who, old_amount+pennies);
    }
}

int payfor(dbref who, int cost)
{
    dbref tmp;

    if (Guest(who) || has_pow(db[who].owner,NOTHING,POW_MONEY))
    {
        return TRUE;
    }
    else if ((tmp = Pennies(db[who].owner)) >= cost)
    {
        s_Pennies(db[who].owner, tmp-cost);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void add_bytesused(dbref who, int payment)
{
    char buf[20];
    int tot;

    if (!*atr_get(who,A_BYTESUSED))
    {
        recalc_bytes(who);
    }

    tot = atoi(atr_get(who, A_BYTESUSED)) + payment;
    sprintf(buf, "%d", tot);
    atr_add(who, A_BYTESUSED, buf);

    if (!*atr_get(who,A_BYTESMAX))
    {
        // no byte quota. 
        return;
    }
    if (tot>atoi(atr_get(who, A_BYTESMAX)))
    {
        db[who].i_flags |= I_QUOTAFULL;
    }
    else
    {
        db[who].i_flags &=~ I_QUOTAFULL;
    }
}

void recalc_bytes(dbref own)
{
    dbref i;

    for (i = 0; i < db_top; i++)
    {
        if (db[i].owner == own)
        {
            db[i].size = 0;
            db[i].i_flags |= I_UPDATEBYTES;
        }
    }

    atr_add(own, A_BYTESUSED, "0");
}

void add_quota(dbref who, int payment)
{
    char buf[20];

    sprintf(buf, "%d", atoi(atr_get(db[who].owner, A_RQUOTA)) + payment);
    atr_add(db[who].owner, A_RQUOTA, buf);

    recalc_bytes(db[who].owner);
}

int pay_quota(dbref who, int cost)
{
    int quota;
    char buf[20];

#ifdef NO_QUOTA
    return TRUE;
#endif

    if (db[db[who].owner].i_flags & I_QUOTAFULL)
    {
        return FALSE;
    }

    // determine quota 
    quota = atoi(atr_get(db[who].owner, A_RQUOTA));

    // enough to build? 
    quota -= cost;
    if ( quota < 0 )
    {
        return FALSE;
    }

    // doc the quota 
    sprintf(buf, "%d", quota);
    atr_add(db[who].owner, A_RQUOTA, buf);

    recalc_bytes(db[who].owner);

    return TRUE;
}

int sub_quota(dbref who, int cost)
{
    char buf[20];

    // doc the quota 
    sprintf(buf, "%d", atoi(atr_get(db[who].owner, A_RQUOTA)) - cost);
    atr_add(db[who].owner, A_RQUOTA, buf);

    recalc_bytes(db[who].owner);
    return TRUE;
}

// !!! index function hack !!! 
static int my_index(char *s1, int c)
{
    while(*s1 && (*s1++!=c))
    ;

    return(*s1);
}

int ok_attribute_name(char *name)
{
    return (name
        && *name
        && !strchr(name,'=')
        && !strchr(name,',')
        && !strchr(name,';')
        && !strchr(name,':')
        && !strchr(name,'.')
        && !strchr(name,'[')
        && !strchr(name,']')
        && !strchr(name,' '));
}

int ok_thing_name(char *name)
{
    return ok_name(name) && !my_index(name, ';');
}

int ok_exit_name(char *name)
{
    return ok_name(name);
}

int ok_room_name(char *name)
{
    return ok_name(name) && !my_index(name, ';');
}

static int ok_name(char *name)
{
    return (name
      && *name
      && *name != LOOKUP_TOKEN
      && *name != NUMBER_TOKEN
      && *name != NOT_TOKEN
      && !my_index(name, ARG_DELIMITER)
      && !my_index(name, AND_TOKEN)
      && !my_index(name, OR_TOKEN)
      && string_compare(name, "me")
      && string_compare(name, "home")
      && string_compare(name, "here"));
}

int ok_object_name(dbref obj, char *name)
{
    switch (Typeof(obj))
    {
        case TYPE_THING:
            return ok_thing_name(name);
        case TYPE_EXIT:
            return ok_exit_name(name);
        case TYPE_ROOM:
            return ok_room_name(name);
    }

    log_error("i'm very unhappy here in predicates.c. :(");

    return FALSE;
}

int ok_player_name(dbref player, char *name, char *alias)
{
    char *scan;

    if (!ok_name(name) || my_index(name,';') || strlen(name) > player_name_limit)
    {
        return FALSE;
    }

    if (!string_compare(name, "i") ||
        !string_compare(name, "me") ||
        !string_compare(name, "my") ||
        !string_compare(name, "you") ||
        !string_compare(name, "your") ||
        !string_compare(name, "he") ||
        !string_compare(name, "she") ||
        !string_compare(name, "it") ||
        !string_compare(name, "his") ||
        !string_compare(name, "her") ||
        !string_compare(name, "hers") ||
        !string_compare(name, "its") ||
        !string_compare(name, "we") ||
        !string_compare(name, "us") ||
        !string_compare(name, "our") ||
        !string_compare(name, "they") ||
        !string_compare(name, "them") ||
        !string_compare(name, "their") ||
        !string_compare(name, "a") ||
        !string_compare(name, "an") ||
        !string_compare(name, "the") ||
        !string_compare(name, "one") ||
        !string_compare(name, "to") ||
        !string_compare(name, "if") ||
        !string_compare(name, "and") ||
        !string_compare(name, "or") ||
        !string_compare(name, "but") ||
        !string_compare(name, "at") ||
        !string_compare(name, "of") ||
        !string_compare(name, "for") ||
        !string_compare(name, "foo") ||
        !string_compare(name, "so") ||
        !string_compare(name, "this") ||
        !string_compare(name, "that") ||
        !string_compare(name, ">") ||
        !string_compare(name, ".") ||
        !string_compare(name, "-") ||
        !string_compare(name, ">>") ||
        !string_compare(name, "..") ||
        !string_compare(name, "--") ||
        !string_compare(name, "->") ||
        !string_compare(name, ":)") ||
        !string_compare(name, "clear")) // +mail clear
    {
        return FALSE;
    }

    for(scan = name; *scan; scan++)
    {
        if (!isprint(*scan))
        {
            // was isgraph(*scan) 
            return FALSE;
        }
        if (*scan == '~')
        {
            return FALSE;
        }
    }

    if (lookup_player(name) != NOTHING && lookup_player(name) != player)
    {
        return FALSE;
    }

    if (!string_compare(name, alias))
    {
        return FALSE;
    }

    if (*name && name[strlen(name)-1] == ':')
    {
        char buf[BUF_SIZE];

        strcpy(buf,name);
        buf[strlen(buf)-1] = '\0';

        if ( (lookup_player(buf) != NOTHING)
          && (lookup_player(buf) != player)
           )
        {
            return FALSE;
        }
    }

    if (player != NOTHING)
    {
        // this isn't being created
        int min;

        if (*alias)
        {
            min = strlen(alias);
            if (!strchr(name,' '))
            {
                if (strlen(name)<min)
                {
                    min = strlen(name);
                }
            }
            if ( (lookup_player(alias) != player)
              && (lookup_player(alias) != NOTHING)
               )
            {
                return FALSE;
            }
        }
        else
        {
            if (strchr(name, ' '))
            {
                return FALSE;
            }

            min = strlen(name);
        }

        if (min > player_reference_limit)
        {
            return FALSE;
        }
    }

    if (*alias && (!ok_name(alias) || strchr(alias,' ')))
    {
        return FALSE;
    }

    if (strlen(name) > player_name_limit)
    {
        return FALSE;
    }

    return TRUE;
}

int ok_password(char *password)
{
    char *scan;

    if (*password == '\0')
    {
        return FALSE;
    }

    for (scan = password; *scan; scan++)
    {
        if (!(isprint(*scan) && !isspace(*scan)))
        {
            return FALSE;
        }
    }

    return TRUE;
}

char *main_exit_name(dbref exit)
{
    static char buf[BUF_SIZE];
    char *s;

    strcpy (buf, db[exit].name);

    if ((s=strchr(buf,';')))
    {
        *s = '\0';
    }

    return buf;
}

static void sstrcat(char *old, char *string, char *app)
{
    char *s;
    int hasnonalpha = 0;

    if ((strlen(app) + (s=(strlen(string)+string))-old) > 950)
    {
        return;
    }

    strcpy (s, app);

    for (; *s; s++)
    {
        if ((*s == '(') && !hasnonalpha)
        {
            *s = '<';
        }
        else
        {
            if ((*s==',') || (*s==';') || (*s == '['))
            {
                *s=' ';
            }
            if (!isalpha(*s) && *s != '#' && *s != '.')
            {
                hasnonalpha = 1;
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// pronoun_substitute()
///////////////////////////////////////////////////////////////////////////////
//
// %-type substitutions for pronouns
//
// %s/%S for subjective pronouns (he/she/it/e/they,      He/She/It/E/They)
// %o/%O for objective pronouns  (him/her/it/em/them,    Him/Her/It/Em/Them)
// %p/%P for possessive pronouns (his/her/its/eir/their, His/Her/Its/Eir/etc)
// %n    for the player's name.
//
// privs - object whose privileges are used
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void pronoun_substitute(char *result, dbref player, char *str, dbref privs)
{
    char buf[BUF_SIZE];
    char c, *s, *p;
    char *ores;
    dbref thing;
    ATTR *atr;
    int gender;

    static char *subjective[NUM_GENDERS] = { "it",  "he",  "she" };
    static char *possessive[NUM_GENDERS] = { "its", "his", "her" };
    static char  *objective[NUM_GENDERS] = { "it",  "him", "her" };

    if ((privs < 0) || (privs >= db_top))
    {
        privs = 2;
    }

    // figure out the player's gender 
    switch(*atr_get(player, A_SEX))
    {
        case 'M':
        case 'm':
            gender = MALE;
            break;
        case 'f':
        case 'F':
        case 'w':
        case 'W':
            gender = FEMALE;
            break;
        default:
            // Everything else
            gender = NEUTRAL;
    }

    strcpy(ores = result, spname(player));
    result += strlen(result);
    *result++ = ' ';

    while (*str && ((result-ores) < 1000))
    {
        if (*str=='[')
        {
            str++;
            exec(&str, buf, privs, player, 0);

            if ((strlen(buf) + (result-ores)) > 950)
            {
                continue;
            }

            strcpy(result, buf);
            result += strlen(result);

            if (*str==']')
            {
                str++;
            }
        }
        else if (*str == '%')
        {
            *result = '\0';
            c = *(++str);

            switch (c)
            {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if (!wptr[c-'0'])
                    {
                        break;
                    }

                    sstrcat(ores,result,wptr[c-'0']);
                    break;
                case 'v':
                case 'V':
                {
                    int a;
                    a = to_upper(str[1]);

                    if ((a<'A') || (a>'Z'))
                    {
                        break;
                    }

                    if (*str)
                    {
                        str++;
                    }

                    sstrcat(ores,result,atr_get(privs,A_V[a-'A']));
                }
                    break;
                case 's':
                case 'S':
                    sstrcat(ores, result, subjective[gender]);
                    break;
                case 'p':
                case 'P':
                   sstrcat(ores,result, possessive[gender]);
                   break;
                case 'o':
                case 'O':
                    sstrcat(ores,result, objective[gender]);
                    break;
                case 'n':
                case 'N':
                    sstrcat(ores,result, safe_name(player));
                    break;
                case '#':
                    if ((strlen(result)+result-ores) > 990)
                    {
                        break;
                    }
                    sprintf(result+strlen(result), "#%d", player);
                    break;
                case '/':
                   str++;

                   if ((s = strchr(str,'/')))
                   {
                       *s = '\0';

                       if ((p = strchr(str, ':')))
                       {
                           *p = '\0';
                           thing = (dbref) atoi(++str);
                           *p = ':';
                           str = ++p;
                       }
                       else
                       {
                           thing = privs;
                       }

                       atr = atr_str(privs,thing,str);

                       if (atr && can_see_atr(privs,thing,atr))
                       {
                           sstrcat(ores,result, atr_get(thing, atr));
                       }

                       *s = '/';
                       str = s;
                   }
                   break;
                case 'r':
                case 'R':
                    sstrcat(ores,result, "\n");
                    break;
                case 't':
                case 'T':
                    sstrcat(ores,result, "\t");
                    break;
                case 'a':
                case 'A':
                    if (!power(player, POW_BEEP))
                    {
                        sstrcat(ores, result, "");
                    }
                    else
                    {
                        sstrcat(ores, result, "\a");
                    }
                    break;
                default:
                    if ((result-ores) > 990)
                    {
                        break;
                    }
 
                    *result = *str;
                    result[1] = '\0';
                    break;
            }

            if (isupper(c) && c !='N')
            {
                *result = to_upper(*result);
            }

            result += strlen(result);

            if (*str)
            {
               str++;
            }
        }
        else
        {
            if ((result-ores) > 990)
            {
                break;
            }

            // check for escape 
            if ((*str=='\\') && (str[1]))
            {
                str++;
            }

            *result++ = *str++;
        }
    }

    *result = '\0';
}

void push_list(dbref **list, dbref item)
{
    int len;
    dbref *newlist;

    if ((*list) == NULL)
    {
        len = 0;
    }
    else
    {
        for (len = 0; (*list)[len] != NOTHING; len++)
        ;
    }

    len += 1;

    newlist = malloc((len+1)*sizeof(dbref));

    if (*list)
    {
        bcopy(*list, newlist, sizeof(dbref)*(len));
    }
    else
    {
        newlist[0] = NOTHING;
    }

    if (*list)
    {
        free(*list);
    }

    newlist[len-1] = item;
    newlist[len] = NOTHING;

    (*list) = newlist;
}

void remove_first_list(dbref **list, dbref item)
{
    int pos;

    if (!*list)
    {
        return;
    }

    for (pos=0; (*list)[pos] != item && (*list)[pos] != NOTHING; pos++)
    ;

    if ((*list)[pos] == NOTHING)
    {
        return;
    }

    for (; (*list)[pos] != NOTHING; pos++)
    {
        (*list)[pos] = (*list)[pos+1];
    }

    // and ignore the reallocation, unless we have to free it. 
    if ((*list)[0] == NOTHING)
    {
        free(*list);
        (*list) = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
// starts_with_player()
///////////////////////////////////////////////////////////////////////////////
//    Looks for a PC whose name starts with the string that's passed and
// returns the db reference to it.
///////////////////////////////////////////////////////////////////////////////
// Returns: (int) player character ID
///////////////////////////////////////////////////////////////////////////////
dbref starts_with_player(char *name)
{
    static char buf[BUF_SIZE];
    dbref ch_found;
    char *s;

    // Strip any ANSI color codes
    strcpy(buf, color(name, REMOVE));

    // Check for spaces
    if ((s = strchr(name, ' ')))
    {
        // Truncate after first word, only first one is important
        // for lookup_player()
        buf[s-name] = '\0';
    }

    // Lookup the player
    ch_found = lookup_player(buf);

    // Check results
    if (ch_found == NOTHING || string_compare(db[ch_found].name, buf))
    {
        return NOTHING;
    }
    else
    {
        // Found 'em!'
        return ch_found;
    }
}

int is_in_zone(dbref player, dbref zone)
{
    dbref zon;

    DOZONE(zon,player)
    {
        if (zon == zone)
        {
            return TRUE;
        }
    }

    return FALSE;
}

char *safe_name(dbref foo)
{
    if (Typeof(foo) == TYPE_EXIT)
    {
        return main_exit_name(foo);
    }

    return db[foo].name;
}
