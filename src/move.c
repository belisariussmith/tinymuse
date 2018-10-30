///////////////////////////////////////////////////////////////////////////////
// move.c                                                      TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: move.c,v 1.10 1993/09/18 19:04:11 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

#define Dropper(thing) (Hearer(thing) && (db[db[thing].owner].flags & CONNECT || db[thing].flags&CONNECT))

// External declarations
extern void enter_room();

void moveto(dbref what, dbref where)
{
    enter_room(what, where);
}

static void moveit(dbref what, dbref where)
{
    dbref loc,old;

    if (Typeof(what) == TYPE_EXIT && Typeof(where) == TYPE_PLAYER)
    {
        log_error("moving exit to player");
        report();
        return;
    }

    // remove what from old loc 
    if ((loc = old = db[what].location) != NOTHING)
    {
        if (Typeof(what) == TYPE_EXIT)
        {
            db[loc].exits = remove_first(db[loc].exits, what);
        }
        else
        {
            db[loc].contents = remove_first(db[loc].contents, what);
        }

        if (Hearer(what) && (old != where))
        {
            did_it(what, loc, A_LEAVE, NULL, A_OLEAVE, Dark(old) ? NULL : "has left.", A_ALEAVE);
        }

        db[loc].contents = remove_first(db[loc].contents, what);
    }

    // test for special cases 
    switch(where)
    {
        case NOTHING:
            db[what].location = NOTHING;
            return;            // NOTHING doesn't have contents 
        case HOME:
            if (Typeof(what) == TYPE_EXIT || Typeof(what) == TYPE_ROOM)
            {
                return;
            }
            where = db[what].link;    // home 
            break;
    }

    // now put what in where 
    if (Typeof(what) == TYPE_EXIT)
    {
        PUSH(what,db[where].exits);
    }
    else
    {
        PUSH(what, db[where].contents);
    }

    db[what].location = where;

    if (Hearer(what) && (where!=NOTHING) && (old!=where))
    {
        did_it(what, where, A_ENTER, NULL, A_OENTER, Dark(where) ? NULL : "has arrived.", A_AENTER);
    }
      
}

static void send_contents(dbref loc, dbref dest)
{
    dbref first;
    dbref rest;

    first = db[loc].contents;
    db[loc].contents = NOTHING;

    // blast locations of everything in list 
    DOLIST(rest, first)
    {
        db[rest].location = NOTHING;
    }

    while (first != NOTHING)
    {
        rest = db[first].next;

        if (Dropper(first))
        {
            db[first].location = loc;
            PUSH(first,db[loc].contents);
        }
        else
        {
            enter_room(first,(db[first].flags & STICKY) ? HOME : dest);
        }

        first = rest;
    }

    db[loc].contents = reverse(db[loc].contents);
}

static void maybe_dropto(dbref loc, dbref dropto)
{
    dbref thing;

    if (loc == dropto)
    {
        // bizarre special case 
        return;
    }

    if (Typeof(loc) != TYPE_ROOM)
    {
        return;
    }

    // check for players 
    DOLIST(thing, db[loc].contents)
    {
        if (Dropper(thing))
        {
            return;
        }
    }

    // no players, send everything to the dropto 
    send_contents(loc, dropto);
}

