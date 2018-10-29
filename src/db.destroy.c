///////////////////////////////////////////////////////////////////////////////
// destroy.c                                               TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: destroy.c,v 1.8 1993/09/18 19:03:56 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <ctype.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "externs.h"

static int gstate = 0;
static struct object *o;
static int thing;

dbref first_free = NOTHING;
static void dbmark(dbref loc);
static void dbunmark();
static void dbmark1();
static void dbunmark1();
static void dbmark2();
static void mark_float();

// External declarations
extern dbref update_bytes_counter;

// things must be completely dead before using this.. use do_empty first.
static void free_object(dbref obj)
{
    db[obj].next = first_free;
    first_free = obj;
}

#define CHECK_REF(thing) if((thing)<-3 ||  (thing)>=db_top || ((thing)>=-1 && IS_GONE(thing)))
//#define CHECK_REF(a) if ((((a)>-1) && (db[a].flags & GOING)) || (a>=db_top) || (a<-3))

// check for free list corruption 
#define NOT_OK(thing)\
  ((db[thing].location!=NOTHING) || ((db[thing].owner!=1) && (db[thing].owner != root)) ||\
   ((db[thing].flags&~(0x8000))!=(TYPE_THING | GOING)))

/////////////////////////////////////////////////////////////////////
// free_get()
/////////////////////////////////////////////////////////////////////
//     Return a cleaned up object off the free list or NOTHING
/////////////////////////////////////////////////////////////////////
// Returns: (int) reference to database object
/////////////////////////////////////////////////////////////////////
dbref free_get()
{
    char buf[BUF_SIZE];
    dbref newobj;

    if (first_free == NOTHING)
    {
        return(NOTHING);
    }

    newobj     = first_free;
    first_free = db[first_free].next;

    // Make sure this object really should be in free list 
    if (NOT_OK(newobj))
    {
        static int nrecur = 0;

        if (nrecur++ == 20)
        {
	    first_free = NOTHING;
	    report();
	    log_error("Removed free list and continued");
  	    return(NOTHING);
	}

        report();
        sprintf(buf, "Object #%d shouldn't free, fixing free list", newobj);
        log_error(buf);
        fix_free_list();
        nrecur--;

        return(free_get());
    }

    // free object name 
    SET(db[newobj].name, NULL);

    return(newobj);
}

static int object_cost(dbref thing)
{
    char buf[BUF_SIZE];

    switch(Typeof(thing))
    {
        case TYPE_THING:
            return OBJECT_DEPOSIT(Pennies(thing));
        case TYPE_ROOM:
            return room_cost;
        case TYPE_EXIT:
            if (db[thing].link != NOTHING)
            {
                return exit_cost;
            }
            else
            {
                return exit_cost + link_cost;
            }
        case TYPE_PLAYER:
            return 1000;
        default:
            sprintf(buf, "Illegal object type: %ld, thing_cost", Typeof(thing));
            log_error(buf);
            return 5000;
    }
}

