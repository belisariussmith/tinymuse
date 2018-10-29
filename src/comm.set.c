// set.c 
// $Id: set.c,v 1.23 1994/01/26 22:28:27 nils Exp $ 
// commands which set parameters 

#include <stdio.h>
#include <ctype.h>
#include <crypt.h> // added for crypt() 

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "match.h"
#include "interface.h"
#include "externs.h"
#include "credits.h"

static struct hearing
{
    dbref obj;
    int did_hear;
    struct hearing *next;
} *hearing_list=NULL;

void do_recycle(dbref player, char *name)
{
    dbref thing;

    if (controls(player, db[player].location, POW_MODIFY))
    {
        init_match(player, name, NOTYPE);
    }
    else
    {
        init_match (player, name, TYPE_THING);
    }

    if (controls(player, db[player].location, POW_MODIFY))
    {
        match_exit();
    }

    match_everything();
    thing = match_result();

    if ( (thing != NOTHING)
      && (thing != AMBIGUOUS)
      && !controls(player, thing, POW_MODIFY)
      && !( Typeof(thing) == TYPE_THING && (db[thing].flags & THING_DEST_OK) )
       )
    {
        send_message(player, perm_denied());
        return;
    }

    if (db[thing].children && (*db[thing].children) != NOTHING)
    {
        send_message (player, "Warning: It has children.");
    }

    if (thing < 0)
    {
        // I hope no wizard is this stupid but just in case 
        send_message(player, "I don't know what that is, sorry.");
        return;
    }

    if (thing == 0 || thing == 1 || thing == player_start || thing == root)
    {
        send_message(player, "Don't you think that's sorta an odd thing to destroy?");
        return;
    }

    // what kind of thing we are destroying? 
    char *k;
    switch(Typeof(thing))
    {
        case TYPE_PLAYER:
            send_message(player,"destroying players isn't allowed, try a @nuke instead.");
            return;
        case TYPE_THING:
        case TYPE_ROOM:
        case TYPE_EXIT:

            k = atr_get(thing, A_DOOMSDAY);

            if (*k)
            {
                if (db[thing].flags & GOING)
                {
                    send_message(player, "It seems it's already gunna go away in %s... if you wanna stop it, use @undestroy", time_format_2(atol(k)-now));
                    return;
                }
                else
                {
                    send_message(player, "Sorry, it's protected.");
                }
            }
            else
            {
                if (db[thing].flags & GOING)
                {
                    send_message(player, "It seems to already be destroyed.");
                    return;
                }
                else
                {
                    k = atr_get (player, A_DOOMSDAY);
                    if (*k)
                    {
                        destroy_obj (thing, atoi(k));
                        send_message(player, "Okay, %s will go away in %s.", unparse_object (player, thing), time_format_2(atoi(k)));
                    }
                    else
                    {
                        destroy_obj (thing, atoi(default_doomsday));
                        send_message(player, "Okay, %s will go away in %s.", unparse_object(player, thing), time_format_2(atoi(default_doomsday)));
                    }
                }
            }
            break;
        default:
            break;
    }
}

void destroy_obj (dbref obj, int no_seconds)
{
    if (!(db[obj].flags & QUIET))
    {
        do_pose(obj, "shakes and starts to crumble", "", 0);
    }

    atr_add (obj, A_DOOMSDAY, int_to_str (no_seconds + now));
    db[obj].flags |= GOING;
    do_halt (obj, "");
}

