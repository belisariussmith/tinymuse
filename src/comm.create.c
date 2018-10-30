// create.c
// $Id: create.c,v 1.15 1994/02/18 22:51:55 nils Exp $

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "externs.h"

extern char *BROKE_MESSAGE;

// utility for open and link
static dbref parse_linkable_room(dbref player, char *room_name)
{
    dbref room;

    // skip leading NUMBER_TOKEN if any
    if (*room_name == NUMBER_TOKEN)
    {
        room_name++;
    }

    // parse room
    if(!string_compare(room_name, "here"))
    {
        room = db[player].location;
    }
    else if(!string_compare(room_name, "home"))
    {
        // HOME is always linkable
        return HOME;
    }
    else
    {
        room = parse_dbref(room_name);
    }

    // check room
    if (room < 0 || room >= db_top || Typeof(room) == TYPE_EXIT)
    {
        // !!! if Wizard and unsafe do it any ways !!!
        //    if (power(player, TYPE_DIRECTOR) && unsafe)
        // return(room);*/
        if (room < 0 || room >= db_top)
        {
            send_message(player, "#% is not a valid object.",room);
        }
        else
        {
            send_message(player, "%s is an exit!",unparse_object(player,room));
        }

        return NOTHING;
    }
    else if (!can_link_to(player, room, POW_MODIFY))
    {
        // !!! ... !!! XX
        //    if (power(player, TYPE_DIRECTOR) && unsafe)
        // return(room);*/
        send_message(player, "You can't link to %s.", unparse_object(player,room));
        return NOTHING;
    }
    else
    {
        return room;
    }
}

