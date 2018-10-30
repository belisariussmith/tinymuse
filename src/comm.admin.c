// Wizard-only commands
// $Id: admin.c,v 1.11 1993/09/18 19:03:38 nils Exp $

#include "tinymuse.h"
#include "db.h"
#include "config.h"

#include "interface.h"
#include "match.h"
#include "externs.h"
#include "credits.h"

#include "admin.h"
#include "player.h"

#include <ctype.h>
#include <crypt.h>

#define  ANY_OWNER	-2

// local function declarations 
static object_flag_type convert_flags P((dbref player, int is_wizard, char *s, object_flag_type *, object_flag_type *));

// added 12/1/90 by jstanley to add @search command details in file game.c 
// Ansi: void do_search(dbref player, char *arg1, char *arg3); 
void do_search(dbref player, char *arg1, char *arg3)
{
    int flag;
    int restrict_zone;

    dbref thing;
    dbref from;
    dbref to;
    dbref restrict_owner;
    dbref restrict_link;

    object_flag_type flag_mask;
    object_flag_type restrict_type;
    object_flag_type restrict_class;

    char        *arg2;
    char        *restrict_name;
    extern char *str_index();

    char buf[3100];

    int destitute = TRUE;     // destitute by default

    // parse first argument into two 
    arg2 = str_index(arg1, ' ');

    if ( arg2 != NULL )
    {
        *arg2++ = '\0';		// arg1, arg2, arg3 
    }
    else
    {
        if ( *arg3 == '\0' )
        {
            arg2 = "";		// arg1 
        }
        else
        {
            arg2 = arg1;	// arg2, arg3 
            arg1 = "";
        }
    }

    // set limits on who we search 
    restrict_owner = NOTHING;

    if ( *arg1 == '\0' )
    {
        restrict_owner = power(player,POW_EXAMINE) ? ANY_OWNER : player;
    }
    else if ( arg1[0] == '#' )
    {
        restrict_owner = atoi(&arg1[1]);

        if ( restrict_owner < 0 || db_top <= restrict_owner )
        {
            restrict_owner = NOTHING;
        }
        else if ( Typeof(restrict_owner) != TYPE_PLAYER )
        {
            restrict_owner = NOTHING;
        }
    }
    else if ( strcmp(arg1, "me") == 0 )
    {
        restrict_owner = player;
    }
    else
    {
        restrict_owner = lookup_player(arg1);
    }

    if ( restrict_owner == NOTHING )
    {
        send_message(player, "%s: No such player", arg1);
        return;
    }

    // set limits on what we search for 
    flag = 0;
    flag_mask = 0;
    restrict_name = NULL;
    restrict_type = NOTYPE;
    restrict_link = NOTHING;
    restrict_zone = NOTHING;
    restrict_class = 0;

    switch (arg2[0])
    {
        case '\0':
            // the no class requested class  :) 
            break;
        case 'c':
            if ( string_prefix("class", arg2))
            {
                restrict_class=name_to_class(arg3);
                if (!restrict_class || !power(player,POW_WHO))
                {
                    send_message(player, "unknown class!");
                    return;
                }
                restrict_type=TYPE_PLAYER;
            }
            else
            {
                flag = FALSE;
            }
            //    if(restrict_class==TYPE_DIRECTOR && !power(player,TYPE_ADMIN))
            //    restrict_class=TYPE_ADMIN;
            //    if(restrict_class==TYPE_JUNIOR && !power(player,TYPE_ADMIN))
            //    restrict_class=TYPE_OFFICIAL;
            break;
        case 'e':
            if ( string_prefix("exits", arg2) )
            {
                restrict_name = arg3;
                restrict_type = TYPE_EXIT;
            }
            else
            {
                flag = FALSE;
            }
            break;
        case 'f':
            if ( string_prefix("flags", arg2) )
            {
                // convert_flags ignores previous values of flag_mask and restrict_type while setting them.
                if ( ! convert_flags(player, power(player,POW_EXAMINE), arg3, // power?XXX 
                &flag_mask, &restrict_type) )
                {
                    return;
                }
            }
            else
            {
                flag = FALSE;
            }
            break;
        case 'l':
            if (string_prefix("link", arg2))
            {
                if ((restrict_link = match_thing(player, arg3)) == NOTHING)
                {
	            flag = TRUE;
                }
            }
            else
            {
                flag = TRUE;
            }
            break;
        case 'n':
            if ( string_prefix("name", arg2) )
            {
                restrict_name = arg3;
            }
            else
            {
                flag = FALSE;
            }
            break;
        case 'o':
            if ( string_prefix("objects", arg2) )
            {
                restrict_name = arg3;
                restrict_type = TYPE_THING;
            }
            else
            {
                flag = FALSE;
            }
            break;
        case 'p':
            if ( string_prefix("players", arg2) )
            {
                restrict_name = arg3;

                if ( *arg1 == '\0' )
                {
                    restrict_owner = ANY_OWNER;
                }
 
                restrict_type = TYPE_PLAYER;
            }
            else
            {
                flag = FALSE;
            }
            break;
        case 'r':
            if ( string_prefix("rooms", arg2) )
            {
                restrict_name = arg3;
                restrict_type = TYPE_ROOM;
            }
            else
            {
                flag = FALSE;
            }
            break;
        case 't':
            if ( string_prefix("type", arg2) )
            {
                if ( arg3[0] == '\0' )
                {
                    break;
                }

                if ( string_prefix("room", arg3) )
                {
                    restrict_type = TYPE_ROOM;
                }
                else if ( string_prefix("exit", arg3) )
                {
                    restrict_type = TYPE_EXIT;
                }
                else if ( string_prefix("thing", arg3) )
                {
                    restrict_type = TYPE_THING;
                }
                else if ( string_prefix("player", arg3) )
                {
                    if ( *arg1 == '\0' )
                    {
                        restrict_owner = ANY_OWNER;
                    }

                    restrict_type = TYPE_PLAYER;
                }
                else
                {
                    send_message(player, "%s: unknown type", arg3);
                    return;
                }
            }
            else
            {
                flag = FALSE;
            }
            break;
        case 'z':
            if (string_prefix ("zone", arg2))
            {
                if ((restrict_zone = match_thing(player, arg3)) == NOTHING)
                {
                    flag = FALSE;
                }
                else
                {
                    restrict_type = TYPE_ROOM;
                }
            }
            else
            {
                flag = TRUE;
            }
            break;
        default:
            flag = FALSE;
    }

    if ( flag )
    {
        send_message(player, "%s: unknown class", arg2);
        return;
    }

    if (restrict_owner != ANY_OWNER)
    {
        if (!controls (player, restrict_owner, POW_EXAMINE))
        {
            send_message(player, "You need a search warrant to do that!");
            return;
        }
    }

    if (restrict_owner == ANY_OWNER && restrict_type != TYPE_PLAYER)
    {
        if (!power (player, POW_EXAMINE))
        {
            send_message(player, "You need a search warrant to do that!");
            return;
        }
    }

    // make sure player has money to do the search
    if ( ! payfor(player, search_cost) )
    {
        send_message(player, "Searches cost %d credits", search_cost);
        return;
    }

    // room search
    if ( restrict_type == TYPE_ROOM || restrict_type == NOTYPE )
    {
        flag = FALSE;

        for ( thing = 0; thing < db_top; thing++ )
        {
            if ( Typeof(thing) != TYPE_ROOM )
            {
                continue;
            }

            if ( restrict_owner != ANY_OWNER && restrict_owner != db[thing].owner )
            {
                continue;
            }

            if ( (db[thing].flags & flag_mask) != flag_mask )
            {
                continue;
            }

            if ( restrict_name != NULL )
            {
                if ( ! string_prefix(db[thing].name, restrict_name) )
                {
                    continue;
                }
            }

            if (restrict_zone != NOTHING)
            {
                if (restrict_zone != db[thing].zone)
                {
                    continue;
                }
            }

            if (restrict_link != NOTHING && db[thing].link != restrict_link)
            {
                continue;
            }

            if ( flag )
            {
                flag = FALSE;
                destitute = 0;
                send_message(player, "");	// aack! don't use a newline! 
                send_message(player, "ROOMS:");
            }

            send_message(player, unparse_object(player, thing));
        }
    }

    // exit search
    if ( restrict_type == TYPE_EXIT || restrict_type == NOTYPE )
    {
        flag = TRUE;

        for ( thing = 0; thing < db_top; thing++ )
        {
            if ( Typeof(thing) != TYPE_EXIT )
            {
                continue;
            }

            if ( restrict_owner != ANY_OWNER && restrict_owner != db[thing].owner )
            {
                continue;
            }

            if ( (db[thing].flags & flag_mask) != flag_mask )
            {
                continue;
            }

            if ( restrict_name != NULL )
            {
                if ( ! string_prefix(db[thing].name, restrict_name) )
                {
                    continue;
                }
            }

            if (restrict_link != NOTHING && db[thing].link != restrict_link)
            {
                continue;
            }

            if ( flag )
            {
                flag = FALSE;
                destitute = 0;
                send_message(player, "");	// aack! don't use a newline!
                send_message(player, "EXITS:");
            }

            from = find_entrance(thing);
            to   = db[thing].link;

            strcpy(buf, unparse_object(player, thing));
            strcat(buf, " [from ");
            strcat(buf, from == NOTHING ? "NOWHERE" : unparse_object(player, from));
            strcat(buf, " to ");
            strcat(buf, to   == NOTHING ? "NOWHERE" : unparse_object(player,   to));
            strcat(buf, "]");

            send_message(player, buf);
        }
    }

    // object search 
    if ( restrict_type == TYPE_THING || restrict_type == NOTYPE )
    {
        flag = TRUE;

        for ( thing = 0; thing < db_top; thing++ )
        {
            if ( Typeof(thing) != TYPE_THING )
            {
                continue;
            }

            // we're not searching for going things 
            if (!(flag_mask & GOING))
            {
                // in case of free object 
                if ( db[thing].flags & GOING && !*atr_get(thing,A_DOOMSDAY) )
                {
                    continue;
                }
            }

            if ( restrict_owner != ANY_OWNER && restrict_owner != db[thing].owner )
            {
                continue;
            }

            if ( (db[thing].flags & flag_mask) != flag_mask )
            {
                continue;
            }

            if ( restrict_name != NULL )
            {
                if ( ! string_prefix(db[thing].name, restrict_name) )
                {
                    continue;
                }
            }

            if (restrict_link != NOTHING && db[thing].link != restrict_link)
            {
                continue;
            }

            if ( flag )
            {
                flag = FALSE;
                destitute = 0;
                send_message(player, "");	// aack! don't use a newline! 
                send_message(player, "OBJECTS:");
            }

            strcpy(buf, unparse_object(player, thing));
            strcat(buf, " [owner: ");
            strcat(buf, unparse_object(player, db[thing].owner));
            strcat(buf, "]");

            send_message(player, buf);
        }
    }

    // player search
    if ( restrict_type == TYPE_PLAYER || ( power(player,POW_EXAMINE) && restrict_type == NOTYPE ) )
    {
        flag = TRUE;
        for (thing = 0; thing < db_top; thing++)
        {
            if ( Typeof(thing) != TYPE_PLAYER )
            {
                continue;
            }

            if ( (db[thing].flags & flag_mask) != flag_mask )
            {
                continue;
            }

            // this only applies to wizards on this option
            if ( restrict_owner != ANY_OWNER && restrict_owner != db[thing].owner )
            {
                continue;
            }

            if ( restrict_name != NULL )
            {
                if ( ! string_prefix(db[thing].name, restrict_name) )
                {
                    continue;
                }
            }

            if ( restrict_class != 0 )
            {
                if ((!db[thing].pows) || db[thing].pows[0] != restrict_class)
                {
                    continue;
                }
            }

            if (restrict_link != NOTHING && db[thing].link != restrict_link)
            {
                continue;
            }

            if ( flag )
            {
                flag = FALSE;
                destitute = 0;
                send_message(player,"");	// aack! don't use newlines!
                send_message(player, "PLAYERS:");
            }

            strcpy(buf, unparse_object(player, thing));

            if ( controls(player,thing,POW_EXAMINE) )
            {
                strcat(buf, " [location: ");
                strcat(buf, unparse_object(player, db[thing].location));
                strcat(buf, "]");
            }
            send_message(player, buf);
        }
    }

    // if nothing found matching search criteria 
    if ( destitute )
    {
        send_message(player, "Nothing found.");
    }
}