void do_name(dbref player, char *name, char *newname, int is_direct)
{
    char buf[BUF_SIZE];
    dbref thing;
    char *password;

    if ((thing = match_controlled(player, name, POW_MODIFY)) != NOTHING)
    {
        // check for bad name 
        if (*newname == '\0')
        {
            send_message(player, "Give it what new name?");
            return;
        }

        // check for renaming a player 
        if (Typeof(thing) == TYPE_PLAYER)
        {
            if (!is_direct)
            {
                send_message(player, "sorry, players must change their names directly from a net connection.");
                return;
            }
            if ( player == thing && ! power(player, POW_MEMBER))
            {
                send_message(player, "Sorry, only registered %s users may change their name.", muse_name);
                return;
            }

            if ((password = strrchr(newname,' ')))
            {
                while (isspace(*password))
                {
                    password--;
                }

                password++;
                *password = '\0';
                password++;

                while (isspace(*password))
                {
                    password++;
                }
            }
            else
            {
                password = newname + strlen(newname);
            }

            // check for reserved player names 
            if ( string_prefix(newname, guest_prefix) )
            {
                send_message(player, "Only guests may have names beginning with '%s'", guest_prefix);
                return;
            }

            // check for null password 
            if(!*password)
            {
                send_message(player, "You must specify a password to change a player name.");

                send_message(player, "E.g.: name player = newname password");
                return;
            }
            else if ( *Pass(player)
                   && strcmp(Pass(player), password)
                   && strcmp(crypt(password, "XX"), Pass(player))
                    )
            {
                send_message(player, "Incorrect password.");
                return;
            }
            else if ( !(ok_player_name(thing, newname, atr_get(thing,A_ALIAS))))
            {
                send_message(player, "You can't give a player that name.");
                return;
            }

            // everything ok, notify
            sprintf(buf, "NAME CHANGE: %s(#%d) to %s", db[thing].name, thing, newname);
            log_sensitive(buf);
            sprintf(buf, "%s is now known as %s.", db[thing].name, newname);
            notify_in(db[thing].location, thing, buf); 
            delete_player(thing);
            SET(db[thing].name,newname);
            add_player(thing);
            send_message(player, "Name set.");

            return;
        }

        // we're an object. 
        if ( ! ok_object_name(thing, newname) )
        {
            send_message(player, "That is not a reasonable name.");
            return;
        }

        // everything ok, change the name 
        if (Hearer(thing))
        {
            sprintf(buf, "%s is now known as %s.", db[thing].name, newname);
            notify_in(db[thing].location, thing, buf);
        }

        SET(db[thing].name, newname);
        check_spoofobj (player, thing);
        send_message(player, "Name set.");
    }
}

void do_describe(dbref player, char *name, char *description)
{
    dbref thing;

    if ((thing = match_controlled(player, name,POW_MODIFY)) != NOTHING)
    {
        s_Desc(thing,description);
        send_message(player, "Description set.");
    }
}

void do_unlink(dbref player, char *name)
{
    dbref exit;

    init_match(player, name, TYPE_EXIT);
    match_exit();
    match_here();

    if (power(player, POW_REMOTE))
    {
        match_absolute();
    }

    switch(exit = match_result())
    {
        case NOTHING:
            send_message(player, "Unlink what?");
            break;
        case AMBIGUOUS:
            send_message(player, "I don't know which one you mean!");
            break;
        default:
            if (!controls(player, exit, POW_MODIFY))
            {
                send_message(player, perm_denied());
            }
            else
            {
                switch(Typeof(exit))
                {
                    case TYPE_EXIT:
                        db[exit].link = NOTHING;
                        send_message(player, "Unlinked.");
                        break;
                    case TYPE_ROOM:
                        db[exit].link = NOTHING;
                        send_message(player, "Dropto removed.");
                        break;
                    default:
                        send_message(player, "You can't unlink that!");
                        break;
                }
            }
    }
}