// use this to create an exit
void do_open(dbref player, char *direction, char *linkto, dbref pseudo)
{
    dbref loc = (pseudo != NOTHING) ? pseudo : db[player].location;
    // dbref pseudo;  // a phony location for a player if a back exit is needed
    dbref exit;

    //dbref loczone;
    //dbref linkzone;
    // appear to be assigned, but unused -- Belisarius

    if ((loc == NOTHING) || (Typeof(loc) == TYPE_PLAYER))
    {
        send_message(player, "Sorry you can't make an exit there.");
        return;
    }


    if (!*direction)
    {
        send_message(player, "Open where?");
        return;
    }
    else if (!ok_exit_name(direction))
    {
        send_message(player, "%s is a strange name for an exit!", direction);
        return;
    }

    if(!controls(player, loc, POW_MODIFY) && !(db[loc].flags & ROOM_DIG_OK))
    {
        send_message(player, perm_denied());
    }
    else if ( can_pay_fees(def_owner(player), exit_cost, QUOTA_COST) )
    {
        // create the exit
        exit = new_object();

        // initialize everything
        SET(db[exit].name, direction);
        db[exit].owner  = def_owner(player);
        db[exit].zone   = NOTHING;
        db[exit].flags  = TYPE_EXIT;
        db[exit].flags |= (db[db[exit].owner].flags & INHERIT_POWERS);
        check_spoofobj(player, exit);

        // link it in
        PUSH(exit, Exits(loc));
        db[exit].location = loc;
        db[exit].link = NOTHING;

        // and we're done
        send_message(player, "%s opened.", direction);

        // check second arg to see if we should do a link
        if (*linkto != '\0')
        {
            if ((loc = parse_linkable_room(player, linkto)) != NOTHING)
            {
                if (!payfor(player, link_cost))
                {
                    send_message(player, "You don't have enough Credits to link.");
                }
                else
                {
                    //loczone = get_zone_first(player);
                    //linkzone = get_zone_first(loc);
                    // appear to be unused -- Belisarius

                    // it's ok, link it
                    db[exit].link = loc;
                    send_message(player, "Linked to %s.", unparse_object(player, loc));
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// do_link()
///////////////////////////////////////////////////////////////////////////////
//     Use this to link to a room that you own, it seizes ownership of the exit 
// costs 1 penny, plus a penny transferred to the exit owner if they aren't you 
// you must own the linked-to room AND specify it by room number 
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void do_link(dbref player, char *name, char *room_name)
{
    dbref thing;
    dbref room;
    char buf[BUF_SIZE];

    init_match(player, name, TYPE_EXIT);
    match_everything();

    if ((thing = noisy_match_result()) != NOTHING)
    {
        switch(Typeof(thing))
        {
            case TYPE_EXIT:
                if ( (room = parse_linkable_room(player, room_name)) == NOTHING)
                {
                    return;
                }

                if ( (room != HOME) && !controls(player, room, POW_MODIFY) && ! (db[room].flags & LINK_OK))
                {
                    send_message(player,perm_denied());
                    break;
                }

                // we're ok, check the usual stuff
                if (db[thing].link != NOTHING)
                {
                    if (controls(player, thing, POW_MODIFY))
                    {
                        //if (Typeof(db[thing].link) == TYPE_PLAYER)
                        //    send_message(player, "That exit is being carried.");
                        //else
                        send_message(player, "%s is already linked.", unparse_object(player,thing));
                    }
                    else
                    {
                        send_message(player, perm_denied());
                    }
                }
                else
                {
                    // handle costs
                    if (db[thing].owner == db[player].owner)
                    {
                        if (!payfor(player, link_cost))
                        {
                            send_message(player, "It costs a Credit to link this exit.");
                            return;
                        }
                    }
                    else
                    {
                        if ( ! can_pay_fees(def_owner(player), link_cost+exit_cost, QUOTA_COST) )
                        {
                            return;
                        }
                        else
                        {
                            // pay the owner for his loss
                            giveto(db[thing].owner, exit_cost);
                            add_quota(db[thing].owner, QUOTA_COST);
                        }
                    }

                    // link has been validated and paid for; do it
                    db[thing].owner = def_owner(player);

                    if (!(db[player].flags & INHERIT_POWERS))
                    {
                        db[thing].flags &= ~(INHERIT_POWERS);
                    }

                    db[thing].link = room;

                    // notify the player
                    send_message(player, "%s linked to %s.", unparse_object_a(player, thing), unparse_object_a(player, room));
                }
                break;
            case TYPE_PLAYER:
            case TYPE_THING:
                init_match(player, room_name, NOTYPE);
                match_exit();
                match_neighbor();
                match_possession();
                match_me();
                match_here();
                match_absolute();
                match_player();

                if ((room = noisy_match_result()) < 0)
                {
                    // send_message(player,"No match."); noisy_match_result talks bout it
                    return;
                }

                if (Typeof(room) == TYPE_EXIT)
                {
                    send_message(player, "%s is an exit.",unparse_object(player, room));
                    return;
                }

                // abode
                if (!controls(player,room,POW_MODIFY) && !(db[room].flags&LINK_OK))
                {
                    send_message(player, perm_denied());
                    break;
                }

                if (!controls(player, thing, POW_MODIFY) && ((db[thing].location!=player) || !(db[thing].flags & LINK_OK )))
                {
                    send_message(player, perm_denied());
                }
                else if (room == HOME)
                {
                    send_message(player, "Can't set home to home.");
                }
                else
                {
                    // do the link
                    db[thing].link = room; // home
                    send_message(player, "Home set to %s.", unparse_object(player, room));
                }
                break;
            case TYPE_ROOM:
                if ((room = parse_linkable_room(player, room_name)) == NOTHING)
                {
                    return;
                }

                if (Typeof(room) != TYPE_ROOM)
                {
                    send_message(player, "%s is not a room!",
                    unparse_object(player, room));
                    return;
                }

                if ((room != HOME) && !controls(player, room, POW_MODIFY) && !(db[room].flags & LINK_OK))
                {
                    send_message(player, perm_denied());
                    break;
                }

                if (!controls(player, thing, POW_MODIFY))
                {
                    send_message(player, perm_denied());
                }
                else
                {
                    // do the link, in location....no, in link! yay!
                    db[thing].link = room; // dropto
                    send_message(player, "Dropto set to %s.", unparse_object(player, room));
                }
                break;
            default:
                send_message(player, "Internal error: weird object type.");
                sprintf(buf, "PANIC weird object: Typeof(%d) = %ld", (int)thing, Typeof(thing));
                log_error(buf);
                report();
                break;
        }
    }
}

// Links a room to a "zone object"
void do_zlink(dbref player, char *arg1, char *arg2)
{
    dbref room;
    dbref object;
    //dbref old_zone; // Appears to not be assigned, but never used -- Belisarius

    init_match(player, arg1, TYPE_ROOM);
    match_here();
    match_absolute();
    if ((room = noisy_match_result()) != NOTHING)
    {
        init_match(player, arg2, TYPE_THING);
        match_neighbor();
        match_possession();
        match_absolute();

        //old_zone = get_zone_first(room);  // Assigned but never used -- Belisarius
        if ((object = noisy_match_result()) != NOTHING)
        {
            if ( (!(controls(player, room, POW_MODIFY) && controls(player, object, POW_MODIFY)))
	          || (Typeof(room) != TYPE_ROOM && Typeof(room)!= TYPE_THING && !power(player, POW_SECURITY))
               )
            {
                send_message(player, perm_denied());
            }
            else if (is_in_zone(object, room))
            {
	            send_message(player, "I think you're feeling a bit loopy.");
            }
            else
            {
                if (db[object].zone == NOTHING && object != db[0].zone)
                {
                    // silly person doesn't know
                    // how to set up a zone
                    // right..
	            db[object].zone = db[0].zone;
                }
                db[room].zone = object;

                send_message(player, "%s zone set to %s", db[room].name, db[object].name);
            }
        }
    }
}

void do_unzlink(dbref player, char *arg1)
{
    dbref room;

    init_match(player, arg1, TYPE_ROOM);
    match_here();
    match_absolute();

    if ((room = noisy_match_result()) != NOTHING)
    {
        if (!controls(player, room, POW_MODIFY))
        {
            send_message(player, perm_denied());
        }
        else
        {
            if (Typeof(room) == TYPE_ROOM)
            {
                db[room].zone = db[0].zone;
            }
            else
            {
                db[room].zone = NOTHING;
            }

            send_message(player, "Zone unlinked.");
        }
    }

    return;
}

// special link - sets the universal zone
void do_ulink(dbref player, char *arg1)
{
    dbref thing, p, oldu;

    if (player != root)
    {
        send_message(player, "You don't have the authority. So sorry.");
        return;
    }

    init_match(player, arg1, TYPE_THING);
    match_possession();
    match_neighbor();
    match_absolute();

    if ((thing = noisy_match_result()) != NOTHING)
    {
        oldu = db[0].zone;
        db[0].zone = thing;

        for (p = 0; p < db_top; p++)
        {
            if ((Typeof(p) == TYPE_ROOM) && !(db[p].flags & GOING) && ((db[p].zone == oldu) || (db[p].zone == NOTHING)))
            {
               db[p].zone = thing;
            }
        }
    }

    db[thing].zone = NOTHING;
    send_message(player, "Universal zone set to %s.", db[thing].name);
}

// use this to create a room
void do_dig(dbref player, char *name, char *argv[])
{
    dbref room;
    dbref where;

    // we don't need to know player's location!  hooray!
    where = db[player].location;  // MMZ

    if (*name == '\0')
    {
        send_message(player, "Dig what?");
    }
    else if (!ok_room_name(name))
    {
        send_message(player, "That's a silly name for a room!");
    }
    else if ( can_pay_fees(def_owner(player), room_cost, QUOTA_COST) )
    {
        room = new_object();

        // Initialize everything
        SET(db[room].name,name);
        db[room].owner = def_owner(player);
        db[room].flags = TYPE_ROOM;
        db[room].location = room;
        db[room].zone = db[where].zone;
        db[room].flags |= (db[db[room].owner].flags & INHERIT_POWERS);
        check_spoofobj(player,room);
        send_message(player, "%s created with room number %d.", name, room);

        if (argv[1] && *argv[1])
        {
            char nbuff[50];
            sprintf(nbuff,"%d",room);
            do_open(player,argv[1],nbuff,NOTHING);
        }

        if (argv[2] && *argv[2])
        {
            char nbuff[50];
            sprintf(nbuff,"%d",db[player].location);
            do_open(player,argv[2],nbuff,room);
        }
    }
}

void check_spoofobj(dbref player, dbref thing)
{
    dbref p;

    if ((p = starts_with_player(db[thing].name)) != NOTHING && !controls(player, p, POW_SPOOF) && !(db[thing].flags & HAVEN))
    {
        send_message(player, "Warning: %s can be used to spoof an existing player. It has as been set haven.",unparse_object(player,thing));
        db[thing].flags |= HAVEN;
    }
}

// use this to create an object
void do_create(dbref player, char *name, int cost)
{
    dbref loc;
    dbref thing;

    if(*name == '\0')
    {
        send_message(player, "Create what?");
        return;
    }
    else if (!ok_thing_name(name))
    {
        send_message(player, "That's a silly name for a thing!");
        return;
    }
    else if (cost < 0)
    {
        send_message(player, "You can't create an object for less than nothing!");
        return;
    }
    else if (cost < thing_cost)
    {
        cost = thing_cost;
    }

    if (can_pay_fees(def_owner(player), cost, QUOTA_COST))
    {
        // create the object
        thing = new_object();

        // initialize everything
        SET(db[thing].name,name);
        db[thing].location = player;
        db[thing].zone     = NOTHING;
        db[thing].owner    = def_owner(player);
        db[thing].flags    = TYPE_THING;
        db[thing].flags   |= (db[db[thing].owner].flags & INHERIT_POWERS);
        check_spoofobj(player, thing);

        // endow the object
        s_Pennies(thing, OBJECT_ENDOWMENT(cost));
        
        if (Pennies(thing) > MAX_OBJECT_ENDOWMENT)
        {
            s_Pennies(thing, MAX_OBJECT_ENDOWMENT);
        }

        // home is here (if we can link to it) or player's home
        if ((loc = db[player].location) != NOTHING && controls(player, loc, POW_MODIFY))
        {
            db[thing].link = loc;
        }
        else
        {
            db[thing].link = db[player].link;
        }

        db[thing].exits = NOTHING;

        // link it in
        PUSH(thing, db[player].contents);

        // and we're done
        send_message(player, "%s created.", unparse_object(player,thing));
    }
}


void do_clone(dbref player, char *arg1, char *arg2)
{
    dbref clone,thing;

    if ( Guest(db[player].owner) )
    {
        send_message(player, "Guests can't clone objects\n");
        return;
    }

    init_match(player, arg1, NOTYPE);
    match_everything();

    thing = noisy_match_result();

    if ((thing == NOTHING) || (thing == AMBIGUOUS))
    {
        return;
    }

    if ( !controls(player, thing, POW_SEEATR))
    {
        send_message(player, perm_denied());
        return;
    }

    if (Typeof(thing) != TYPE_THING)
    {
        send_message(player, "You can only clone things.");
        return;
    }

    if (!can_pay_fees(def_owner(player), thing_cost, QUOTA_COST))
    {
        send_message(player, BROKE_MESSAGE);
        return;
    }

    // Make new object
    clone = new_object();
    // Literal copy
    memcpy(&db[clone], &db[thing], sizeof(struct object));

    db[clone].name  = NULL;
    db[clone].owner = def_owner(player);
    db[clone].flags&=~(HAVEN|BEARING); // common parent flags

    if (!(db[player].flags & INHERIT_POWERS))
    {
        db[clone].flags&=~INHERIT_POWERS;
    }

    SET(db[clone].name,(*arg2)?arg2:db[thing].name);

    check_spoofobj(player,clone);

    // Set the pennies of the close to 1
    s_Pennies(clone, 1);

    // copies the noninherited attributes.
    atr_cpy_noninh(clone, thing);

    db[clone].contents = db[clone].location = db[clone].next = NOTHING;
    db[clone].atrdefs  = NULL;
    db[clone].parents  = NULL;
    db[clone].children = NULL;

    PUSH_L (db[clone].parents, thing);
    PUSH_L (db[thing].children, clone);

    // Let the player know
    send_message(player, "%s cloned with number %d.", unparse_object(player,thing),clone);

    // Place in the right area
    moveto(clone, db[player].location);

    // Let everyone know
    did_it(player, clone, NULL, NULL, NULL, NULL, A_ACLONE);
}

void do_robot(dbref player, char *name, char *pass)
{
    dbref thing;

    if (!power(player,POW_PCREATE))
    {
        send_message(player,"You can't make robots.");
        return;
    }

    if ( ! can_pay_fees(def_owner(player), robot_cost, QUOTA_COST) )
    {
        send_message(player,"Sorry you don't have enough money to make a robot.");
        return;
    }

    if ((thing = create_player(name, pass, CLASS_VISITOR, player_start)) == NOTHING)
    {
        giveto(player, robot_cost);
        add_quota(player, QUOTA_COST);

        send_message(player, "%s already exists.",name);

        return;
    }

    db[thing].owner = db[player].owner;
    atr_clr(thing, A_RQUOTA);

    enter_room(thing,db[player].location);

    send_message(player, "%s has arrived.",unparse_object(player,thing));
}