static object_flag_type convert_flags(dbref player, int is_wizard, char *s, object_flag_type *p_mask, object_flag_type *p_type)
{
    int index;
    int last_id=' ';
    object_flag_type mask, type;

    // Should the below be placed outside local scope and into header file? -- Belisarius
    static struct
    {
        int id, type;
        object_flag_type bits;
    }

    fdata[] =
    {
        {'G',NOTYPE,GOING},{'p',NOTYPE,PUPPET},{'I',NOTYPE,INHERIT_POWERS},
        {'S',NOTYPE,STICKY},{'D',NOTYPE,DARK},{'L',NOTYPE,LINK_OK},
        {'H',NOTYPE,HAVEN},{'C',NOTYPE,CHOWN_OK},{'e',NOTYPE,ENTER_OK},
        {'s',TYPE_PLAYER,PLAYER_SLAVE},{'n',TYPE_PLAYER, PLAYER_NEWBIE},
        {'c',NOTYPE,CONNECT},{'K',TYPE_THING,THING_KEY},
        {'d',TYPE_THING,THING_DEST_OK},{'J',TYPE_ROOM,ROOM_JUMP_OK},
        {'R',TYPE_ROOM,0},{'E',TYPE_EXIT,0},{'P',TYPE_PLAYER,0},
        {'T',TYPE_THING,0},{'d',TYPE_ROOM,ROOM_DIG_OK},
        {'v',NOTYPE,SEE_OK},{'t',TYPE_PLAYER,PLAYER_TERSE},
        {'o',NOTYPE,OPAQUE},{'q',NOTYPE,QUIET},{'f',TYPE_ROOM,ROOM_FLOATING},
        {'N',TYPE_PLAYER,PLAYER_NO_WALLS},{'m',TYPE_PLAYER,PLAYER_MORTAL},
        {'X',TYPE_THING,THING_SACROK},{'l',TYPE_THING,THING_LIGHT},
        {'l',TYPE_ROOM,EXIT_LIGHT},{'b',NOTYPE, BEARING},
        {'A',TYPE_ROOM,ROOM_AUDITORIUM},{'a',TYPE_PLAYER,PLAYER_NO_ANN},
        {'+',TYPE_PLAYER,PLAYER_NO_COM},{'@',TYPE_PLAYER,PLAYER_ANSI},
        {0,0,0}
    };

    mask = 0;
    type = NOTYPE;

    for (; *s != '\0'; s++ )
    {
        // tmp patch to stop hidden cheating 
        if ( *s == 'c' && ! is_wizard )
        {
            continue;
        }

        for ( index = 0; fdata[index].id != 0; index++ )
        {
            if ( *s == fdata[index].id )
            {
                // handle object specific flag problems 
                if ( fdata[index].type != NOTYPE )
                {
                    // make sure we aren't specific to a different type 
                    if ( type != NOTYPE && type != fdata[index].type )
                    {
                        send_message(player, "Flag '%c' conflicts with '%c'.", last_id, fdata[index].id);
                        return FALSE;
                    }

                    // make us object specific to this type
                    type = fdata[index].type;

                    // always save last specific flag id
                    last_id = *s;
                }

                // add new flag into search mask
                mask |= fdata[index].bits;

                // stop searching for *this* flag
                break;
            }
        }

        // flag not found 
        if ( fdata[index].id == 0 )
        {
            send_message(player, "%c: unknown flag", (int) *s);
            return FALSE;
        }
    }

    // return new mask and type
    *p_mask = mask;
    *p_type = type;

    return TRUE;
}