void do_chown(dbref player, char *name, char *newobj)
{
    char buf[BUF_SIZE];
    dbref thing;
    dbref owner;

    sprintf(buf, "Player %s(#%d) attempts: @chown %s=%s",	db[player].name, player, name, newobj);
    log_sensitive(buf);

    init_match(player, name, TYPE_THING);
    match_possession();
    match_here();
    match_exit();
    match_absolute();

    switch(thing = match_result())
    {
        case NOTHING:
            send_message(player, "You don't have that!");
            return;
        case AMBIGUOUS:
            send_message(player, "I don't know which you mean!");
            return;
    }

    if(!*newobj || !string_compare(newobj,"me"))
    {
        owner = def_owner(player); // @chown thing or @chown thing=me 
    }
    else if ((owner = lookup_player(newobj)) == NOTHING)
    {
        send_message(player, "I couldn't find that player.");
    }

    if (power(player,POW_SECURITY))
    {
        if (Typeof(thing) == TYPE_PLAYER && db[thing].owner != thing)
        {
            db[thing].owner = thing;
        }
    }

    if (owner == NOTHING)
    {
        // for the else 
    }
    else if ( (db[thing].owner == thing && Typeof(thing) == TYPE_PLAYER) && !power(player, POW_SECURITY))
    {
        // if non-robot player 
        send_message(player, "Players always own themselves.");
    }
    else if (! controls(player, owner, POW_CHOWN) ||
	   ( ! controls(player, thing, POW_CHOWN) &&
	    ( ! (db[thing].flags & CHOWN_OK) ||
	     ( (Typeof(thing) == TYPE_THING) &&
	      (db[thing].location != player && !power(player,POW_CHOWN)) ) )))
    {
        send_message(player, perm_denied());
    }
    else
    {
        if ( power(player, POW_CHOWN) )
        {
            // adjust quotas 
            add_quota(db[thing].owner, QUOTA_COST);
            sub_quota(db[owner].owner, QUOTA_COST);
            // adjust credits 
            payfor(player, thing_cost);
            giveto(db[thing].owner, thing_cost);
        }
        else
        {
            if ( Pennies(db[player].owner) < thing_cost )
            {
                send_message(player,"You don't have enough money.");
                 return;
            }

            // adjust quotas
            if ( ! pay_quota(owner, QUOTA_COST) )
            {
                send_message(player, (player == owner) ? "Your quota has run out." : "Nothing happens.");
	        return;
            }

            add_quota(db[thing].owner, QUOTA_COST);
            // adjust credits 
            payfor(player, thing_cost);
            giveto(db[thing].owner, thing_cost);
        }

        sprintf(buf, "Player %s(%d) succeeds with: @chown %s=%s", db[player].name, player, unparse_object_a(thing, thing), unparse_object_a(owner, owner));
        log_sensitive(buf);

        if (db[thing].flags&CHOWN_OK || !controls(player, db[owner].owner,POW_CHOWN))
        {
            db[thing].flags |= HAVEN;
            db[thing].flags &= ~CHOWN_OK;
            db[thing].flags &= ~INHERIT_POWERS;
        }

        db[thing].owner = db[owner].owner;
        send_message(player, "Owner changed.");
    }
}

void mark_hearing(dbref obj)
{
    int i;
    struct hearing *mine;

    mine           = malloc (sizeof (struct hearing));
    mine->next     = hearing_list;
    mine->did_hear = Hearer(obj);
    mine->obj      = obj;

    hearing_list = mine;

    for (i = 0; db[obj].children && db[obj].children[i] != NOTHING; i++)
    {
        mark_hearing(db[obj].children[i]);
    }
}

void check_hearing()
{
    char buf[BUF_SIZE];
    struct hearing *mine;
    int now_hear;
    dbref obj;

    while (hearing_list)
    {
        mine         = hearing_list;
        hearing_list = hearing_list->next;

        obj      = mine->obj;
        now_hear = Hearer(mine->obj);

        if (now_hear && !mine->did_hear)
        {
            sprintf(buf, "%s grows ears and can now hear.", db[obj].name);
            notify_in(db[obj].location, obj, buf);
        }
	
        if (mine->did_hear && !now_hear)
        {
            sprintf(buf, "%s loses its ears and is now deaf.", db[obj].name);
            notify_in(db[obj].location, obj, buf);
        }
	
        free(mine);
    }
}

void do_unlock(dbref player, char *name)
{
    dbref thing;
    ATTR *attr = A_LOCK;

    if ((thing = match_controlled(player, name, POW_MODIFY)) == NOTHING)
    {
        return;
    }

    if ( thing == root && player != root ) 
    {
        send_message(player, "Not likely.");
        return;
    }

    atr_add(thing, attr, "");
    send_message(player, "Unlocked.");
}

