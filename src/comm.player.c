///////////////////////////////////////////////////////////////////////////////
// player.c                                              TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: player.c,v 1.17 1993/09/07 18:24:24 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include <crypt.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "externs.h"
#include "credits.h"
#include "player.h"
#include "admin.h"

static void destroy_player P((dbref));

char *class_to_name();

dbref connect_player(char *name, char *password)
{
    dbref player;
    //long tt;
    //char *s; 

    player = lookup_player(name);

    if (player == NOTHING)
    {
       return NOTHING;
    }

    if ( (Typeof(player) != TYPE_PLAYER) && (!power(player, POW_INCOMING)) )
    {
       return NOTHING;
    }

    if (!password || !*password)
    {
       return NOTHING;
    }

    if ( (*Pass(player) || Typeof(player) != TYPE_PLAYER)
      && (strcmp(Pass(player), password) || !strncmp(Pass(player), "XX", 2))
      && strcmp(crypt(password, "XX"), Pass(player))
       )
    {
        if ( (*Pass(db[player].owner))
          && (strcmp(Pass(db[player].owner), password) || !strncmp(Pass(db[player].owner),"XX",2))
          && strcmp(crypt(password,"XX"),Pass(db[player].owner))
	       )
        {
            return NOTHING;
        }
    }

    return player;
}

dbref create_guest(char *name, char *password)
{
    dbref player;
    char key[10];

    // make sure that old guest id's don't hang around!
    player = lookup_player(name);
    if ( player != NOTHING )
    {
        if (db[player].pows && Guest(player))
        {
            destroy_player(player);
        }
        else
        {
            return NOTHING;
        }
    }

    // make new player 
    player = create_player(name, password, CLASS_GUEST, guest_start);

    if ( player == NOTHING )
    {
        log_error("failed in create_player");
        return NOTHING;
    }

    // lock guest to him/her-self
    sprintf(key, "#%d&!#%d", player, player);
    atr_add(player, A_LOCK, key);

    // set guest's description
    atr_add(player, A_DESC, guest_description);

    return player;
}

// extra precaution just in case a regular
// player is passed somehow to this routine 
void destroy_guest(dbref guest)
{
    if (!Guest(guest))
    {
        return;
    }

    destroy_player(guest);
}


dbref create_player(char *name, char *password, int class, dbref start)
{
    char buf[BUF_SIZE];
    dbref player;

    if (!ok_player_name(NOTHING, name, "")||!ok_password(password) || strchr(name,' '))
    {
        log_error("failed in name check");
        report();
        return NOTHING;
    }

    // else he doesn't already exist, create him 
    player = new_object();

    // initialize everything 
    SET(db[player].name,name);
    db[player].location = start;
    db[player].link = start;
    db[player].owner = player;
    //  db[player].flags = TYPE_TRIALPL;
    db[player].flags=TYPE_PLAYER | PLAYER_NEWBIE | INHERIT_POWERS;
    db[player].pows=malloc(sizeof(ptype)*2);
    db[player].pows[0] =CLASS_GUEST;	// re@class later. 
    db[player].pows[1] = 0;
    s_Pass(player,crypt(password,"XX"));
    giveto(player,initial_credits); // starting bonus 
    atr_add(player, A_RQUOTA, start_quota);
    atr_add(player, A_QUOTA, start_quota);

    // link him to start 
    PUSH(player, db[start].contents);

    add_player(player);

    if (class != CLASS_GUEST)
    {
        do_channel(player, "+public;pub");
    }

    sprintf(buf, "#%d", player);
    do_class(root, glurpdup(buf), class_to_name(class));

    return player;
}