void enter_room(dbref player, dbref loc)
{
    char buf[BUF_SIZE];
    extern dbref speaker;
    dbref old;
    dbref zon;
    dbref dropto;
    int a1=0,a2=0,a3=0,a4=0,a5=0,a6=0;
    static int deep=0;
    dbref first;
    dbref rest;

    if (Typeof(player) == TYPE_ROOM)
    {
        send_message(speaker, perm_denied());
        return;
    }

    if (Typeof(player) == TYPE_EXIT && !controls(player, loc, POW_MODIFY) && !controls(speaker, loc, POW_MODIFY))
    {
        send_message(speaker, perm_denied());
        return;
    }

    if (deep++ > 15)
    {
        deep--;
        return;
    }

    if (Typeof(loc) == TYPE_EXIT)
    {
        sprintf(buf, "Attempt to move %d to exit %d", player, loc);
        log_error(buf);
        report();
        deep--;
        return;
    }

    // check for room == HOME 
    if (loc == HOME)
    {
        // home 
        loc = db[player].link;
    }

    // get old location 
    old = db[player].location;

    // check for self-loop 
    // self-loops don't do move or other player notification 
    // but you still get autolook and penny check 

    // go there 
    if (loc != old)
    {
        did_it(player, player, A_MOVE, NULL, A_OMOVE, NULL, A_AMOVE);
    }

    moveit(player, loc);

    // monkied around with this code a bit, make sure it works right - Belisarius
    if (loc != old)
    {
        DOZONE(zon, player)
        {
            did_it(player, zon, A_MOVE, NULL, A_OMOVE, NULL, A_AMOVE);
        }
    }

    // check for following players in the room... 

    first = db[old].contents;

    while (first != NOTHING)
    {
        rest = db[first].next;

        sprintf(buf, "#%d", player);

        if (!string_compare(buf, atr_get(first, A_FOLLOW)) && (db[first].flags & CONNECT))
        {
            if (could_doit(first, player, A_LFOLLOW) && (*atr_get(player, A_LFOLLOW)))
            {
                sprintf(buf, "%s follows %s.",db[first].name, db[player].name); 
                notify_in(old, first, buf); 

                enter_room(first, loc);

                sprintf(buf, "%s follows %s into the room.", db[first].name, db[player].name);
                notify_in(loc, first, buf);
            }
            else
            {
                send_message(first, "%s no longer wants you to follow.", db[player].name);
                
                sprintf(buf, "%s stops following %s.", db[first].name, db[player].name);
                notify_in(old, first, buf);
                   
                atr_add(first, A_FOLLOW, "\0");
            }
        }

        first = rest;
    }

    // if old location has STICKY dropto, send stuff through it 
    if ( (a1 = (loc != old)) 
      && (a2 = Dropper(player))
      && (a3 = (old != NOTHING))
      && (a4 = (Typeof(old) == TYPE_ROOM))
      && (a5 = ((dropto = db[old].location) != NOTHING))
      && (a6 = (db[old].flags & STICKY))
       )
    {
        maybe_dropto(old, dropto);
    }


    // autolook 
    look_room(player, loc);

    deep--;
}


///////////////////////////////////////////////////////////////////////////////
// safe_tel()
///////////////////////////////////////////////////////////////////////////////
//    Teleports player to location while removing items they shouldnt take 
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void safe_tel(dbref player, dbref dest)
{
    dbref first;
    dbref rest;

    if (Typeof(player) == TYPE_ROOM)
    {
        return;
    }

    if (dest == HOME)
    {
        dest = db[player].link;
    }

    if (db[db[player].location].owner == db[dest].owner)
    {
        enter_room(player, dest);
        return;
    }

    first = db[player].contents;
    db[player].contents = NOTHING;

    // blast locations of everything in list 
    DOLIST(rest, first)
    {
        db[rest].location = NOTHING;
    }

    while (first != NOTHING)
    {
        rest = db[first].next;

        // if thing is ok to take then move to player else send home 
        if (controls(player,first,POW_MODIFY) || (!(IS(first,TYPE_THING,THING_KEY)) && !(db[first].flags & STICKY)))
        {
            PUSH(first,db[player].contents);
            db[first].location = player;
        }
        else
        {
            enter_room(first,HOME);
        }

        first = rest;
    }

    db[player].contents = reverse(db[player].contents);

    enter_room(player,dest);
}

///////////////////////////////////////////////////////////////////////////////
// can_move()
///////////////////////////////////////////////////////////////////////////////
//    This checks to see whether or not a player can move in a specified
// direction.
///////////////////////////////////////////////////////////////////////////////
// Returns: Boolean 
///////////////////////////////////////////////////////////////////////////////
int can_move(dbref player, char *direction)
{
    if (Typeof(player) == TYPE_ROOM)
    {
        // Rooms can't move!
        return FALSE;
    }
    if (!string_compare(direction, "home"))
    {
        // You can always go home
        return TRUE;
    }

    // otherwise match on exits 
    init_match(player, direction, TYPE_EXIT);
    match_exit();

    return (last_match_result() != NOTHING);
}

