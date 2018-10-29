// speech.c 
// $Id: speech.c,v 1.21 1993/09/18 19:03:46 nils Exp $ 
// Commands which involve speaking 

#include <ctype.h>
#include <sys/types.h>

#include "tinymuse.h"
#include "db.h"
#include "interface.h"
#include "net.h"
#include "match.h"
#include "config.h"
#include "externs.h"

static int can_emit_msg P((dbref, dbref, char *));

///////////////////////////////////////////////////////////////////////////////
// spname()
///////////////////////////////////////////////////////////////////////////////
// generate name for object to be used when spoken 
///////////////////////////////////////////////////////////////////////////////
// OPTIONAL
//     For spoof protection:
//           - player names always capitalized
//           - object names always lower case
///////////////////////////////////////////////////////////////////////////////
char *spname(dbref thing)
{
    return db[thing].name;
}

// this function is a kludge for regenerating messages split by '=' 
char *reconstruct_message(char *arg1, char *arg2)
{
    static char buf[MAX_CMD_BUF];

    if ( arg2 )
    {
        if ( *arg2 )
        {
            strcpy(buf, arg1?arg1:"");
            strcat(buf, " = ");
            strcat(buf, arg2);
            return buf;
        }
    }

    return arg1 ? arg1 : "";
}

void do_say(dbref player, char *arg1, char *arg2)
{
    char buf[MAX_CMD_BUF];
    dbref loc;
    char *message;
    char *bf;
    char buf2[BUF_SIZE];

    if ((loc = getloc(player)) == NOTHING)
    {
        return;
    }

    if (IS(loc,TYPE_ROOM,ROOM_AUDITORIUM) && (!could_doit(player, loc, A_SLOCK) || !could_doit(player, db[loc].zone, A_SLOCK)))
    {
        did_it(player, loc, A_SFAIL, "Shh.", A_OSFAIL, NULL, A_ASFAIL);
        return;
    }

    message = reconstruct_message(arg1, arg2);
    pronoun_substitute(buf, player, message, player);
    bf = buf+strlen(db[player].name)+1;
    // notify everybody
    send_message(player, "You say \"%s|NORMAL|\"", bf);
    sprintf(buf2, "%s says \"%s|NORMAL|\"", spname(player), bf);
    notify_in(loc, player, buf2);
}

void do_whisper(dbref player, char *arg1, char *arg2)
{
    dbref who;
    char buf[MAX_CMD_BUF],*bf;

    pronoun_substitute(buf, player, arg2, player);
    bf = buf + strlen(db[player].name) + 1;
    init_match(player, arg1, TYPE_PLAYER);
    match_neighbor();
    match_me();

    if (power(player, POW_REMOTE))
    {
        match_absolute();
        match_player();
    }

    switch(who = match_result())
    {
        case NOTHING:
            send_message(player, "Whisper to whom?");
            break;
        case AMBIGUOUS:
            send_message(player, "I don't know who you mean!");
            break;
        default:
            if ( *bf == ':' )
            {
                send_message(player, "You whisper-posed %s with \"%s %s\".", db[who].name, spname(player), bf+1);
                send_message(who, "%s whisper-poses: %s %s", spname(player), spname(player), bf+1);
                // wptr[0]=bf;   // Don't pass %0 
                did_it(player, who, NULL, 0, NULL, 0, A_AWHISPER);
                break;
            }

            send_message(player, "You whisper \"%s\" to %s.", bf, db[who].name);
            send_message(who, "%s whispers \"%s\"", spname(player), bf);
      
            // wptr[0]=bf;   // Don't pass %0 
            did_it(player, who, NULL, 0, NULL, 0, A_AWHISPER);
            break;
    }
}