static void destroy_player(dbref player)
{
    dbref loc, thing;

    // destroy all of the player's things/exits/rooms 
    for(thing = 0; thing < db_top; thing++)
    {
        if ( db[thing].owner == player && thing != player)
        {
            moveto(thing, NOTHING);

            switch (Typeof(thing))
            {
                case TYPE_PLAYER:
                    if (db[thing].owner == player && db[player].owner == thing)
                    {
                        db[thing].owner = thing;
                        db[player].owner = player;
                        destroy_player(thing);
                    }

                    do_empty(thing);
                    break;
                case TYPE_THING:
                    do_empty(thing);
                    break;
                case TYPE_EXIT:
                    loc = find_entrance(thing);
                    s_Exits(loc,remove_first(Exits(loc), thing));
                    do_empty(thing);
                    break;
                case TYPE_ROOM:
                    do_empty(thing);
                    break;
            }
        }
    }

    boot_off(player);           // disconnect player (just making sure) :) 
    do_halt(player, "");        // halt processes ? 
    moveto(player, NOTHING);    // send it to nowhere 
    delete_player(player);      // remove player from player list 
    do_empty(player);           // destroy player-object 
}

void do_pcreate(dbref creator, char *player_name, char *player_password)
{
    char buf[BUF_SIZE];
    dbref player;

    if ( ! power(creator, POW_PCREATE) )
    {
        send_message(creator, perm_denied());
        return;
    }

    if (lookup_player(player_name) != NOTHING)
    {
        send_message(creator, "there is already a %s",unparse_object(creator,lookup_player(player_name)));
        return;
    }

    if (!ok_player_name(NOTHING, player_name, "") || strchr(player_name,' '))
    {
        send_message(creator, "illegal player name '%s'",player_name);
        return;
    }

    player = create_player(player_name, player_password, CLASS_CITIZEN, player_start);

    if (player == NOTHING)
    {
        send_message(creator, "failure creating '%s'", player_name);
        return;
    }

    send_message(creator, "New player '%s' created with password '%s'",
			  player_name, player_password);
    sprintf(buf, "%s pcreated %s", unparse_object_a(root, creator), unparse_object_a(root, player));
    log_sensitive(buf);

    atr_add(player, A_CREATOR, unparse_object(root, creator));
}

void do_password(dbref player, char *old, char *newobj)
{
    if ( ! has_pow (player,NOTHING,POW_MEMBER) )
    {
        send_message(player, "Only registered %s users may change their passwords.", muse_name);
        return;

    }

    if (!*Pass(player) || (strcmp(old, Pass(player)) && strcmp(crypt(old, "XX"), Pass(player))))
    {
        send_message(player, "Sorry");
    }
    else if(!ok_password(newobj))
    {
        send_message(player, "Bad new password.");
    }
    else
    {
        s_Pass(player,crypt(newobj,"XX"));
        send_message(player, "Password changed.");
    }
}

void do_nuke(dbref player, char *name)
{
    char buf[BUF_SIZE];
    dbref victim;

    //    if ( RLevel(player) < TYPE_ADMIN )
    if ((!power(player,POW_NUKE)) || (Typeof(player)!=TYPE_PLAYER))
    {
        send_message(player, "This is a restricted command.");
        return;
    }

    init_match(player, name, TYPE_PLAYER);
    match_neighbor();
    match_absolute();
    match_player();

    if ((victim = noisy_match_result()) == NOTHING)
    {
        return;
    }

    if (Typeof(victim) != TYPE_PLAYER)
    {
        send_message(player, "You can only nuke players!");
    }
    else if ( ! controls(player, victim,POW_NUKE) )
    {
        send_message(player, "You don't have the authority to do that.");
    }
    else if (owns_stuff(victim))
    {
        send_message(player, "You must @wipeout their junk first.");
    }
    else
    {
        // get rid of that guy, destroy him 
        while (boot_off(victim)) ; // boot'm off lotsa times! 
        do_halt(victim, "");
        delete_player(victim);
        db[victim].flags = TYPE_THING;
        db[victim].owner = root;
        destroy_obj(victim, atoi(default_doomsday));

        send_message(player, "%s - nuked.", db[victim].name);
        sprintf(buf, "%s(#%d) executed: @nuke %s(#%d)", db[player].name, player, db[victim].name, victim);
        log_sensitive(buf);
    }
}