void do_set(dbref player, char *name, char *flag, int allow_commands)
{
    dbref thing;
    char *p;
    char *q;
    object_flag_type f;
    int her;
    dbref leader;
    char leader_buf[BUF_SIZE];
    char buf[BUF_SIZE];

    // find thing 
    if ((thing = match_thing(player, name)) == NOTHING)
    {
        return;
    }

    if ( thing == root && player != root )
    {
        send_message(player, "Only root can set him/herself!");
        return;
    }

    if (!*atr_get(db[thing].owner, A_BYTESUSED))
    {
        recalc_bytes(db[thing].owner);
    }

    her = Hearer(thing);

    // get leader
    if (*atr_get(thing, A_LEADER))
    {
        sscanf(atr_get(thing, A_LEADER), "#%s", leader_buf);
        leader = atoi(leader_buf);
    }

    if (leader < 0 || leader > db_top)
    {
        leader = NOTHING;
    }

    // check for attribute set first 
    for( p = flag; *p && (*p != ':'); p++ )
    ;

    if ( *p )
    {
        ATTR *attr;

        *p++='\0';
        if ( ! (attr = atr_str(player, thing, flag)) )
        {
            send_message(player, "Sorry that isn't a valid attribute.");
            return;
        }
        // if ( (attr->flags & AF_WIZARD) && !((attr->obj == NOTHING)?power(player, POW_WATTR):controls(player,attr->obj,POW_WATTR)))
        // {
        //     send_message(player, "Sorry only Administrators can change that.");
        //     return;
        // }
        if (!can_set_atr(player, thing, attr) && attr != A_USERS)
        {
            send_message(player, "You can't set that attribute.");
            return;
        }
        if (attr == A_USERS && (!can_set_atr(player, thing, attr) ||
            !power(player, POW_WATTR)) && (player != leader))
        {
            send_message(player, "Sorry, you may not set its users.");
            return;
        }
        if (attr == A_ALIAS && Typeof(thing) != TYPE_PLAYER)
        {
            send_message(player, "Sorry only players can have aliases.");
            return;
        }
        if (attr == A_ALIAS && !ok_player_name(thing, db[thing].name, p))
        {
            send_message(player, "you can't set alias to that.");
            return;
        }
        if (db[db[thing].owner].i_flags & I_QUOTAFULL && strlen(p) > strlen(atr_get(thing, attr)))
        {
            send_message(player, "your quota has run out.");
            return;
        }
        if (attr->flags & AF_LOCK)
        {
            if ((q = process_lock(player, p)))
            {
                db[thing].mod_time = now;
                atr_add(thing, attr, q);

                if (*q)
                {
                    send_message(player, "Locked.");
                }
                else
                {
                    send_message(player, "Unlocked.");
                }
            }
            return;
        }

        if (attr == A_ALIAS)
        {
            delete_player(thing);
        }

        mark_hearing(thing);

        if (!allow_commands && (*p == '!' || *p == '$'))
        {
            // urgh. fix it
            char *x;
            x = newglurp(strlen(p) + 2);
            strcpy(x, "_");
            strcat(x, p);
            p = x;
        }

        db[thing].mod_time = now;
        atr_add(thing, attr, p);

        if (attr == A_ALIAS)
        {
            add_player(thing);
        }

        if (!(db[player].flags & QUIET))
        {
            send_message(player, "%s - Set.", db[thing].name);
        }

        check_hearing();

        return;
    }

    // move p past NOT_TOKEN if present
    for( p = flag; *p && (*p == NOT_TOKEN || isspace(*p)); p++)
    ;

    // identify flag
    if(*p == '\0')
    {
        send_message(player, "You must specify a flag to set.");
        return;
    }

    f = 0;

    switch(Typeof(thing))
    {
        case TYPE_THING:
            if ( string_prefix("KEY", p) )
            {
                f = THING_KEY;
            }
            if ( string_prefix("DESTROY_OK", p) )
            {
                f = THING_DEST_OK;
            }
            if ( string_prefix("LIGHT", p) )
            {
                f = THING_LIGHT;
            }
            //	if ( string_prefix("ROBOT",p) )
            //  {
            //      f = THING_ROBOT;
            //  }
            if ( string_prefix("X_OK", p) )
            {
                f = THING_SACROK;
            }
            if ( string_prefix("DIG_OK", p) )
            {
                f = THING_DIG_OK;
            }
            break;
        case TYPE_PLAYER:
            if ( string_prefix("NEWBIE", p) || string_prefix("NOVICE", p) )
            {
                f = PLAYER_NEWBIE;
            }
            if ( string_prefix("SLAVE", p) )
            {
                f = PLAYER_SLAVE;
            }
            if ( string_prefix("TERSE", p) )
            {
                f = PLAYER_TERSE;
            }
            if ( string_prefix("ANSI", p) || string_prefix("@ANSI", p) )
            {
                f = PLAYER_ANSI;
            }
            if ( string_prefix("MORTAL", p) )
            {
                f = PLAYER_MORTAL;
            }
            if ( string_prefix("NO_ANNOUNCE", p) || string_prefix("ANNOUNCE", p) )
            {
                f = PLAYER_NO_ANN;
            }
            if ( string_prefix("NO_COM", p) || string_prefix("+COM", p) )
            {
                f = PLAYER_NO_COM;
            }
            if ( string_prefix("NO_WALLS", p) )
            {
                f = PLAYER_NO_WALLS;
            }
            break;
        case TYPE_ROOM:
            if ( string_prefix("ABODE", p) )
            {
                f = ROOM_JUMP_OK;
            }
            if ( string_prefix("AUDITORIUM", p) )
            {
                f = ROOM_AUDITORIUM;
            }
            if ( string_prefix("JUMP_OK", p) )
            {
                f = ROOM_JUMP_OK;
            }
            if ( string_prefix("FLOATING", p) )
            {
                f = ROOM_FLOATING;
            }
            if ( string_prefix("DIG_OK", p) )
            {
                f = ROOM_DIG_OK;
            }
            break;
        case TYPE_EXIT:
            if ( string_prefix("LIGHT",p) )
            {
                f = EXIT_LIGHT;
            }
            if (string_prefix("TRANSPARENT",p))
            {
                f = OPAQUE;
            }
            break;
    }
    if ( ! f )
    {
        if ( string_prefix("GOING", p) )
        {
            if (player != root || Typeof(thing) == TYPE_PLAYER)
            {
                send_message(player, "I think the @[un]destroy command is more what you're looking for.");
                return;
            }
            else
            {
                send_message(player, "I hope you know what you're doing.");
                f = GOING;
            }
        }
        else if (string_prefix ("BEARING", p))
        {
	        f = BEARING;
        }
        else if( string_prefix("LINK_OK", p) )
        {
            f = LINK_OK;
        }
        else if ( string_prefix("QUIET",p) )
        {
            f = QUIET;
        }
        else if( string_prefix("DARK", p) )
        {
            f = DARK;
        }
        else if( string_prefix("DEBUG", p) )
        {
            f = DARK;
        }
        else if( string_prefix("STICKY", p) )
        {
            f = STICKY;
        }
        else if( string_prefix("PUPPET",p) )
        {
            f = PUPPET;
        }
        else if ( string_prefix("INHERIT",p) )
        {
            f = INHERIT_POWERS;
        }
        else if( string_prefix("ENTER_OK",p) )
        {
            f = ENTER_OK;
        }
        else if( string_prefix("CHOWN_OK",p) )
        {
            f = CHOWN_OK;
        }
        else if( string_prefix("SEE_OK",p) )
        {
            send_message(player,"Warning: the see_ok flag has been renamed to 'visible'");
            f = SEE_OK;
        }
        else if( string_prefix("VISIBLE",p) )
        {
            f = SEE_OK;
        }
        //      else if( string_prefix("UNIVERSAL",p) )
        // f = UNIVERSAL;*/
        else if( string_prefix("OPAQUE",p) )
        {
            f = OPAQUE;
        }
        else if( string_prefix("HAVEN", p) || string_prefix("HALTED",p) )
        {
            f = HAVEN;
        }
        else
        {
            send_message(player, "I don't recognize that flag.");
            return;
        }
    }

    if (f == BEARING && *flag == NOT_TOKEN)
    {
        int i;

        for (i = 0; db[thing].children && db[thing].children[i] != NOTHING; i++)
        {
            if (db[db[thing].children[i]].owner != db[player].owner)
            {
                if (!controls (player, db[thing].children[i], POW_MODIFY))
                {
                    send_message(player, "Sorry, you don't control its child, %s.", unparse_object (player,db[thing].children[i]));
                    return;
                }
                else
                {
                    if (db[db[thing].children[i]].owner != db[thing].owner)
                    {
                        send_message(player, "Warning: you are locking in %s as a child.", unparse_object (player, db[thing].children[i]));
                    }
                }
            }
        }
    }

    // check for restricted flag 
    if (f == HAVEN && *flag == NOT_TOKEN)
    {
        dbref p;
        p = starts_with_player(db[thing].name);

        if (p != NOTHING && !controls(player, p, POW_SPOOF))
        {
            send_message(player, "Sorry, a player holds that name.");
            return;
        }
    }

    if (!controls(player, thing, POW_WFLAGS) || !power(player, POW_WFLAGS))
    {
        if (f == PLAYER_NO_COM || f == PLAYER_NO_ANN )
        {
            send_message(player, perm_denied());
            return;
        }
    }

    if ( Typeof(thing) == TYPE_PLAYER && f == PLAYER_SLAVE )
    {
        if (!has_pow(player, thing, POW_SLAVE) || db[player].owner == thing)
        {
            send_message(player, "You can't enslave/unslave that!");
            return;
        }
        sprintf(buf, "%s(#%d) %sslaved %s", db[player].name, player, (*flag == NOT_TOKEN) ? "un" : "en", unparse_object(thing, thing));
        log_sensitive(buf);
    }
    else
    {
        if (!controls (player, thing, POW_MODIFY))
        {
            send_message(player, perm_denied());
            return;
        }
    }

    if (!controls(player, db[thing].owner, POW_SECURITY) && f == INHERIT_POWERS)
    {
        send_message(player, "Sorry, you cannot do that.");
        return;
    }

    // else everything is ok, do the set 
    if( *flag == NOT_TOKEN )
    {
        // reset the flag 
        db[thing].flags &= ~f;
        send_message(player, "Flag reset.");

        if ( (f == PUPPET) && her && !Hearer(thing))
        {
            sprintf(buf, "%s loses its ears and becomes dead.", db[thing].name);
            notify_in(db[thing].location, thing, buf);
        }
    }
    else
    {
        // set the flag 
        db[thing].flags |= f;

        if ( (f == PUPPET) && !her )
        {
            sprintf(buf, "%s grows ears and can now hear.", db[thing].name);
            notify_in(db[thing].location, thing, buf);
        }

        send_message(player, "Flag set.");
    }
}