void do_pose(dbref player, char *arg1, char *arg2, int possessive)
{
    dbref loc;
    char lastchar, *format;
    char *message;
    char buf[MAX_CMD_BUF];
    char buf2[BUF_SIZE*2];
    char *bf;

    if ((loc = getloc(player)) == NOTHING)
    {
        return;
    }

    if (IS(loc,TYPE_ROOM,ROOM_AUDITORIUM) && (!could_doit(player, loc, A_SLOCK) || !could_doit(player, db[loc].zone, A_SLOCK)))
    {
        did_it(player, loc, A_SFAIL, "Shh.", A_OSFAIL, NULL, A_ASFAIL);
        return;
    }

    if ( possessive )
    {
        // get last character of player's name
        lastchar = to_lower(db[player].name[strlen(db[player].name)-1]);
        format = (lastchar == 's') ? "%s' %s" : "%s's %s";
    }
    else
    {
        format = "%s %s";
    }

    message = reconstruct_message(arg1, arg2);
    pronoun_substitute(buf, player, message, player);
    bf = strlen(db[player].name)+buf+1;

    // notify everybody
    sprintf(buf2, format, spname(player), bf);
    notify_in(loc, NOTHING, buf2);
       
}

void do_echo(dbref player, char *arg1, char *arg2, int type)
{
    char *message = reconstruct_message(arg1, arg2);
    char buf[BUF_SIZE];

    if (type == 0)
    {
        pronoun_substitute(buf, player, message, player);
        message = buf+strlen(db[player].name) +1;
    }

    send_message(player, message);
}

void do_emit(dbref player, char *arg1, char *arg2, int type)
{
    char buf[MAX_CMD_BUF];
    char buf2[BUF_SIZE];
    char *message;
    char *bf = buf;
    dbref loc;

    if ((loc = getloc(player)) == NOTHING)
    {
        return;
    }

    if (IS(loc, TYPE_ROOM, ROOM_AUDITORIUM) && !controls(player,loc,POW_SPOOF) && (!could_doit(player, loc, A_SLOCK) || !could_doit(player, db[loc].zone, A_SLOCK)))
    {
        did_it(player, loc, A_SFAIL, "Shh.", A_OSFAIL, NULL, A_ASFAIL);
        return;
    }

    message = reconstruct_message(arg1,arg2);

    if ( type == FALSE )
    {
        pronoun_substitute(buf, player, message, player);
        bf = buf + strlen(db[player].name) + 1;
    }

    if ( type == TRUE)
    {
        strcpy(buf, message);
        bf = buf;
    }

    if (power(player, POW_SPOOF))
    {
        sprintf(buf2, "%s",bf);
        notify_in(loc, NOTHING, buf2);
        return;
    }

    if (can_emit_msg(player, db[player].location, bf))
    {
        sprintf(buf2, "%s", bf);
        notify_in(loc, NOTHING, buf2);
    }
    else
    {
        send_message(player, perm_denied());
    }
}

static void notify_in_zone P((dbref, char *));

static void notify_in_zone (dbref zone, char *msg)
{
    dbref thing;
    static int depth = 0;

    if (depth > 10)
    {
        return;
    }

    depth++;

    for (thing = 0; thing < db_top; thing++)
    {
        if (db[thing].zone == zone)
        {
            notify_in_zone(thing, msg);
            notify_in(thing, NOTHING, msg);
        }
    }

    depth--;
}