ptype name_to_pow(char *nam)
{
    int k;

    for (k = 0; k < NUM_POWS; k++)
    {
        if (!string_compare(powers[k].name, nam))
        {
            return powers[k].num;
        }
    }

    return FALSE;
}

static char *pow_to_name(ptype pow)
{
    int k;

    for (k = 0; k < NUM_POWS; k++)
    {
        if (powers[k].num == pow)
        {
            return powers[k].name;
        }
    }

    return "<unknown power>";
}

char *get_class(dbref player)
{
    extern char *type_to_name();

    if (Typeof(player) == TYPE_PLAYER)
    {
        return class_to_name(*db[player].pows);
    }
    else
    {
        return type_to_name(Typeof(player));
    }
}

// reclassify player 
void do_class(dbref player, char *arg1, char *class)
{
    int i, newlevel;
    dbref who;
    char buf[BUF_SIZE];

    if ( *arg1 == '\0' )
    {
        who = player;
    }
    else
    {
        init_match(player, arg1, TYPE_PLAYER);
        match_me();
        match_player();
        match_neighbor();
        match_absolute();

        if ( (who = noisy_match_result()) == NOTHING )
        {
            return;
        }
    }

    if (Typeof(who) != TYPE_PLAYER)
    {
        send_message(player, "Aint a player. Quit that animist stuff.");
        return;
    }

    if ( *class == '\0' )
    {
        if (!power(player, POW_WHO))
        {
            send_message(player, perm_denied());
            return;
        }

        // above search restricts this result to a PLAYER class 
        class = get_class(who);

        send_message(player, "%s is %s %s", db[who].name, (*class == 'O' || *class == 'A') ? "an" : "a", class);
        return;
    }

    // find requested level assignment
    i = name_to_class(class);

    if (i == 0)
    {
        send_message(player, "'%s': no such classification", class);
        return;
    }

    newlevel = i;

    // insure player has the power to make that assignment 
    // root can reclass without restriction.  Directors can
    // reclass people from any lower rank to any other lower
    // rank.  No other class may use this command. 
    sprintf(buf, "%s tried to: @class %s=%s", unparse_object(player, player), arg1, class);
    log_sensitive(buf);

    if (!has_pow(player,who,POW_CLASS)
       || (Typeof(player)!=TYPE_PLAYER)
       || ((newlevel >= db[player].pows[0]) && !is_root(player)))
    {
        send_message(player, "You don't have the authority to do that.");
        return;
    }

    // root must remain a director.  This is a safety feature. 
    if ( who == root && newlevel != CLASS_DIR)
    {
        send_message(player, "Sorry, player #%d cannot resign their position.", root);
        return;
    }
    sprintf(buf, "%s executed: @class %s=%s", unparse_object(player, player), arg1, class);
    log_sensitive(buf);

    //  db[who].flags &= ~TYPE_MASK;
    //  db[who].flags |= newlevel;
    send_message(player, "%s is now reclassified as: %s", db[who].name, class_to_name(newlevel));
    send_message(who, "You have been reclassified as: %s", class_to_name(newlevel));

    if (!db[who].pows)
    {
        db[who].pows = malloc(sizeof(ptype)*2);
        db[who].pows[1] = 0;
    }

    db[who].pows[0] = newlevel;

    for (i = 0; i < NUM_POWS; i++)
    {
        set_pow(who,powers[i].num,powers[i].init[class_to_list_pos(newlevel)]);
    }
}