// go through and rebuild the free list 
void fix_free_list()
{
    char buf[BUF_SIZE];
    dbref thing;
    char *ch;

    first_free = NOTHING;

    // destroy all rooms+make sure everything else is really dead 
    for (thing = 0; thing < db_top; thing++)
    {
        if (IS_DOOMED(thing))
        {
            if ((atol(ch = atr_get(thing,A_DOOMSDAY)) < now) && (atol(ch) > 0))
            {
                do_empty(thing);
            }
        }
        else
        {
            // if something other than room make sure it is located in NOTHING
            // otherwise undelete it, needed incase @tel used on object 
            if (NOT_OK(thing))
            {
                db[thing].flags&=~GOING;
            }
        }
    }

    first_free = NOTHING;
    // check for references to destroyed objects */

    for (thing = db_top-1; thing >= 0; thing--)
    {
        // if object is alive make sure it doesn't refer to any dead objects 
        if (!IS_GONE(thing))
        {
	    CHECK_REF(db[thing].exits)

	    switch(Typeof(thing))
            {
	        case TYPE_PLAYER:
	        case TYPE_THING:
	        case TYPE_ROOM:
                {
                    // yuck probably corrupted set to nothing 
                    sprintf(buf, "Dead exit in exit list (first) for room #%d: %d",thing, db[thing].exits);
	            log_error(buf);
	            report();
	            db[thing].exits = NOTHING;
	        }
	    }

            CHECK_REF(db[thing].zone)

            switch(Typeof(thing))
            {
	        case TYPE_ROOM:
	            log_error(buf);
                    sprintf(buf, "Zone for #%d is #%d! setting it to the global zone.", thing, db[thing].zone);
	            db[thing].zone = db[0].zone;
                    break;
	    }

	    CHECK_REF(db[thing].link)

            switch(Typeof(thing))
            {
                case TYPE_PLAYER:
                case TYPE_THING:
                    db[thing].link = player_start;
                    break;
                case TYPE_EXIT:
                case TYPE_ROOM:
                    db[thing].link = NOTHING;
                    break;
            }

            CHECK_REF(db[thing].location)

            switch(Typeof(thing))
            {
                case TYPE_PLAYER: // this case shouldn't happen but just incase 
                case TYPE_THING:
                    db[thing].location = NOTHING;
                    moveto(thing, player_start);
                    break;
                case TYPE_EXIT:
                    db[thing].location = NOTHING;
                    destroy_obj(thing, atoi(bad_object_doomsday));
                    break;
                case TYPE_ROOM:
                    db[thing].location = thing; // rooms are in themselves
                    break;
            }

            if (((db[thing].next < 0) || (db[thing].next >= db_top)) && (db[thing].next != NOTHING))
            {
                sprintf(buf, "Invalid next pointer from object %s(%d)",db[thing].name, thing);
                log_error(buf);
                report();
                db[thing].next = NOTHING;
            }

            if ((db[thing].owner < 0) || (db[thing].owner >= db_top) || Typeof(db[thing].owner) != TYPE_PLAYER)
            {
                log_error(buf);
                sprintf(buf, "Invalid object owner %s(%d): %d",db[thing].name,thing, db[thing].owner);
                report();
                db[thing].owner=root;
                db[thing].flags|=HAVEN;
            }
        }
        else
        {
	    // if object is dead stick in free list 
	    free_object(thing);
        }
    }

    // mark all rooms that can be reached from limbo 
    dbmark(player_start);
    mark_float();
    dbmark2();
    // look through list and inform any player with an unconnected room 
    dbunmark();
}

// Check data base for disconnected rooms and exits 
static void dbmark(dbref loc)
{
    dbref thing;

    if ( (loc < 0) || (loc >= db_top) || (db[loc].i_flags & I_MARKED) || (Typeof(loc) != TYPE_ROOM))
    {
        return;
    }

    if (Typeof(loc) == TYPE_ROOM)
    {
        db[loc].i_flags |= I_MARKED;
    }

    // recursively trace 
    for (thing = Exits(loc); thing != NOTHING; thing = db[thing].next)
    {
        dbmark(db[thing].link);
    }
}

static void dbmark2()
{
    dbref loc;

    for (loc = 0; loc < db_top; loc++)
    {
        if (Typeof(loc) == TYPE_PLAYER || Typeof(loc) == TYPE_THING)
        {
            if (db[loc].link != NOTHING)
            {
                dbmark(db[loc].link);
            }

            if (db[loc].location != NOTHING)
            {
                dbmark(db[loc].location);
            }
        }
    }
}

static void dbunmark()
{
    char buf[BUF_SIZE];
    dbref loc;
    int num_rooms_disconnected = 0;
    int num_exits_disconnected = 0;

    for (loc = 0; loc < db_top; loc++)
    {
        if (Typeof(loc) == TYPE_EXIT && (db[loc].link == NOTHING))
        {
            num_exits_disconnected++;
        }

        if (db[loc].i_flags & I_MARKED)
        {
            db[loc].i_flags&=~I_MARKED;
        }
        else
        {
            if (Typeof(loc) == TYPE_ROOM)
            {
              	num_rooms_disconnected++;

                if (*atr_get(db[loc].owner, A_USERS))
                {
                    sprintf(buf, "*** Warning: Room '%s' is disconnected.", unparse_object(db[loc].owner, loc));
                    notify_group(db[loc].owner, buf);
          	}
                else
                {
                    sprintf(buf, "*** Warning: Room '%s' is disconnected.", unparse_object(root, loc));
                    send_message(db[loc].owner, buf);
                }

                sprintf(buf, "[%s] * Room #%d\t[owner=#%d, zone=#%d] is disconnected.", dbinfo_chan, loc, db[loc].owner, db[loc].zone);
                com_send(dbinfo_chan, buf);
                
            	dest_info(NOTHING,loc);
            }
        }
    }

    if (num_rooms_disconnected == NONE)
    {
        sprintf(buf, "[%s] * There are no disconnected rooms.", dbinfo_chan);
    }
    else if (num_rooms_disconnected == 1)
    {
        sprintf(buf, "[%s] * There is one disconnected room.", dbinfo_chan);
    }
    else
    {
        sprintf(buf, "[%s] * There are %d disconnected rooms.", dbinfo_chan, num_rooms_disconnected);
    }

    com_send(dbinfo_chan, buf);

    if (num_exits_disconnected == NONE)
    {
        sprintf(buf, "[%s] * There are no unlinked exits.", dbinfo_chan);
    }
    else if (num_exits_disconnected == 1)
    {
        sprintf(buf, "[%s] * There is one unlinked exit.", dbinfo_chan);
    }
    else
    {
        sprintf(buf, "[%s] * There are %d unlinked exits.", dbinfo_chan, num_exits_disconnected);
    }

    com_send(dbinfo_chan, buf);
}