void do_general_emit(dbref player, char *arg1, char *arg2, int emittype)
{
    dbref who;
    char buf[MAX_CMD_BUF];
    char *bf = buf;

    if ( emittype != 4 )
    {
        pronoun_substitute(buf, player, arg2, player);
        bf = buf + strlen(db[player].name) + 1;
    }
    if ( emittype == 4 )
    {
        bf = arg2;
        while ( *bf && !(bf[0]=='=') )
        {
            bf++;
        }
        if (*bf)
        {
            bf++;
        }

        emittype = 0;
    }

    init_match(player, arg1, TYPE_PLAYER);
    match_absolute();
    match_player();
    match_neighbor();
    match_possession();
    match_me();
    match_here();

    who = noisy_match_result();

    if ( get_room(who) != get_room(player)
     && !controls(player, get_room(who), POW_REMOTE)
     && !controls_a_zone(player, who, POW_REMOTE) )
    {
        send_message(player, perm_denied());
        return;
    }

    if ( IS(db[who].location, TYPE_ROOM, ROOM_AUDITORIUM)
     && !controls(player,db[who].location, POW_SPOOF)
     && (!could_doit(player, db[who].location, A_SLOCK) || !could_doit(player, db[who].zone, A_SLOCK))
       )
    {
        did_it(player, db[who].location, A_SFAIL, "Shh.", A_OSFAIL, NULL, A_ASFAIL);
        return;
    }

    switch( who )
    {
        case NOTHING:
            break;
        default:
            if (emittype == 0)    // pemit 
            {
                if (can_emit_msg(player, db[who].location, bf) || controls (player, who, POW_SPOOF))
                {
                    send_message(who, bf);
                    // wptr[0]=bf;  // Do not pass %0 
                    did_it(player, who, NULL, 0, NULL, 0, A_APEMIT);
                    if (!(db[player].flags & QUIET))
                    {
                        send_message(player, "%s just saw \"%s\".", unparse_object(player, who), bf);
                    }
                }
                else
                {
                    send_message(player, perm_denied());
                }
            }
            else if (emittype == 1)
            {
                // room. 
                if ( ( (controls(player, who, POW_REMOTE) && controls(player, who, POW_SPOOF))
                     || db[player].location == who)
                  && (can_emit_msg(player, who, bf))
                   )
                {
                    notify_in(who, NOTHING, bf);
                    if (!(db[player].flags & QUIET))
                    {
                        send_message(player, "Everything in %s saw \"%s\".", unparse_object(player, who), bf);
                    }
                }
                else
                {
                    send_message(player, perm_denied());
                    return;
                }
            }
            else if (emittype == 2)
            {
                // oemit 
                if (can_emit_msg(player, db[who].location, bf))
                {
                    notify_in(db[who].location, who, bf);
                }
                else
                {
                    send_message(player, perm_denied());
                    return;
                }
            }
            else if (emittype == 3) // zone 
            {
                if (controls(player, who, POW_REMOTE) && controls(player, who, POW_SPOOF) && controls(player, who, POW_MODIFY) && can_emit_msg(player, (dbref) -1, bf))
                {
                    if (db[who].zone == NOTHING && !(db[player].flags & QUIET))
                    {
                        send_message(player, "%s might not be a zone... but i'll do it anyways", unparse_object(player, who));
                    }

                    notify_in_zone (who, bf);

                    if (!(db[player].flags & QUIET))
                    {
                        send_message(player, "Everything in zone %s saw \"%s\".", unparse_object(player, who), bf);
                    }
                }
                else
                {
                    send_message(player, perm_denied());
                }
            }
    } // switch
}

static int can_emit_msg(dbref player, dbref loc, char *msg)
{
    char mybuf[BUF_SIZE];
    char *s;
    dbref thing, save_loc;

    for (; *msg == ' '; msg++)
    ;

    strcpy(mybuf, msg);

    for (s = mybuf; *s && (*s != ' '); s++)
    ;

    if (*s)
    {
        *s = '\0';
    }

    if ((thing = lookup_player(mybuf)) != NOTHING && !string_compare(db[thing].name, mybuf))
    {
        if (!controls(player, thing, POW_SPOOF))
        {
            return FALSE;
        }
    }

    if ((s-mybuf)>2 && !strcmp(s-2,"'s"))
    {
        *(s-2) = '\0';

        if ((thing = lookup_player(mybuf)) != NOTHING && !string_compare(db[thing].name, mybuf))
        {
            if (!controls(player, thing, POW_SPOOF))
            {
                return FALSE;
            }
        }
    }

    // yes, get ready, another awful kludge 
    save_loc = db[player].location;
    db[player].location = loc;
    init_match (player, mybuf, NOTYPE);
    match_perfect();
    db[player].location = save_loc;
    thing = match_result();

    if (thing != NOTHING && !controls(player, thing, POW_SPOOF))
    {
        return FALSE;
    }

    return TRUE;
}