// check for abbreviated set command 
int test_set(dbref player, char *command, char *arg1, char *arg2, int is_direct)
{
    extern ATTR *builtin_atr_str P((char *));
    ATTR *a;
    char buff[BUF_SIZE*2];

    if (command[0] != '@')
    {
        return(FALSE);
    }

    if (!(a=builtin_atr_str (command+1)))
    {
        init_match (player, arg1, NOTYPE);
        match_everything();

        if (match_result()!=NOTHING && match_result()!=AMBIGUOUS)
        {
            a = atr_str(player, match_result(), command+1);

            if (a)
            {
                sprintf (buff, "%s:%s",command+1,arg2);
                do_set (player, arg1, buff, is_direct);
                return(TRUE);
            }
        }
    }
    else if (!(a->flags & AF_NOMOD))
    {
        sprintf(buff, "%s:%s", command+1, arg2);
        do_set(player, arg1, buff, is_direct);
        return TRUE;
    }

    return(FALSE);
}

int parse_attrib(dbref player, char *s, dbref *thing, ATTR **atr, int withpow)
{
    char buff[BUF_SIZE];
    strcpy(buff, s);

    // get name up to / 
    for (s = buff; *s && (*s!='/'); s++)
    ;

    if (!*s)
    {
        return FALSE;
    }

    *s++ = 0;

    if (withpow != 0)
    {
        if ((*thing = match_controlled(player, buff, withpow)) == NOTHING)
        {
            return FALSE;
        }
    }
    else
    {
        init_match (player, buff, NOTYPE);
        match_everything();

        if ((*thing = match_result()) == NOTHING)
        {
            return FALSE;
        }
    }

    // rest is attrib name
    if (!((*atr) = atr_str(player, *thing, s)))
    {
        return FALSE;
    }

    if (withpow != 0)
    {
        if ( ((*atr)->flags & AF_DARK)
          || (!controls(player, *thing, POW_SEEATR) && !((*atr)->flags & AF_OSEE))
           )

        {
          return FALSE;
        }
    }

    return TRUE;
}