// Check data base for disconnected objects 
static void dbmark1()
{
    char buf[BUF_SIZE];
    dbref thing;
    dbref loc;

    for (loc = 0; loc < db_top; loc++)
    {
        if (Typeof(loc)!=TYPE_EXIT)
        {
            for (thing=db[loc].contents;thing!=NOTHING;thing=db[thing].next)
            {
                if ((db[thing].location!=loc) || (Typeof(thing)==TYPE_EXIT))
                {
                    sprintf(buf, "Contents of object %d corrupt at object %d cleared", loc,thing);
                    log_error(buf);
	            db[loc].contents = NOTHING;
	            break;
                }

                db[thing].i_flags |= I_MARKED;
            }

            for (thing=db[loc].exits; thing!=NOTHING; thing=db[thing].next)
            {
                if ((db[thing].location != loc) || (Typeof(thing)!=TYPE_EXIT))
                {
                    sprintf(buf, "Exits of object %d corrupt at object %d. cleared.", loc, thing);
                    log_error(buf);
                    db[loc].exits = NOTHING;
                    break;
                }

                db[thing].i_flags |= I_MARKED;
            }
        }
    }
}

static void dbunmark1()
{
    char buf[BUF_SIZE];
    dbref loc;

    for (loc = 0; loc < db_top; loc++)
    {
        if (db[loc].i_flags & I_MARKED)
        {
            db[loc].i_flags &=~ I_MARKED;
        }
        else
        {
            if (!IS_GONE(loc))
            {
                if (((Typeof(loc) == TYPE_PLAYER) || (Typeof(loc) == TYPE_THING)))
                {
                    sprintf(buf, "DBCK: Moved object %d", loc);
                    log_error(buf);

                    if (db[loc].location > 0 && db[loc].location < db_top && Typeof(db[loc].location) != TYPE_EXIT)
                    {
                        moveto(loc, db[loc].location);
                    }
                    else
                    {
                        moveto (loc, 0);
                    }
                }
                else if (Typeof(loc) == TYPE_EXIT)
                {
                    log_error(buf);
                    sprintf(buf, "DBCK: moved exit %d", loc);

                    if (db[loc].location > 0 && db[loc].location < db_top && Typeof(db[loc].location) != TYPE_EXIT)
                    {
                        moveto(loc, db[loc].location);
                    }
                    else
                    {
                        moveto(loc, 0);
                    }
                }
            }
        }
    }
}

static void calc_memstats()
{
    char buf[BUF_SIZE];
    int i;
    int j = 0;

    for (i = 0; i < db_top; i++)
    {
        j += mem_usage(i);
    }

    sprintf(buf, "[%s] * There are %d bytes being used in memory, total.", dbinfo_chan, j);
    com_send(dbinfo_chan, buf);
           
    sprintf(buf, "[%s] * The first freed object was #%d.", dbinfo_chan, first_free);
    com_send(dbinfo_chan, buf);
}