void do_teleport(dbref player, char *arg1, char *arg2)
{
    dbref victim;
    dbref destination;
    char *to;

    // get victim, destination 
    if (*arg2 == '\0')
    {
        victim = player;
        to = arg1;
    }
    else
    {
        init_match(player, arg1, NOTYPE);
        match_neighbor();
        match_possession();
        match_me();
        match_absolute();
        match_player();
        match_exit();

        if ( (victim = noisy_match_result()) == NOTHING )
        {
            return;
        }

        to = arg2;
    }

    // get destination
    if (string_compare(to,"home"))
    {
        init_match(player, to, TYPE_PLAYER);
        match_here();
        match_absolute();
        match_neighbor();
        match_me();
        match_player();
        match_exit();

        destination = match_result();
    }
    else
    {
        destination = HOME;
    }

    switch (destination)
    {
        case NOTHING:
            send_message(player, "I don't know where %s is.", to);
            break;
        case AMBIGUOUS:
            send_message(player, "I don't know which %s you mean!", to);
            break;
        case HOME:
            if ( Typeof(victim) != TYPE_PLAYER && Typeof(victim) != TYPE_THING)
            {
                send_message(player, "Can't touch %s.", to);
                return;
            }
            if ( controls(player, victim, POW_TELEPORT) ||	controls(player, db[victim].location, POW_TELEPORT) )
            {
                // replace above 2 lines with IS() macro? 
                send_message(victim, "You feel a sudden urge to leave this place and go home...");
                safe_tel(victim,destination);
                return;
            }
            send_message(player, perm_denied());
            return;
        default:
            // check victim, destination types, teleport if ok 
            if (Typeof(victim) == TYPE_ROOM)
            {
                send_message(player, "Can't move rooms!");
                return;
            }
            if ( (Typeof(victim) == TYPE_EXIT
                  && (Typeof(destination) == TYPE_PLAYER || Typeof(destination) == TYPE_EXIT))
              || (Typeof(victim) == TYPE_PLAYER
                  && Typeof(destination) == TYPE_PLAYER))
            {
                send_message(player, "Bad destination.");
                return;
            }

            if ( Typeof(destination) != TYPE_EXIT )
            {
                if ( (controls(player, victim, POW_TELEPORT) || controls(player, db[victim].location, POW_TELEPORT))
                  && (Typeof(victim) != TYPE_EXIT || controls(player,destination, POW_MODIFY))
                  && (controls(player, destination, POW_TELEPORT) || IS(destination, TYPE_ROOM, ROOM_JUMP_OK))
                   )
                {
	                if (!check_zone(player, victim, destination, 1))
                    {
	                    return;
                   	}
                    else
                    {
	                    did_it(victim, get_zone_first(victim), A_LEAVE, NULL, A_OLEAVE, NULL, A_ALEAVE);
	                    safe_tel(victim, destination);
	                    did_it(victim, get_zone_first(victim), A_ENTER, NULL, A_OENTER, NULL, A_AENTER);
	                    return;
	                }
                }
                send_message(player, perm_denied());
                return;
            }
            else
            {
                // dest is TYPE_EXIT 
                if ( controls(player, db[victim].location, POW_TELEPORT) ||
	  controls(player,victim,POW_TELEPORT) || power(player, POW_TELEPORT) )
                {
          	      if ( (  (controls(player,db[victim].location,POW_TELEPORT) || controls(player,victim,POW_TELEPORT))
	                   && controls(player,destination,POW_TELEPORT))
          	         || power(player,POW_TELEPORT))
                    {
                        do_move(victim, to);
                    }
                    else
                    {
                        send_message(player, perm_denied());
                    }

                }
            }
    } // switch(destination)
}