void do_nopow_class(dbref player, char *arg1, char *class)
{
    char buf[BUF_SIZE];
    int i, newlevel;
    dbref who;

    if ( *arg1 == '\0' )
    {
        who = player;
    }
    else
    {
        init_match(player, arg1, TYPE_PLAYER);
        match_me();
        match_player();
        match_neighbor();
        match_absolute();

        if ( (who = noisy_match_result()) == NOTHING )
        {
            return;
        }
    }

    if (Typeof(who) != TYPE_PLAYER)
    {
        send_message(player,"Aint a player. Quit that animist stuff.");
        return;
    }

    if ( *class == '\0' )
    {
        if (!power(player, POW_WHO))
        {
            send_message(player,perm_denied());
            return;
        }

        // above search restricts this result to a PLAYER class 
        class = get_class(who);

        send_message(player, "%s is %s %s", db[who].name, (*class == 'O' || *class == 'A') ? "an" : "a", class);
        return;
    }

    // find requested level assignment 
    i = name_to_class(class);

    if (i == 0)
    {
        send_message(player, "'%s': no such classification", class);
        return;
    }

    newlevel = i;

    // insure player has the power to make that assignment 
    // root can reclass without restriction.  Directors can
    // reclass people from any lower rank to any other lower
    // rank.  No other class may use this command. 
    sprintf(buf, "%s tried to: @nopow_class %s=%s", unparse_object(player, player), arg1, class);
    log_sensitive(buf);

    if (!has_pow(player,who,POW_CLASS)
       || (Typeof(player)!=TYPE_PLAYER)
       || (((newlevel >= db[player].pows[0]) ||
	    (db[who].pows && db[who].pows[0]>newlevel)) && !is_root(player)))
    {
        send_message(player, "You don't have the authority to do that.");
        return;
    }

    // root must remain a director.  This is a safety feature. 
    if ( who == root && newlevel != CLASS_DIR)
    {
        send_message(player, "Sorry, player #%d cannot resign their position.", root);
        return;
    }
    sprintf(buf, "%s executed: @nopow_class %s=%s", unparse_object(player, player), arg1, class);
    log_sensitive(buf);

    //db[who].flags &= ~TYPE_MASK;
    //db[who].flags |= newlevel;
    send_message(player, "%s is now reclassified as: %s", db[who].name, class_to_name(newlevel));
    send_message(who, "You have been reclassified as: %s", class_to_name(newlevel));

    if (!db[who].pows)
    {
        db[who].pows = malloc(sizeof(ptype)*2);
        db[who].pows[1] = 0;
    }
    db[who].pows[0] = newlevel;
}

void do_empower(dbref player, char *whostr, char *powstr)
{
    char buf[BUF_SIZE];
    ptype pow;
    ptype powval;
    dbref who;
    int k;
    char *i;

    if (Typeof(player) != TYPE_PLAYER)
    {
        send_message(player, "You're not a player, silly!");
        return;
    }

    i = strchr(powstr, ':');

    if (!i)
    {
        send_message(player, "Badly formed power specification. need powertype:powerval");
        return;
    }
    *(i++) = '\0';

    if(!string_compare(i, "yes"))
    {
        powval = PW_YES;
    }
    else if(!string_compare(i, "no"))
    {
        powval = PW_NO;
    }
    else if(!string_compare(i, "yeseq"))
    {
        powval = PW_YESEQ;
    }
    else if(!string_compare(i, "yeslt"))
    {
        powval = PW_YESLT;
    }
    else
    {
        send_message(player, "The power value must be one of yes, no, yeseq, or yeslt");
        return;
    }

    pow = name_to_pow(powstr);

    if (!pow)
    {
        send_message(player, "unknown power: %s", powstr);
        return;
    }

    who = match_thing(player, whostr);

    if (who == NOTHING)
    {
        return;
    }

    if (Typeof(who) != TYPE_PLAYER)
    {
        send_message(player, "ain't a player.");
        return;
    }

    if (!has_pow(player, who, POW_SETPOW))
    {
        send_message(player, perm_denied());
        return;
    }

    if (get_pow(player, pow) < powval && !is_root(player))
    {
        send_message(player, "But you yourself don't have that power!");
        return;
    }

    for (k = 0; k < NUM_POWS; k++)
    {
        if (powers[k].num == pow)
        {
            if (powers[k].max[class_to_list_pos(*db[db[who].owner].pows)] >= powval)
            {
                set_pow(who,pow,powval);
                sprintf(buf, "%s(%d) granted %s(%d) power %s, level %s", db[player].name, (int)player, db[who].name, (int)who, powstr, i);
                log_sensitive(buf);

                if (powval != PW_NO)
                {
                    send_message(who, "You have been given the power of %s.",pow_to_name(pow));
                    send_message(player, "%s has been given the power of %s.", db[who].name, pow_to_name(pow));
                }

                switch(powval)
                {
                    case PW_YES:
                        send_message(who,"You can use it on anyone");
                        break;
                    case PW_YESEQ:
                        send_message(who,"You can use it on people your class and under");
                        break;
                    case PW_YESLT:
                        send_message(who,"You can use it on people under your class");
                        break;
                    case PW_NO:
                        send_message(who, "Your power of %s has been removed.", pow_to_name(pow));
                        send_message(player, "%s's power of %s has been removed.", db[who].name, pow_to_name(pow));
                        break;
                }
                return;
            }
            else
            {
                send_message(player, "Sorry, that is beyond the maximum for that level.");
                return;
            }
            send_message(player, "Internal error. Help.");
        }
    }
}