void do_dbck(dbref player)
{
    extern dbref speaker;
    dbref i;
    speaker = root;

    for (i = 0; i < db_top; i++)
    {
        int m;
        dbref j;

        for (j = db[i].exits, m = 0; j != NOTHING; j = db[j].next, m++)
        {
            if (m > 1000) db[j].next = NOTHING;
        }

        for (j = db[i].contents, m = 0; j!= NOTHING; j = db[j].next, m++)
        {
            if (m > 1000) db[j].next = NOTHING;
        }
    }

    if (!has_pow(player,NOTHING,POW_DB))
    {
        send_message(player, "@dbck is a restricted command.");
        return;
    }

    fix_free_list();
    dbmark1();
    dbunmark1();
    calc_memstats();
}


// send contents of destroyed object home+destroy exits 
// all objects must be moved to nothing or otherwise unlinked first 
void do_empty(dbref thing)
{
    static int nrecur = 0;
    int i;
    ATRDEF *k, *next;

    if (nrecur++ > 20)
    {
        // if run away recursion return 
        report();
        log_error("Runaway recursion in do_empty");
        nrecur--;
        return;
    }

    while (boot_off(thing));

    if (Typeof(thing) != TYPE_ROOM)
    {
        moveto(thing, NOTHING);
    }

    for (k = db[thing].atrdefs; k; k = next)
    {
        next = k->next;

        if (0 == --k->a.refcount)
        {
            free(k->a.name);
            free(k);
        }
    }

    db[thing].atrdefs = NULL;

    switch(Typeof(thing))
    {
        case TYPE_THING:
        case TYPE_PLAYER:
            moveto (thing, NOTHING);
        case TYPE_ROOM:
            {
                // if room destroy all exits out of it 
                dbref first;
                dbref rest;

                // before we kill it tell people what is happening 
                if (Typeof(thing) == TYPE_ROOM)
                {
                    dest_info(thing, NOTHING);
                }

                // return owners deposit 
                db[thing].zone = NOTHING;
                first = Exits(thing);

                // Clear all exits out of exit list 
                while (first != NOTHING)
                {
                    rest = db[first].next;

                    if (Typeof(first) == TYPE_EXIT)
                    {
                         do_empty(first);
                    }

                    first = rest;
                }

                first = db[thing].contents;

                // send all objects to nowhere 
                DOLIST(rest, first)
                {
                    if (db[rest].link == thing)
                    {
                        db[rest].link = db[db[rest].owner].link;

                        if (db[rest].link == thing)
                        {
                             db[rest].link = 0;
                        }
                    }
                }

                // now send them home 
                while (first != NOTHING)
                {
                    rest = db[first].next;

                    // if home is in thing set it to limbo 
                    moveto (first, HOME);
                    first = rest;
                }
            }
            break;
    }

    // refund owner 
    if (!(db[db[thing].owner].flags & QUIET))
    {
        send_message(db[thing].owner, "You get back your %d credit deposit for %s.", object_cost(thing), unparse_object(db[thing].owner,thing));
    }

    giveto(db[thing].owner, object_cost(thing));
    add_quota(db[thing].owner, 1);

    // chomp chomp 
    atr_free(thing);
    db[thing].list = NULL;

    if (db[thing].pows)
    {
        free(db[thing].pows);
        db[thing].pows = 0;
    }

    // don't eat name otherwise examine will crash 
    s_Pennies(thing, 0);

    db[thing].owner    = root;
    db[thing].flags    = GOING | TYPE_THING; // toad it 
    db[thing].location = NOTHING;
    db[thing].link     = NOTHING;

    for (i = 0; db[thing].children && db[thing].children[i] != NOTHING; i++)
    {
        REMOVE_FIRST_L(db[db[thing].children[i]].parents, thing);
    }

    if (db[thing].children)
    {
        free(db[thing].children);
    }

    db[thing].children = NULL;

    for (i = 0; db[thing].parents && db[thing].parents[i] != NOTHING; i++)
    {
        REMOVE_FIRST_L(db[db[thing].parents[i]].children, thing);
    }

    if (db[thing].parents)
    {
        free(db[thing].parents);
    }

    db[thing].parents = NULL;
    do_halt(thing,"");
    free_object(thing);
    nrecur--;
}