void do_edit(dbref player, char *it, char *argv[])
{
    dbref thing;
    int d,len;
    char *r,*s,*val;
    char dest[BUF_SIZE];
    ATTR *attr;

    if (!parse_attrib(player, it, &thing, &attr, POW_MODIFY))
    {
        send_message(player, "No match.");
        return;
    }

    if(!attr)
    {
        send_message(player, "Gack! Don't do that. it makes me uncomfortable.");
        return;
    }

    if ( (attr->flags & AF_WIZARD)
      && (!power(player, POW_WATTR) || !controls(player, thing, POW_MODIFY))
       )
                                
    {
        send_message(player, "Eeg! Tryin to edit a admin-only prop? hrm. don't do it.");
        return;
    }

    if (!controls(player, thing, POW_MODIFY))
    {
        send_message(player, perm_denied());
        return;
    }

    if (attr == A_ALIAS)
    {
        send_message(player, "To set an alias, do @alias me=<new alias>. Don't use @edit.");
        return;
    }

    if (!argv[1] || !*argv[1])
    {
        send_message(player, "Nothing to do.");
        return;
    }

    val =  argv[1];
    r   = (argv[2]) ? argv[2] : "";

    // replace all occurances of val with r 
    s = atr_get(thing, attr);
    len = strlen(val);

    for (d = 0; (d < 1000) && *s ; )
    {
        if (strncmp(val, s, len) == 0)
        {
            if ((d+strlen(r)) < 1000)
            {
                strcpy(dest+d, r);
                d += strlen(r);
                s += len;
            }
            else
            {
                dest[d++]= *s++;
            }
        }
        else
        {
            dest[d++] = *s++;
        }
    }

    dest[d++] = 0;

    if (db[db[thing].owner].i_flags & I_QUOTAFULL && strlen(dest) > strlen(atr_get(thing, attr)))
    {
        send_message(player,"your quota has run out.");
        return;
    }

    atr_add(thing,attr,dest);

    if (!(db[player].flags&QUIET))
    {
        send_message(player, "set.");
        do_examine(player, it, 0);
    }
}