void do_announce(dbref player, char *arg1, char *arg2)
{
    char *message;
    char buf[BUF_SIZE*2];
    struct descriptor_data *d;

    if ( Guest(player) || (Typeof(player)!=TYPE_PLAYER && !power(player,POW_SPOOF)))
    {
        send_message(player, "You can't do that.");
        return;
    }
    else
    {
        if ( db[db[player].owner].flags & PLAYER_NO_ANN )
        {
            send_message(player, "You aren't allowed to @announce.");
            message = reconstruct_message(arg1, arg2);
            sprintf(buf, "%s(#%d) [owner=%s(#%d)] (NO_ANN) attempted to @announce %s", db[player].name, player, db[db[player].owner].name, db[player].owner, message);
            log_io(buf);

            return;
        }
    }

    message = reconstruct_message(arg1, arg2);

    if ( power(player, POW_ANNOUNCE) || payfor(player, announce_cost))
    {
        if (*arg1 == ';')
        {
            sprintf(buf, "From across the MUSE, %s's %s", spname(player), arg1+1);
        }
        else if (*arg1 == ':')
        {
            sprintf(buf, "From across the MUSE, %s %s", spname(player), arg1+1);
        }
        else
        {
            if ( power(player, POW_ANNOUNCE))
            {
                sprintf(buf, "%s announces \"%s\"", spname(player), message);
            }
            else
            {
                sprintf(buf, "%s announces \"%s\"", unparse_object(player, player), message);
            }
        }
    }
    else
    {
        send_message(player, "Sorry, you don't have any credits.");
        return;
    }

    sprintf(buf, "%s(#%d) [owner=%s(#%d)] executes: @announce %s", db[player].name, player, db[db[player].owner].name, db[player].owner, message);
    log_io(buf);

    strcat(buf,"\n");

    for (d = descriptor_list; d; d = d->next)
    {
        if ( d->state != CONNECTED)
            continue;
        if ( db[d->player].flags & PLAYER_NO_WALLS )
            continue;
        if (Typeof(d->player) != TYPE_PLAYER)
            continue;

        queue_string(d, buf);
    }
}

void do_broadcast(dbref player, char *arg1, char *arg2)
{
    char *message;
    struct descriptor_data *d;
    char buf[BUF_SIZE*2];

    if ( ! power(player, POW_BROADCAST) )
    {
        send_message(player, "You don't have the authority to do that.");
        return;
    }

    message = reconstruct_message(arg1, arg2);
    sprintf(buf, "Official broadcast from %s: \"%s\"\n", db[player].name, message);

    sprintf(buf, "%s(#%d) executes: @broadcast %s", db[player].name, player, message);
    log_important(buf);
           
    for (d = descriptor_list; d; d = d->next)
    {
        if ( d->state != CONNECTED )
        {
            continue;
        }

        queue_string(d, buf);
    }
}

void do_gripe(dbref player, char *arg1, char *arg2)
{
    char buf[BUF_SIZE];
    dbref loc;
    char *message;

    loc = db[player].location;
    message = reconstruct_message(arg1, arg2);
    sprintf(buf, "GRIPE from %s(%d) in %s(%d): %s", db[player].name, player, db[loc].name, loc, message);
    log_gripe(buf);

    send_message(player, "Your complaint has been duly noted.");
}

static char *title(dbref player)
{
    static char buf[BUF_SIZE*2];

    if (!*atr_get(player,A_ALIAS))
    {
        sprintf(buf, "%s (#%d)", db[player].name, player);
    }
    else
    {
        sprintf(buf, "%s (%s)", db[player].name, atr_get(player,A_ALIAS));
    }

    return buf;
}