void do_undestroy(dbref player, char *arg1)
{
    dbref object;

    object = match_controlled(player, arg1, POW_EXAMINE);

    if (object == NOTHING)
    {
        return;
    }

    if (!(db[object].flags & GOING))
    {
        send_message(player, "%s is not scheduled for destruction", unparse_object(player, object));
        return;
    }

    db[object].flags &= ~GOING;

    if (atol(atr_get(object, A_DOOMSDAY)) > 0)
    {
        atr_add(object, A_DOOMSDAY, "");
        send_message(player, "%s has been saved from destruction.", unparse_object(player, object));
    }
    else
    {
        send_message(player, "%s is protected, and the GOING flag shouldn't have been set in the first place so what on earth happened?", unparse_object(player, object));
    }
}

void zero_free_list()
{
  first_free = NOTHING;
}

void do_check(dbref player, char *arg1)
{
    dbref obj;

    if (!power(player,POW_SECURITY))
    {
        send_message(player, perm_denied());
        return;
    }

    obj = match_controlled (player, arg1, POW_MODIFY);

    if (obj == NOTHING)
    {
        return;
    }

    thing = obj;
    gstate = 1;
    send_message(player, "Okay, i set the garbage point.");
}

void info_db(dbref player)
{
    send_message(player, "db_top: #%d", db_top);
    send_message(player, "first_free: #%d", first_free);
    send_message(player, "update_bytes_counter: #%d", update_bytes_counter);
    send_message(player, "garbage point: #%d", thing);

    do_stats(player, "");
}