// Note: special match, link_ok objects are considered
// controlled for this purpose 
dbref match_controlled(dbref player, char *name, int cutoff_level)
{
    dbref match;

    init_match(player, name, NOTYPE);
    match_everything();

    match = noisy_match_result();

    if (match != NOTHING && !controls(player, match, cutoff_level))
    {
        send_message(player, perm_denied());
        return NOTHING;
    }
    else
    {
        return match;
    }
}

void do_force(dbref player, char *what, char *command)
{
    dbref victim;

    if ((victim = match_controlled(player, what, POW_MODIFY)) == NOTHING)
    {
        send_message(player, "Sorry.");
        return;
    }

    //  if ((Typeof(victim) == TYPE_ROOM) || (Typeof(victim) == TYPE_EXIT))
    //  {
    //      send_message(player, "You can only force players and things.");
    //      return;
    //  }

    if ( victim == root )
    {
        send_message(player, "You can't force root!!");
        return;
    }

    // force victim to do command
    parse_que(victim, command, player);
}

int try_force(dbref player, char *command)
{
    char buff[BUF_SIZE];
    char *s;

    // first see if command prefixed by object # 
    if (*command=='#' && command[1] != ' ')
    {
        strcpy(buff, command);

        for(s = buff+1; *s && *s!=' '; s++)
        {
            if (!isdigit(*s))
            {
                return FALSE;
            }
        }

        if (!*s)
        {
            return(FALSE);
        }

        *s++ = 0;

        do_force(player, buff, s);

        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

void do_stats(dbref player, char *name)
{
    extern char *type_to_name();
    dbref owner;
    int i, total;
    int obj[NUM_OBJ_TYPES];
    int pla[NUM_CLASSES];

    if ( *name == '\0' )
    {
        owner = ANY_OWNER;
    }
    else if ( *name == '#' )
    {
        owner = atoi(&name[1]);

        if ( owner < 0 || db_top <= owner )
        {
            owner = NOTHING;
        }
        else if ( Typeof(owner) != TYPE_PLAYER )
        {
            owner = NOTHING;
        }
    }
    else if ( strcmp(name, "me") == 0 )
    {
        owner = player;
    }
    else
    {
        owner = lookup_player(name);
    }

    if ( owner == NOTHING )
    {
        send_message(player, "%s: No such player", name);
        return;
    }

    if ( ! controls(player, owner, POW_STATS) )
    {
        if ( owner != ANY_OWNER && owner != player )
        {
            send_message(player, "You need a search warrant to do that!");
            return;
        }
    }

    calc_stats(owner, &total, obj, pla);

    if (owner == ANY_OWNER)
    {
        send_message(player, "%s Database Breakdown:", muse_name);
    }
    else
    {
        send_message(player, "%s database breakdown for %s:", muse_name, unparse_object (player, owner));
    }

    send_message(player, "%7d Total Objects", total);

    for ( i = 0; i < NUM_OBJ_TYPES; i++ )
    {
        if (type_to_name(i) && *type_to_name(i) != ' ')
        {
            send_message(player, "%7d %ss", obj[i], type_to_name(i));
        }
    }

    send_message(player, "%7d %ss", pla[CLASS_CITIZEN], class_to_name(CLASS_CITIZEN));

#ifdef TEST_MALLOC
    if ( power(player, TYPE_HONWIZ) )
    {
        send_message(player, "Malloc count = %d.", malloc_count);
    }
#endif // TEST_MALLOC 
}

void do_pstats(dbref player, char *name)
{
    dbref owner;
    int total;
    int obj[NUM_OBJ_TYPES];  // number of object types 
    int pla[NUM_CLASSES];

    if ( *name == '\0' )
    {
        owner = ANY_OWNER;
    }
    else
    {
        send_message(player, "%s: No such player", name);
        return;
    }

    if ( ! power(player, POW_STATS) )
    {
        send_message(player, "Maybe next time. Sorry!");
        return;
    }

    calc_stats(owner, &total, obj, pla);
    send_message(player, "%s Player Breakdown:", muse_name);
    send_message(player, "%7d Players", obj[TYPE_PLAYER]);

    for (int i = 1; i < NUM_CLASSES; i++)
    {
        send_message(player, "%7d %ss", pla[i], class_to_name(i));
    }
}

// Old void calc_stats(dbref owner, int *total, int *players, int count[NUM_OBJ_TYPES]) 
void calc_stats(dbref owner, register int *total, int obj[NUM_OBJ_TYPES], int pla[NUM_CLASSES])
{
    dbref thing;

    // zero out count stats
    *total = 0;

    for (int i = 0; i < NUM_OBJ_TYPES; i++ )
    {
        obj[i] = 0;
    }

    for (int i = 0; i < NUM_CLASSES; i++)
    {
        pla[i] =0 ;
    }

    for (thing = 0; thing < db_top; thing++)
    {
        if (owner == ANY_OWNER || owner == db[thing].owner)
        {
            if ( !(db[thing].flags & GOING))
            {
      	        ++obj[Typeof(thing)];

                if (Typeof(thing) == TYPE_PLAYER)
                {
                    ++pla[(unsigned char)*db[thing].pows];
                }

      	        ++*total;
            }
        }
    }
}

int owns_stuff(dbref player)
{
    int matches = 0;

    for (dbref i = 0; i < db_top; i++)
    {
        if (db[i].owner == player && i != player) matches++;
    }

    return matches;
}

void do_wipeout(dbref player, char *arg1, char *arg3)
{
    int type;
    dbref victim;

    char *arg2;

    int do_all = FALSE;
    char buf[BUF_SIZE];

    if (!power(player, POW_SECURITY))
    {
        send_message(player, "Sorry, only wizards may perform mass destruction.");
        return;
    }

    sprintf(buf, "%s tries @wipeout %s=%s",unparse_object(player,player),arg1,arg3);
    log_sensitive(buf);

    for (arg2 = arg1; *arg2 && *arg2 != ' '; arg2++) ;

    if (!*arg2)
    {
        send_message(player, "You must specify the object type to destroy.");
        return;
    }

    *arg2 = '\0';
    arg2++;

    if (strcmp(arg2, "type"))
    {
        send_message(player, "The syntax is \"@wipeout <player> type=<obj type>\".");
        return;
    }

    victim = lookup_player(arg1);

    if (victim == NOTHING)
    {
        send_message(player, "No idea.. who's that?");
        return;
    }

    if (!controls(player, victim, POW_MODIFY))
    {
        send_message(player, perm_denied());
        return;
    }

    if (string_prefix("objects", arg3))
    {
        type = TYPE_THING;
    }
    else if (string_prefix("rooms", arg3))
    {
        type = TYPE_ROOM;
    }
    else if (string_prefix("exits", arg3))
    {
        type = TYPE_EXIT;
    }
    else if (!strcmp("all", arg3))
    {
        do_all = TRUE;
        type = NOTYPE;
    }
    else
    {
        send_message(player, "Unknown type.");
        return;
    }

    for (dbref n = 0; n < db_top; n++)
    {
        if ((db[n].owner == victim && n != victim) && (Typeof(n) == type || do_all))
        {
            destroy_obj (n, 60);      // destroy in 1 minute 
        }
    }

    switch (type)
    {
        case TYPE_THING:
            send_message(player, "Wiped out all objects.");
            send_message(victim, "All your objects have been destroyed by %s.", unparse_object (victim, player));
            break;
        case TYPE_ROOM:
            send_message(player, "Wiped out all rooms.");
            send_message(victim, "All your rooms have been destroyed by %s.", unparse_object (victim, player));
            break;
        case TYPE_EXIT:
            send_message(player, "Wiped out all exits.");
            send_message(victim, "All your exits have been destroyed by %s.", unparse_object (victim, player));
            break;
        case NOTYPE:
            send_message(player, "Wiped out every blessed thing.");
            send_message(victim, "All your stuff has been repossessed by %s. Oh, well.", unparse_object (victim, player));
            break;
    }
}

void do_chownall(dbref player, char *arg1, char *arg2)
{
    dbref playerA;
    dbref playerB;
    dbref n;
    char buf[BUF_SIZE];

    if (!power(player, POW_SECURITY))
    {
        send_message(player, "Sorry, only wizards may mass chown.");
        return;
    }

    sprintf(buf, "%s tries @chownall %s=%s", unparse_object(player, player), arg1, arg2);
    log_sensitive(buf);
    init_match(player, arg1, TYPE_PLAYER);
    match_neighbor();
    match_player();

    if ((playerA = noisy_match_result()) != NOTHING)
    {

        init_match(player, arg2, TYPE_PLAYER);
        match_neighbor();
        match_player();

        if ((playerB = noisy_match_result()) != NOTHING)
        {
            if ((controls(player, playerA, POW_MODIFY)) && (controls(player, playerB, POW_MODIFY)))
            {
                for (n = 0; n < db_top; n++)
                {
                    if (db[n].owner == playerA && n != playerA)
                    {
                        db[n].owner = playerB;
                    }
                }
            }
            else
            {
                send_message(player, perm_denied());
            }
        }
    }

    send_message(player, "Owner changed.");
}

void do_poor(dbref player, char *arg1)
{
    dbref a;
    int amt = atoi(arg1);

    if ( player != root )
    {
        return;
    }

    for ( a = 0; a < db_top; a++)
    {
        if ( Typeof(a) == TYPE_PLAYER )
        {
            s_Pennies(a, amt);
        }
    }
}

void do_allquota(dbref player, char *arg1)
{
    int count, limit, owned;
    char buf[20];
    dbref who, thing;

    if ( player != root )
    {
        send_message(player,"Don't. @allquota isn't nice.");
        return;
    }

    count = 0;
    send_message(player, "working...");

    for ( who = 0; who < db_top; who++ )
    {
        if ( Typeof(who) != TYPE_PLAYER )
        {
            continue;
        }

        // count up all owned objects 
        owned = -1;  // a player is never included in his own quota 

        for ( thing = 0; thing < db_top; thing++ )
        {
            if ( db[thing].owner == who )
            {
                if ((db[thing].flags&(TYPE_THING|GOING)) != (TYPE_THING|GOING))
                {
                    ++owned;
                }
            }
        }

        limit = atoi(arg1);

        // stored as a relative value
        sprintf(buf, "%d", limit - owned);
        atr_add(who, A_RQUOTA, buf);
        sprintf(buf, "%d", limit);
        atr_add(who, A_QUOTA, buf);

        ++count;
    }

    send_message(player, "done (%d players processed).", count);
}

void do_newpassword(dbref player, char *name, char *password)
{
    dbref victim;
    char buf[BUF_SIZE];

    if ((victim = lookup_player(name)) == NOTHING)
    {
        send_message(player, "%s: no such player.", name);
    }
    else if ( (Typeof(player)!=TYPE_PLAYER || !has_pow(player,victim,POW_NEWPASS))
          && !(Typeof(victim)!=TYPE_PLAYER && controls(player,victim,POW_MODIFY))
            )
    {
        send_message(player, perm_denied());
        return;
    }
    else if (*password != '\0' && !ok_password(password))
    {
        // Wiz can set null passwords, but not bad passwords 
        send_message(player, "Bad password");
    }
    else if ( victim == root && player != root )
    {
        send_message(player, "You cannot change that player's password.");
    }
    else
    {
        // it's ok, do it 
        s_Pass(victim, crypt(password, "XX"));
        send_message(player, "Password changed.");
        sprintf(buf, "%s(%d) executed: @newpassword %s(#%d)", db[player].name, player, db[victim].name, victim);
        log_sensitive(buf);
        send_message(victim, "Your password has been changed by %s.", db[player].name);
    }
}

void do_boot(dbref player, char *name)
{
    dbref victim;
    char buf[BUF_SIZE];

    // player only - no inherited powers

    init_match(player, name, TYPE_PLAYER);
    match_neighbor();
    match_absolute();
    match_player();

    if ((victim = noisy_match_result()) == NOTHING)
    {
        return;
    }

    if (!has_pow(player,victim,POW_BOOT) && !(Typeof(victim)!=TYPE_PLAYER && controls(player,victim,POW_BOOT)))
    {
        send_message(player, perm_denied());
        return;
    }

    if ( victim == root )
    {
        send_message(player, "You can't boot root!");
        return;
    }

    // notify people
    send_message(player, "%s - Booted.", db[victim].name);
    sprintf(buf, "%s(#%d) executed: @boot %s(#%d)", db[player].name, player, db[victim].name, victim);
    log_sensitive(buf);
    send_message (victim, "You have been booted by %s.",unparse_object (victim, player));
    boot_off(victim);
}

void do_cboot(dbref player, char *arg1)
{
    struct descriptor_data *d;
    int toboot;
    char buf[BUF_SIZE];

    if (!isdigit(*arg1))
    {
        send_message(player, "that's not a number.");
        return;
    }

    toboot = atoi(arg1);

    d = descriptor_list;
    while (d && d->concid != toboot)
    {
        d = d->next;
    }

    if (!d)
    {
        send_message(player, "Unable to find specified concid.");
        return;
    }

    if (d->state == CONNECTED)
    {
        if (has_pow(player, d->player, POW_BOOT))
        {
            sprintf(buf, "%s(#%d) executes: @cboot %d (descriptor %d, player %s(#%d))", db[player].name, player, toboot, d->descriptor, db[d->player].name, d->player);
            log_sensitive(buf);
            send_message(player, "Descriptor %d, concid %d (player %s) - Booted.", d->descriptor, toboot, unparse_object(player,d->player));
            shutdownsock(d);
        }
        else
        {
            // power check fail
            send_message(player, perm_denied());
            return;
        }
    }
    else
    {
        // not a connected player
        if (power(player, POW_BOOT))
        {
            sprintf(buf, "%s(#%d) executes: @cboot %d (descriptor %d, unconnected from host %s)", db[player].name, player, toboot, d->descriptor, d->addr);
            log_sensitive(buf);
            send_message(player, "Concid %d - Booted.", toboot);
            shutdownsock(d);
        }
        else
        {
            // power check fail
            send_message(player, perm_denied());
            return;
        }
    }
}


void do_join(dbref player, char *arg1)
{
    dbref victim;
    dbref to;

    // get victim, destination
    victim = player;
    to = lookup_player(arg1);

    if (!controls(victim, to, POW_JOIN) && !controls(victim, db[to].location, POW_JOIN))
    {
        send_message(player, "Sorry. You don't have wings.");
        return;
    }

    if ( to == NOTHING || db[to].location == NOTHING)
    {
        send_message(player, "%s: no such player.", arg1);
        return;
    }

    moveto(victim, db[to].location);
}

void do_summon(dbref player, char *arg1)
{
    dbref victim;
    dbref dest;

    // get victim, destination
    dest = db[player].location;
    victim = lookup_player(arg1);

    if ( ! controls(player, victim,POW_SUMMON) && !controls(player, db[victim].location, POW_SUMMON))
    {
        send_message(player, "Sorry. That player doesn't have wings.");
        return;
    }

    if ( victim == NOTHING )
    {
        send_message(player, "%s: no such player", arg1);
        return;
    }

    moveto(victim, dest);
}

void do_swap(dbref player, char *arg1, char *arg2)
{
    dbref thing1, thing2;
    struct object swapbuf;
    ATRDEF *d;
    struct descriptor_data *des;
    char buf[BUF_SIZE];

    thing1 = match_controlled(player, arg1, POW_MODIFY);

    if (thing1 == NOTHING)
    {
        return;
    }

    thing2 = match_controlled(player, arg2, POW_MODIFY);

    if (thing2 == NOTHING)
    {
        return;
    }

    if (Typeof(thing1) == TYPE_PLAYER || Typeof(thing2) == TYPE_PLAYER)
    {
        if (!power(player, POW_SECURITY))
        {
            send_message(player, perm_denied());
            return;
        }

        sprintf(buf, "%s executed: @swap %s=%s", unparse_object_a(root, player), unparse_object_a(root, thing1), unparse_object_a(root, thing2));
        log_sensitive(buf);
    }

    send_message(player, "%s and %s are now:", unparse_object_a(player, thing1), unparse_object_a(player,thing2));

    if (Typeof(thing1) == TYPE_PLAYER)
    {
        delete_player(thing1);
    }
    if (Typeof(thing2) == TYPE_PLAYER)
    {
        delete_player(thing2);
    }

    swapbuf = db[thing2];
    db[thing2] = db[thing1];
    db[thing1] = swapbuf;

// Should be placed outside of local scope or even in header -- Belisarius
#define SWAPREF(x) do { \
  if((x) == thing1) \
    (x) = thing2; \
  else if ((x) == thing2) \
    (x) = thing1; \
} while (0)

    for (dbref i = 0; i < db_top; i++)
    {
        int j;

        SWAPREF(db[i].location);
        SWAPREF(db[i].zone);
        SWAPREF(db[i].contents);
        SWAPREF(db[i].exits);
        SWAPREF(db[i].link);
        SWAPREF(db[i].next);
        SWAPREF(db[i].owner);

        for (j = 0; db[i].parents && db[i].parents[j] != NOTHING; j++)
        {
            SWAPREF(db[i].parents[j]);
        }

        for (j = 0; db[i].children && db[i].children[j] != NOTHING; j++)
        {
            SWAPREF(db[i].children[j]);
        }

        for (d = db[i].atrdefs; d; d = d->next)
        {
            SWAPREF(d->a.obj);
        }
    }

    for (des = descriptor_list; des; des=des->next)
    {
        if (des->state == CONNECTED)
        {
            SWAPREF(des->player);
        }
    }

    if (Typeof(thing1) == TYPE_PLAYER)
    {
        add_player(thing1);
    }

    if (Typeof(thing2) == TYPE_PLAYER)
    {
        add_player(thing2);
    }

    if (player == thing1)
    {
        send_message(thing2, "%s and %s.", unparse_object_a(player, thing1), unparse_object_a(player, thing2));
    }
    else if (player == thing2)
    {
        send_message(thing1, "%s and %s.", unparse_object_a(player, thing1), unparse_object_a(player, thing2));
    }
    else
    {
        send_message(player, "%s and %s.", unparse_object_a(player, thing1), unparse_object_a(player, thing2));
    }
}

void do_su (dbref player, char *arg1, char *arg2)
{
    dbref thing;
    struct descriptor_data *d;
    char buf[BUF_SIZE];

    thing = match_thing(player, arg1);

    if (thing == NOTHING)
    {
        return;
    }
    else
    {
        char *class = get_class(thing);
        int level = name_to_class(class);

        if (level == CLASS_GROUP)
        {
            send_message(player, perm_denied());
            return;
        }
    }

    if (*arg2)
    {
        sprintf(buf, "#%d", thing);

        if (connect_player(buf, arg2) != thing)
        {
            send_message(player,perm_denied());
            return;
        }

        sprintf(buf, "su: %s by %s",unparse_object_a(root,thing),unparse_object_a(root,player));
        log_io(buf);
    }
    else
    {
        if (!controls (player, thing, POW_MODIFY))
        {
            send_message(player,perm_denied());
            return;
        }

        sprintf(buf, "su: %s by %s", unparse_object_a(root, thing), unparse_object_a(root, player));
        log_sensitive(buf);
    }

    for (d = descriptor_list; d; d=d->next)
    {
        if (d->state == CONNECTED && d->player == player)
        {
            break;
        }
    }

    if (!d)
    {
        return;
    }

    announce_disconnect(d->player);
    d->player = thing;

    if (Guest(player))
    {
        struct descriptor_data *sd;
        int count = 0;

        for (sd = descriptor_list; sd; sd = sd->next)
        {
            if (sd->state == CONNECTED && sd->player == player)
            {
                ++count;
            }
        }

        if ( count == 0 )
        {
            destroy_guest(player);
        }
    }

    announce_connect(d->player);

}

void do_gethost(dbref player, char *arg1)
{
    dbref who;

    init_match(player, arg1, TYPE_PLAYER);
    match_neighbor();
    match_absolute();
    match_player();
    match_me();

    if ((who = noisy_match_result()) == NOTHING)
    {
        return;
    }

    if (!power(player, POW_HOST) && !power(player, POW_GHOST))
    {
        send_message(player, perm_denied());
        return;
    }
    else if (power(player, POW_GHOST) && !Guest(who) && !power(player, POW_HOST))
    {
        send_message(player, perm_denied());
        return;
    }
    else if (!(db[who].flags & CONNECT))
    {
        send_message(player, "That player is not connected!");
        return;
    }
    else
    {
        struct descriptor_data *d;

        for (d = descriptor_list; d; d = d->next)
        {
            if (d->player == who)
            {
                send_message(player, "Site for %s: %s@%s", unparse_object(root, who), d->user, d->addr);
            }
        }
              
    }
}