void do_page(dbref player, char *arg1, char *arg2)
{
    dbref *x;
    int k;

    x = lookup_players(player, arg1);

    if (x[0] == 0)
    {
        return;
    }

    if (!payfor(player, page_cost*(x[0])))
    {
        send_message(player,"You don't have enough Credits.");
        return;
    }

    for (k = 1; k <= x[0]; k++)
    {
        if ((db[x[k]].owner == x[k]) ?(!(db[x[k]].flags & CONNECT)) : !(*atr_get(x[k],A_APAGE) || Hearer(x[k])))
        {
            send_message(player, "%s is either hidden from you or not connected.", db[x[k]].name);

            if (*Away(x[k]))
            {
                send_message(player, "Away message from %s: %s", spname(x[k]), atr_get(x[k],A_AWAY));
            }
        }
        else if (!could_doit(player,x[k],A_LHIDE) && !has_pow(player, x[k], POW_WHO))
        {
            send_message(player, "%s is either hidden from you or not connected.", db[x[k]].name);

            if (*Away(x[k]))
            {
                send_message(player, "Away message from %s: %s", spname(x[k]), atr_get(x[k],A_AWAY));
            }
            if (could_doit(player,x[k],A_LPAGE))
            {
                if (!*arg2)
                {
                    send_message(x[k], "You sense that %s is looking for you in %s", title(player), unparse_object(x[k], db[player].location));
                }
                else if ( *arg2 == ':' )
                {
                    send_message(x[k], "(HIDDEN) %s page-poses: %s %s", title(player), spname(player),arg2+1);
                    if (db[x[k]].owner != x[k])
                    {
                        wptr[0] = arg2;
                    }
                }
                else if ( *arg2 == ';' )
                {
                    send_message(x[k], "(HIDDEN) %s page-poses: %s's %s", title(player), spname(player),arg2+1);
           

                    if (db[x[k]].owner != x[k])
                    {
                        wptr[0] = arg2;
                    }
                }
                else
                {
                    send_message(x[k], "(HIDDEN) %s pages: %s", title(player), arg2);
 
                    if (db[x[k]].owner != x[k])
                    {
                        wptr[0] = arg2;
                    }
                }
            }
        }
        else if (!could_doit(player, x[k], A_LPAGE))
        {
            send_message(player, "%s is not accepting pages.", spname(x[k]));
        
            if (*atr_get(x[k],A_HAVEN))
            {
                send_message(player, "Haven message from %s: %s", spname(x[k]), atr_get(x[k], A_HAVEN));
            }
        }
        else
        {
            if (!*arg2)
            {
                send_message(x[k], "You sense that %s is looking for you in %s", spname(player), db[db[player].location].name);
                send_message(player, "You notified %s of your location.", spname(x[k]));
                //    wptr[0]=arg2;  // Do not pass %0
                did_it(player, x[k], NULL, 0, NULL, 0, A_APAGE);
            }
            else if ( *arg2 == ':' )
            {
                send_message(x[k], "%s page-poses: %s %s",title(player), spname(player), arg2+1);
                send_message(player, "You page-posed %s with \"%s %s\".", db[x[k]].name, spname(player), arg2+1);
                if (db[x[k]].owner != x[k])
                {
                    wptr[0] = arg2;
                }
                did_it(player, x[k], NULL, 0, NULL, 0, A_APAGE);
            }
            else if ( *arg2 == ';' )
            {
                send_message(x[k], "%s page-poses: %s's %s",title(player), spname(player), arg2+1);
                send_message(player, "You page-posed %s with \"%s's %s\".", db[x[k]].name, spname(player), arg2+1);

                if (db[x[k]].owner != x[k])
                {
                    wptr[0] = arg2;
                }

                did_it(player, x[k], NULL, 0, NULL, 0, A_APAGE);
            }
            else
            {
                send_message(x[k], "%s pages: %s", title(player), arg2);
                send_message(player, "You paged %s with \"%s\".", spname(x[k]),arg2);
                  
                if (db[x[k]].owner != x[k])
                {
                    wptr[0] = arg2;
                }

                did_it(player, x[k], NULL, 0, NULL, 0, A_APAGE);
            }

            if (*Idle(x[k]))
            {
                send_message(player, "Idle message from %s: %s", spname(x[k]), atr_get(x[k],A_IDLE));
                send_message(x[k], "Your Idle message has been sent to %s.", spname(player));
            }
        }
    }
}