void do_powers(dbref player, char *whostr)
{
    dbref who;
    int k;

    who = match_thing(player, whostr);

    if (who == NOTHING)
    {
        return;
    }

    if (Typeof(who) != TYPE_PLAYER)
    {
        send_message(player, "ain't a player.{");
        return;
    }

    if (!controls(player, who, POW_EXAMINE))
    {
        send_message(player, perm_denied());
        return;
    }

    send_message(player, "%s's powers:", db[who].name);

    for (k = 0;k < NUM_POWS; k++) {
        ptype m;
        char *l = "";

        m = get_pow(who, powers[k].num);

        switch(m)
        {
            case PW_YES:
                // stay "". 
                break;
            case PW_YESLT:
                l = "for lower classes";
                break;
            case PW_YESEQ:
                l = "for equal and lower classes";
                break;
            case PW_NO:
                continue;
        }

        if (l)
        {
            send_message(player, "  %s (%s) %s",powers[k].description, powers[k].name, l);
        }
    }

    send_message(player,"-- end of list --");
}

int old_to_new_class(int lev)
{
    switch(lev)
    {
        case 0x8:
            return CLASS_GUEST;
        case 0x9:
            return CLASS_VISITOR;
        case 0xA:
            return CLASS_CITIZEN;
        case 0xB:
            return CLASS_GUIDE;
        case 0xC:
            return CLASS_OFFICIAL;
        case 0xD:
            return CLASS_BUILDER;
        case 0xE:
            return CLASS_ADMIN;
        case 0xF:
            return CLASS_DIR;
        default:
          return CLASS_VISITOR;
    }
}

void do_money(dbref player, char *arg1, char *arg2)
{
    int amt, assets;
    dbref who;
    char *credits, buf[20];
    dbref total;
    int obj[NUM_OBJ_TYPES];
    int pla[NUM_CLASSES];

    if ( *arg1 == '\0' )
    {
        who = player;
    }
    else
    {
        init_match(player, arg1, TYPE_PLAYER);
        match_me();
        match_player();
        match_neighbor();
        match_absolute();

        if ( (who = noisy_match_result()) == NOTHING )
        {
            return;
        }
    }

    if ( ! power(player, POW_EXAMINE) )
    {
        if ( *arg2 != '\0' )
        {
            send_message(player, "You don't have the authority to do that.");
            return;
        }

        if ( player != who )  {
            send_message(player, "You need a search warrant to do that.");
            return;
        }
    }

    // calculate assets 
    calc_stats(who, &total, obj, pla);
    assets = obj[TYPE_EXIT]      * exit_cost   +   // exits 
             obj[TYPE_THING]     * thing_cost  +   // things (objects) 
             obj[TYPE_ROOM]      * room_cost   +   // rooms 
            (obj[TYPE_PLAYER]-1) * robot_cost;     // robots 

    // calculate credits 
    if (inf_mon(who))
    {
        amt = 0;
        credits = "UNLIMITED";
    }
    else
    {
        amt = Pennies(who);
        sprintf(buf, "%d credits.", amt);
        credits = buf;
    }

    send_message(player, "Cash...........: %s", credits);
    send_message(player, "Material Assets: %d credits.", assets);
    send_message(player, "Total Net Worth: %d credits.", assets+amt);
    send_message(player, " ");
    send_message(player, "Note: material assets calculation is only an approximation (for now).");
}