///////////////////////////////////////////////////////////////////////////////
// do_move()
///////////////////////////////////////////////////////////////////////////////
//     Move a player from one place to another depending on the direction.
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void do_move(dbref player, char *direction)
{
    char buf[BUF_SIZE];
    dbref exit;
    dbref loc;
    // dbref zresult;  // never used (see below) - Belisarius
    dbref old = db[player].location;

    if (Typeof(player) == TYPE_ROOM)
    {
        send_message(player, "Sorry, rooms aren't allowed to move.");
        return;
    }

    if (!string_compare(direction, "home"))
    {
        // send him home 
        // but.. steal all his possessions 
        if (Typeof(player) == TYPE_ROOM || Typeof(player) == TYPE_EXIT)
        {
            return;
        }

        if (db[player].location == db[player].link)
        {
            send_message(player, "But you're already there!");
            return;
        }

        if (!check_zone(player, player, db[player].link, 2)) return;

        if (((loc = db[player].location) != NOTHING) && !IS(loc, TYPE_ROOM, ROOM_AUDITORIUM))
        {
            // tell everybody else 
            sprintf(buf, "%s goes home.", db[player].name);
            notify_in(loc, player, buf);
        }

        // give the player the messages 
        send_message(player, "There's no place like home...");
        send_message(player, "There's no place like home...");
        send_message(player, "There's no place like home...");
        safe_tel(player, HOME);
    }
    else
    {
        // find the exit
        init_match_check_keys(player, direction, TYPE_EXIT);
        match_exit();

        switch(exit = match_result())
        {
            case NOTHING:
                // try to force the object 
                send_message(player, "You can't go that way.");
                break;
            case AMBIGUOUS:
                send_message(player, "I don't know which way you mean!");
                break;
            default:
                // we got one 
                // check to see if we got through 
                if (could_doit(player, exit, A_LOCK))
                {
                    // if ((zresult = check_zone(player, player, db[exit].link, 0))) {
                    sprintf(buf, "goes through the exit marked %s.", main_exit_name(exit));
                    did_it(player, exit, A_SUCC, NULL, A_OSUCC, (db[exit].flags & DARK) ? NULL : glurpdup(buf), A_ASUCC);

                    switch(Typeof(db[exit].link))
                    {
                        case TYPE_ROOM:
                            enter_room(player, db[exit].link);
                            break;
                        case TYPE_PLAYER:
                        case TYPE_THING:
                            if (db[db[exit].link].flags & GOING)
                            {
                                send_message(player, "You can't go that way.");
                                return;
                            }
                            if (db[db[exit].link].location == NOTHING)
                            {
                                return;
                            }
                            safe_tel(player, db[exit].link);
                            break;
                        case TYPE_EXIT:
                            send_message(player, "This feature coming soon.");
                            break;
                    }

                    sprintf(buf, "arrives from %s", db[old].name);
                    did_it(player, exit, A_DROP, NULL, A_ODROP, (db[exit].flags & DARK) ? NULL : glurpdup(buf), A_ADROP);

                    //
                    // zresult is never assigned. It was previously, but that assignment
                    // was commented out up above.
                    // - Belisarius
                    //
                    //if (zresult > (dbref) 1)
                    //{
                    //    // we entered a new zone
                    //    did_it(player, zresult, A_DROP, NULL, A_ODROP, NULL, A_ADROP);
                    //}
                }
                else
                {
                    did_it(player, exit, A_FAIL, "You can't go that way.", A_OFAIL, NULL, A_AFAIL);
                }
                break;
        }
    }
}