void do_use(dbref player, char *arg1)
{
    dbref thing;
    thing = match_thing(player,arg1);

    if (thing == NOTHING)
    {
        return;
    }

    did_it(player, thing, A_USE, "You don't know how to use that.", A_OUSE, NULL, A_AUSE);
}

void do_cemit(dbref player, char *arg1, char *arg2)
{
    char buf[BUF_SIZE];

    if (!power(player,POW_SPOOF))
    {
        send_message(player, "Permission denied.");
        return;
    }

    sprintf("[%s] %s", arg1, arg2);
    com_send(arg1, buf);
}

void do_wemit(dbref player, char *arg1, char *arg2)
{
    struct descriptor_data *d;
    char *message;
    char buf[BUF_SIZE*2];

    message = reconstruct_message(arg1, arg2);

    if (!power(player,POW_BROADCAST))
    {
        send_message(player, "Permission denied.");
        return;
    }

    sprintf(buf, "%s\n", message);

    for (d = descriptor_list; d; d = d->next)
    {
        if ( d->state != CONNECTED)
        {
            continue;
        }
        queue_string(d, buf);
    }
}

void do_cpage(dbref player, char *arg1, char *arg2)
{
    struct descriptor_data *d, *des = NULL;
    int concid = atoi(arg1);
    char buf[BUF_SIZE*4];

    if (!power(player, POW_BROADCAST))
    {
        send_message(player, "Permission denied.");
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (!(d->cstatus & C_REMOTE) && (d->concid) && (d->concid == concid))
        {
            des = d;
            break;
        }
    }

    if (!des)
    {
        send_message(player, "Invalid concid.");
        return;
    }

    sprintf(buf, "Concid message from %s:  %s\n", db[player].name, arg2);
    queue_string(des, buf);

    send_message(player, "You sent the message '%s' to concid %d (des %d).", arg2, des->concid, des->descriptor);
        
}

void do_to(dbref player, char *arg1, char *arg3)
{
    char buf[MAX_CMD_BUF];
    char buf2[MAX_BUF];
    char *bf;
    dbref loc;
    char *message;
    extern char *str_index();
    dbref to;
    char *arg2;

    // parse first argument into two 
    arg2 = str_index(arg1, ' ');

    if ( arg2 != NULL )
    {
        *arg2++ = '\0';
    }
    else
    {
        arg2 = "";
    }

    if ((loc = getloc(player)) == NOTHING)
    {
        return;
    }

    if (IS(loc,TYPE_ROOM,ROOM_AUDITORIUM) && (!could_doit(player, loc, A_SLOCK) || !could_doit(player, db[loc].zone, A_SLOCK)))
    {
        did_it(player, loc, A_SFAIL, "Shh.", A_OSFAIL, NULL, A_ASFAIL);
        return;
    }

    init_match(player, arg1, TYPE_PLAYER);
    match_neighbor();
    match_me();

    if (power(player, POW_REMOTE))
    {
        match_absolute();
        match_player();
    }

    switch(to = match_result())
    {
        case NOTHING:
            send_message(player, "I don't see that person here.");
            break;
        case AMBIGUOUS:
            send_message(player, "I don't know who you mean!");
            break;
        default:
            message = reconstruct_message(arg2, arg3);
            pronoun_substitute(buf, player, message, player);
            bf=buf+strlen(db[player].name)+1;

            // notify everybody 
            send_message(player, "%s [to %s]: %s|NORMAL|", spname(player), db[to].name, bf);
            sprintf(buf2, "%s [to %s]: %s|NORMAL|", spname(player), db[to].name, bf);
            notify_in(loc, player, buf2);
    }
}