void do_hide(dbref player, char *arg1)
{
    char buf[BUF_SIZE];
    dbref who;
    int hide_all = TRUE;

    if (*arg1)
    {
        who = lookup_player(arg1);
        if (!who)
        {
            return;
        }
    }
    else
    {
        hide_all = TRUE;
    }

    if (hide_all || who)
    {
        if (hide_all)
        {
            sprintf(buf, "#%d%c%c#%d", player, AND_TOKEN, NOT_TOKEN, player);
            atr_add(player, A_LHIDE, buf);
            send_message(player, "You are now hidden from all users.");
        }
        else if (could_doit(who, player, A_LHIDE))
        {
            if (*atr_get(player, A_LHIDE))
            {
                sprintf(buf, "#%s%c%c#%d", atr_get(player, A_LHIDE), AND_TOKEN, NOT_TOKEN, who);
                atr_add(player, A_LHIDE, buf);
                send_message(player, "You are now also hiding from %s.", unparse_object(player, who));
            }
            else
            {
                sprintf(buf, "#%c#%d", NOT_TOKEN, who);
                atr_add(player, A_LHIDE, buf);
                send_message(player, "You are now hiding from %s.", unparse_object(player, who));
            }
        }
        else
        {
            send_message(player, "You're already hiding from that person.");
        }
    }
}