void do_get(dbref player, char *what)
{
    dbref thing;
    dbref loc = db[player].location;

    if (Typeof(player) == TYPE_EXIT)
    {
        send_message(player, "You can't pick up things!");
        return;
    }

    if ((Typeof(loc) != TYPE_ROOM) && !(db[loc].flags & ENTER_OK) && !controls(player, loc, POW_TELEPORT))
    {
        send_message(player, perm_denied());
        return;
    }

    init_match_check_keys(player, what, TYPE_THING);
    match_neighbor();
    match_exit();

    if (power(player, POW_TELEPORT))
    {
        // the wizard has long fingers 
        match_absolute();
    }

    if ((thing = noisy_match_result()) != NOTHING)
    {
        if (db[thing].location == player)
        {
            send_message(player, "You already have that!");
            return;
        }

        switch(Typeof(thing))
        {
            case TYPE_PLAYER:
            case TYPE_THING:
                if (could_doit(player, thing, A_LOCK))
                {
                    moveto(thing, player);
                    send_message(thing, "You have been picked up by %s.", unparse_object(thing, player));
                    did_it(player, thing, A_SUCC, "Taken.", A_OSUCC, NULL, A_ASUCC);
                }
                else
                {
                    did_it(player, thing, A_FAIL, "You can't pick that up.", A_OFAIL,NULL,A_AFAIL);
                }
                break;
            case TYPE_EXIT:
                send_message(player,"You can't pick up exits.");
                return;
            default:
                send_message(player, "You can't take that!");
                break;
        }
    }
}

void do_drop(dbref player, char *name)
{
    dbref loc;
    dbref thing;
    char buf[MAX_CMD_BUF];

    if ((loc = getloc(player)) == NOTHING)
    {
        return;
    }

    init_match(player, name, TYPE_THING);
    match_possession();

    switch(thing = match_result())
    {
        case NOTHING:
            send_message(player, "You don't have that!");
            return;
        case AMBIGUOUS:
            send_message(player, "I don't know which you mean!");
            return;
        default:
            if (db[thing].location != player)
            {
                // Shouldn't ever happen. 
                send_message(player, "You can't drop that.");
            }
            else if (Typeof(thing) == TYPE_EXIT)
            {
                send_message(player, "Sorry you can't drop exits.");
                return;
            }
            else if (db[thing].flags & STICKY)
            {
                send_message(thing, "Dropped.");
                safe_tel(thing,HOME);
            }
            else if (db[loc].link != NOTHING && (Typeof(loc) == TYPE_ROOM) && !(db[loc].flags & STICKY))
            {
                // location has immediate dropto 
                send_message(thing, "Dropped.");
                moveto(thing, db[loc].link);
            }
            else
            {
                send_message(thing, "Dropped.");
                enter_room(thing, loc);
                // sprintf(buf, "%s dropped %s.", db[player].name, db[thing].name);
                // notify_in(loc, player, buf); 
            }
            break;
    }

    sprintf(buf, "dropped %s.", db[thing].name);
    did_it(player,thing,A_DROP, "Dropped.", A_ODROP, buf, A_ADROP);
}

void do_enter(dbref player, char *what)
{
    dbref thing;

    init_match_check_keys(player, what, TYPE_THING);
    match_neighbor();
    match_exit();

    if (power(player, POW_TELEPORT))
    {
        // the wizard has long fingers 
        match_absolute();
    }

    if ((thing = noisy_match_result()) == NOTHING)
    {
        //  send_message(player,"I don't see that here."); 
        return;
    }

    switch(Typeof(thing))
    {
        case TYPE_ROOM:
        case TYPE_EXIT:
            send_message(player, perm_denied());
            break;
        default:
            if (!(db[thing].flags & ENTER_OK) && !controls(player, thing, POW_TELEPORT))
            {
                did_it(player, thing, A_EFAIL, "You can't enter that.", A_OEFAIL, NULL, A_AEFAIL);
                return;
            }

            if (could_doit(player, thing, A_ELOCK) // && check_zone(player, player, thing, 0) 
               )
            {
                safe_tel(player,thing);
            }
            else
            {
                did_it(player, thing, A_EFAIL, "You can't enter that.", A_OEFAIL, NULL, A_AEFAIL);
            }
            break;
    }
}