void do_quota(dbref player, char *arg1, char *arg2)
{
    dbref who;
    char buf[20];
    int owned, limit;

    // Stop attempts to change quota without authority 
    if ( *arg2 )
    {
        if ( ! power(player, POW_SETQUOTA) )
        {
            send_message(player, "You don't have the authority to change someone's quota!");
            return;
        }
    }

    if ( *arg1 == '\0' )
    {
        who = player;
    }
    else
    {
        if ((who = lookup_player(arg1)) == NOTHING || Typeof(who) != TYPE_PLAYER)
        {
            send_message(player, "Who?");
            return;
        }
    }

    if ( Robot(who) )
    {
        send_message(player, "Robots don't have quotas!");
        return;
    }

    // check players authority to control his target 
    if ( ! controls(player, who, POW_SETQUOTA) )
    {
        send_message(player, "You can't %s that player's quota.", (*arg2) ? "change" : "examine");
        return;
    }

    // count up all owned objects 
    
    owned = -1;  // a player is never included in his own quota
  
    // Doesn't this still need to be done? -- Belisarius
    //for ( thing = 0; thing < db_top; thing++ )
    //{
    //    if ( db[thing].owner == who )
    //    {
    //        if ((db[thing].flags & (TYPE_THING|GOING)) != (TYPE_THING|GOING))
    //        {
    //            ++owned;
    //        }
    //    }
    //}

    owned = atoi(atr_get(who, A_QUOTA)) - atoi(atr_get(who, A_RQUOTA));

    // calculate and/or set new limit 
    if ( *arg2 == '\0' )
    {
        limit = owned + atoi(atr_get(who, A_RQUOTA));
    }
    else
    {
        limit = atoi(arg2);

        // stored as a relative value 
        sprintf(buf, "%d", limit - owned);
        atr_add(who, A_RQUOTA, buf);
        sprintf(buf, "%d", limit);
        atr_add(who, A_QUOTA, buf);
    }

    send_message(player, "Objects: %d   Limit: %d", owned, limit);

}

dbref *match_things(dbref player, char *list)
{
    char *x;
    static dbref npl[BUF_SIZE];
    char in[BUF_SIZE];
    char *inp=in;

    if (!*list)
    {
        send_message(player, "You must give a list of things.");
        npl[0] = 0;
        return npl;
    }

    npl[0] = 0;
    strcpy(in, list);

    while (inp)
    {
        x = parse_up(&inp, ' ');

        if (!x)
        {
            inp = x;
        }
        else
        {
            if (*x == '{' && *(strchr(x,'\0')-1) == '}')
            {
                x++;
                *(strchr(x,'\0')-1) = '\0';
            }

            npl[++npl[0]]=match_thing(player, x);

            if (npl[npl[0]] == NOTHING)
            {
                npl[0]--;
            }
        }
    }

    return npl;
}

dbref *lookup_players(dbref player, char *list)
{
    char *x;
    static dbref npl[BUF_SIZE];	// first is number of them. 
    char in[BUF_SIZE];

    char *inp = in;

    if (!*list)
    {
        send_message(player, "You must give a list of players.");
        npl[0] = 0;
        return npl;
    }

    npl[0] = 0;
    strcpy(in, list);

    while (inp)
    {
        x = parse_up(&inp, ' ');

        if (!x)
        {
            inp = x;
        }
        else
        {
            npl[++npl[0]] = lookup_player(x);

            if (npl[npl[0]] == NOTHING)
            {
                send_message(player, "I don't know who %s is.", x);
                npl[0]--;
            }
        }
    }

    return npl;
}

void do_misc(dbref player, char *arg1, char *arg2)
{
}