void do_unhide(dbref player, char *arg1)
{
    char list[BUF_SIZE*2];
    char buf[BUF_SIZE];
    dbref who;
    char *ptr;
    int unhide_all = FALSE;
    char *parse    = 0;

    if (*arg1)
    {
        who = lookup_player(arg1);

        if (!who)
        {
            return;
        }
    }
    else
    {
        unhide_all = TRUE;
    }

    if (unhide_all || who)
    {
        if (unhide_all)
        {
            if (*atr_get(player, A_LHIDE))
            {
                atr_add(player, A_LHIDE, NULL);
                send_message(player, "You are now completely unhidden.");
            }
            else
            {
                send_message(player, "You're already completely unhidden.");
            }
        }
        else if (!could_doit(who, player, A_LHIDE))
        {
            strcpy(list, atr_get(player, A_LHIDE));
            atr_add(player, A_LHIDE, NULL);
            ptr = list;

            while (ptr)
            {
                parse = parse_up(&ptr, AND_TOKEN);

                if (!parse)
                {
                    ptr = parse;
                }
                else
                {
                    parse++;
                    parse++;

                    if (atoi(parse) != who)
                    {
                        if (!(*atr_get(player, A_LHIDE)))
                        {
                            sprintf(buf, "%c#%d", NOT_TOKEN, atoi(parse));
                            atr_add(player, A_LHIDE, buf);
                        }
                        else
                        {
                            sprintf(buf, "%s%c%c#%d", atr_get(player, A_LHIDE), AND_TOKEN, NOT_TOKEN, atoi(parse));
                            atr_add(player, A_LHIDE, buf);
                        }
                    }
                }
            }

            if (*atr_get(player, A_LHIDE))
            {
                send_message(player, "You are no longer hidden from %s.", unparse_object(player, who));
            }
            else
            {
                send_message(player, "You are now completely unhidden.");
            }
        }
        else
        {
            send_message(player, "You're not hidden from that person.");
        }
    }
}

void do_haven(dbref player, char *haven)
{
    if (*haven == '?')
    {
        if (*atr_get(player, A_HAVEN))
        {
            send_message(player, "Your Haven message is: %s", atr_get(player,A_HAVEN));
            return;
        }
        else
        {
            send_message(player, "You have no Haven message.");
            return;
        }
    }

    if (*haven =='\0')
    {
        atr_clr(player, A_HAVEN);
        send_message(player, "Haven message removed.");
        return;
    }

    atr_add(player, A_HAVEN, haven);
    send_message(player, "Haven message set as: %s", haven);
}

void do_idle(dbref player, char *idle)
{
    if (*idle == '?')
    {
        if (*Idle(player))
        {
            send_message(player, "Your Idle message is: %s", atr_get(player,A_IDLE));
            return;
        }
        else
        {
            send_message(player,"You have no Idle message.");
            return;
        }
    }

    if (*idle =='\0')
    {
        atr_clr(player, A_IDLE);
        send_message(player, "Idle message removed.");
        return;
    }

    atr_add(player, A_IDLE, idle);
    send_message(player, "Idle message set as: %s", idle);
}

void do_away(dbref player, char* away)
{
    if (*away == '?')
    {
        if (*Away(player))
        {
            send_message(player, "Your Away message is: %s", atr_get(player, A_AWAY));
            return;
        }
        else
        {
            send_message(player, "You have no Away message.");
            return;
        }
    }

    if (*away == '\0')
    {
        atr_clr(player, A_AWAY);
        send_message(player, "Away message removed.");
        return;
    }

    atr_add(player, A_AWAY, away);
    send_message(player, "Away message set as: %s", away);
}