void do_leave(dbref player)
{
    if (Typeof(db[player].location) == TYPE_ROOM || db[db[player].location].location == NOTHING)
    {
        send_message(player, "you can't leave. you're stuck here forever! mwahahah!");
        return;
    }

    if (could_doit(player, db[player].location, A_LLOCK))
    {
        enter_room(player, db[db[player].location].location);
    }
    else
    {
        did_it(player, db[player].location, A_LFAIL, "you can't leave. you're stuck here forever! mwahahahah!", A_OLFAIL, NULL, A_ALFAIL);
    }
}

///////////////////////////////////////////////////////////////////////////////
// get_room()
///////////////////////////////////////////////////////////////////////////////
//    Find and return the room the object/player is in 
///////////////////////////////////////////////////////////////////////////////
// Returns: (int) database object reference to room
///////////////////////////////////////////////////////////////////////////////
dbref get_room(dbref thing)
{
    dbref holder;
    int depth = 10;

    // Loop through "holders"
    for (holder = thing; depth; depth--, holder = thing, thing = db[holder].location)
    {
        if (Typeof(thing) == TYPE_ROOM)
        {
            // Found the room
            return thing;
        }
    }

    // Not contained by anything? (or buried too deep)
    return (dbref) 0;
}

void do_follow(dbref player, char *arg1)
{
    char buf[BUF_SIZE];
    dbref leader;

    if (!*arg1)
    {
        if (*atr_get(player, A_FOLLOW))
        {
            dbref old;
            char name[16];

            sscanf(atr_get(player, A_FOLLOW), "#%s", name);
            old = atoi(name);

            send_message(player, "You stop following %s.", db[old].name);

            sprintf(buf, "%s stops following %s", db[player].name, db[old].name);
            notify_in(db[player].location, player, buf);

            atr_add(player, A_FOLLOW, "\0");
        }
        else
        {
            send_message(player, "You aren't following anyone.");
        }
        return;
    }

    init_match(player, arg1, TYPE_THING || TYPE_PLAYER);
    match_neighbor();
    match_me();

    if (power(player, POW_REMOTE))
    {
        match_absolute();
        match_player();
    }

    switch (leader = match_result())
    {
        case NOTHING:
            send_message(player, "Follow who?");
            break;
        case AMBIGUOUS:
            send_message(player, "I don't know who you mean!");
            break;
        default:
            if ((Typeof(leader) == TYPE_ROOM) || (Typeof(leader) == TYPE_EXIT) || (Typeof(player) == TYPE_ROOM) || (Typeof(player) == TYPE_EXIT))
            {
                send_message(player, "Permission denied.");
                return;
            }

            if (!(*atr_get(leader, A_LFOLLOW)))
            {
                send_message(player, "Permission denied.");
                return;
            }

            if (!could_doit(player, leader, A_LFOLLOW) || (player == leader))
            {
                send_message(player, "Permission denied.");
                return;
            }

            sprintf(buf, "#%d", player);

            if (!string_compare(buf, atr_get(leader, A_FOLLOW)))
            {
                send_message(player, "But, %s is already following you!", db[leader].name);
                return;
            }

            if (*atr_get(player, A_FOLLOW))
            {
                dbref old;
                char name[16];

                sscanf(atr_get(player, A_FOLLOW), "#%s", name);
                old = atoi(name);

                send_message(player, "You stop following %s.", db[old].name);

                sprintf(buf, "%s stops following %s.", db[player].name, db[old].name);
                notify_in(db[player].location, player, buf);
            }

            send_message(player, "You are now following %s.", db[leader].name);

            sprintf(buf, "%s is now following %s.", db[player].name, db[leader].name);
            notify_in(db[player].location, player, buf);

            sprintf(buf, "#%d", leader);
            atr_add(player, A_FOLLOW, buf);

            break;
    }
}