// garbage collect the database 
void do_incremental()
{
    char buf[BUF_SIZE];
    int j;
    int a;

    switch(gstate)
    {
        case 0:
            // pre collection need to age things first 
            //fprintf(stderr,"Ageing\n");
            //(fprintf(stderr,"agedone\n");
            gstate = 1;
            thing  = 0;
            break;
        case 1: // into the copying stage 
            o = &(db[thing]);

            for (a = 0; (a < garbage_chunk) && (thing < db_top); a++, o++, thing++)
            {
                char buff[BUF_SIZE];
                extern char ccom[];
                int i;

                sprintf(ccom, "object #%d\n", thing);

                if (thing == db_top)
                {
                  	gstate = 0;
                  	break;
                }

                strcpy(buff,o->name);
                SET(o->name,buff);

                atr_collect(thing);

                if (!IS_GONE(thing))
                {
                    ALIST *atr, *nxt;

                    again1:

                    for (i = 0; db[thing].parents && db[thing].parents[i] != NOTHING; i++)
                    {
                        CHECK_REF(db[thing].parents[i])
                        {
                            sprintf(buf, "Bad #%d in parent list on #%d.",db[thing].parents[i], thing);
	                    log_error(buf);
                            REMOVE_FIRST_L (db[thing].parents, db[thing].parents[i]);
	                    goto again1;
	                }

                        for (j = 0; db[db[thing].parents[i]].children && db[db[thing].parents[i]].children[j] != NOTHING; j++)
                        {
	                    if (db[db[thing].parents[i]].children[j] == thing)
                            {
                                j = NOTHING;
                                break;
                            }
                        }

                        if (j != NOTHING)
                        {
                            sprintf(buf, "Wrong #%d in parent list on #%d.",db[thing].parents[i], thing);
                            log_error(buf);
                            REMOVE_FIRST_L (db[thing].parents, db[thing].parents[i]);
                            goto again1;
                        }
                    }

                    again2:

                    for (i = 0; db[thing].children && db[thing].children[i] != NOTHING; i++)
                    {
                        CHECK_REF(db[thing].children[i])
                        {
                            sprintf(buf, "Bad #%d in children list on #%d.", db[thing].children[i], thing);
                            log_error(buf);
                            REMOVE_FIRST_L (db[thing].children, db[thing].children[i]);
                            goto again2; // bad programming style, but it's easiest. 
                        }

                        for (j = 0; db[db[thing].children[i]].parents && db[db[thing].children[i]].parents[j] != NOTHING; j++)
                        {
                            if (db[db[thing].children[i]].parents[j] == thing)
                            {
                                j = NOTHING;
                                break;
                            }
                        }

                        if (j != NOTHING)
                        {
                            sprintf(buf, "Wrong #%d in children list on #%d.",db[thing].children[i], thing);
                            log_error(buf);
                            REMOVE_FIRST_L (db[thing].children, db[thing].children[i]);
                            goto again1;
                        }
                    }

                    for (atr = db[thing].list; atr; atr = nxt)
                    {
                        nxt = AL_NEXT(atr);
                        if (AL_TYPE(atr) && AL_TYPE(atr)->obj != NOTHING && !is_a(thing,AL_TYPE(atr)->obj))
                        atr_add (thing, AL_TYPE(atr), "");
                    }
                    {
                        dbref zon;

                        for (dozonetemp = 0, zon = get_zone_first(thing); zon != NOTHING; zon = get_zone_next(zon), dozonetemp++)
                        {
                            if (dozonetemp > 15)
                            {
                                // ack. inf loop. 
                                sprintf(buf, "%s's zone %s is infinite.", unparse_object_a(1, thing), unparse_object_a(1, zon));
                                log_error(buf);
                                db[zon].zone        = db[0].zone;
                                db[db[0].zone].zone = NOTHING;
                            }
                        }
                    }

                    CHECK_REF(db[thing].exits)

                    switch(Typeof(thing))
                    {
                        case TYPE_PLAYER:
                        case TYPE_THING:
                        case TYPE_ROOM:
                        {
                            // yuck probably corrupted set to nothing 
                            sprintf(buf, "Dead exit in exit list (first) for room #%d: %d", thing, db[thing].exits);
                            log_error(buf);
                            report();
                            db[thing].exits = NOTHING;
                        }
                    }

                    CHECK_REF(db[thing].zone)

                    switch(Typeof(thing))
                    {
                        case TYPE_ROOM:
                            sprintf(buf, "Zone for #%d is #%d! setting it to the global zone.", thing, db[thing].zone);
                            log_error(buf);
                            db[thing].zone = db[0].zone;
                            break;
                    }

                    CHECK_REF(db[thing].link)

                    switch(Typeof(thing))
                    {
                        case TYPE_PLAYER:
                        case TYPE_THING:
                            db[thing].link = player_start;
                            break;
                        case TYPE_EXIT:
                        case TYPE_ROOM:
                            db[thing].link = NOTHING;
                            break;
                    }

                    CHECK_REF(db[thing].location)

                    switch(Typeof(thing))
                    {
                        case TYPE_PLAYER: // this case shouldn't happen but just incase 
                        case TYPE_THING:
                            db[thing].location = NOTHING;
                            moveto(thing,player_start);
                            break;
                        case TYPE_EXIT:
                            db[thing].location = NOTHING;
                            destroy_obj(thing, atoi(bad_object_doomsday));
                            break;
                        case TYPE_ROOM:
                            // rooms are in themselves 
                            db[thing].location = thing;
                            break;
                    }

                    if (((db[thing].next < 0) || (db[thing].next >= db_top)) && (db[thing].next != NOTHING))
                    {
                        sprintf(buf, "Invalid next pointer from object %s(%d)", db[thing].name, thing);
                        log_error(buf);
	   
                        report();
                        db[thing].next = NOTHING;
                    }

                    if ((db[thing].owner < 0) || (db[thing].owner >= db_top) || Typeof(db[thing].owner) != TYPE_PLAYER)
                    {
                        sprintf(buf, "Invalid object owner %s(%d): %d", db[thing].name, thing, db[thing].owner);
                        log_error(buf);
                        report();
                        db[thing].owner = root;
                    }
                }

                if (!*atr_get(o->owner, A_BYTESUSED))
                {
                    recalc_bytes(o->owner);
                }
            }

            // if complete go to state 0 
            if (thing == db_top)
            {
                gstate = 0;
            }

            break;
    }
}

//  Find and mark all floating rooms. 
static void mark_float()
{
    dbref loc;

    for (loc = 0; loc < db_top; loc++)
    {
        if (IS(loc, TYPE_ROOM, ROOM_FLOATING))
        {
            dbmark(loc);
        }
    }
}

// yay! workie! 
void do_upfront(dbref player, char *arg1)
{
    dbref object;
    dbref thing;

    if(!power(player,POW_DB))
    {
        send_message(player, "Restricted command.");
        return;
    }

    if ((thing = match_thing(player, arg1)) == NOTHING)
    {
        return;
    }

    if (first_free == thing)
    {
        send_message(player, "That object is already at the top of the free list.");
        return;
    }

    for (object = first_free; object != NOTHING && db[object].next != thing; object = db[object].next)
    ;

    if (object == NOTHING)
    {
        send_message(player, "That object does not exist in the free list.");
        return;
    }

    db[object].next = db[thing].next;
    db[thing].next  = first_free;
    first_free = thing;
    send_message(player, "Object is now at the front of the free list.");
}
