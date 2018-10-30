// look.c 
// $Id: look.c,v 1.18 1993/09/18 19:03:42 nils Exp $ 
// commands which look at things 

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

static char *eval_sweep P((dbref));

static void look_exits(dbref player, dbref loc, char *exit_name)
{
    dbref thing;
    char buff[BUF_SIZE];

    char *e  = buff;
    int flag = 0;

    // make sure location is a room 
    // if(db[loc].flags & DARK)
    //     return;
    for (thing = Exits(loc); (thing != NOTHING) && (Dark(thing)); thing = db[thing].next) ;

    if (thing == NOTHING)
    {
        return;
    }

    // send_message(player, exit_name);
    for (thing = Exits(loc); thing != NOTHING; thing = db[thing].next)
    {
        if (!(Dark(loc)) || (IS(thing, TYPE_EXIT, EXIT_LIGHT) && controls(thing, loc, POW_MODIFY)))
        {
            if (!(Dark(thing)))
            {
                // chop off first exit alias to display 
                char *s;

                if (!flag)
                {
                    send_message(player, exit_name);
                    flag++;
                }

                if (db[thing].name && ((e-buff) < 1000))
                {
                    for (s = db[thing].name; *s && (*s!=';') && (e-buff < 1000); *e++ = *s++) ;

                    *e++ = '\033';
                    *e++ = '[';
                    *e++ = '0';
                    *e++ = 'm';
                    *e++ = ' ';
                    *e++ = ' ';
                }
            }
        }
    }

    *e++ = 0;

    if(*buff)
    {
        send_message(player, buff);
    }
}


static void look_contents(dbref player, dbref loc, char *contents_name)
{
    dbref thing;
    dbref can_see_loc;
    int contents = 0;

    // check to see if he can see the location */
    // patched so that player can't see in dark rooms even if
    // owned by that player.  (he must use examine command) 
    can_see_loc = ! Dark(loc);

    // check to see if there is anything there 
    DOLIST(thing, db[loc].contents)
    {
        if (can_see(player, thing, can_see_loc))
        {
            contents++;
        }
    }

    if (contents)
    {
        // something exists!  show him everything 
        send_message(player, contents_name);

        DOLIST(thing, db[loc].contents)
        {
            if (can_see(player, thing, can_see_loc))
            {
                send_message(player, "%s %s", unparse_object(player, thing), atr_get(thing, A_CAPTION));
            }
        }
    }
}

static struct all_atr_list *all_attributes_internal(dbref thing, struct all_atr_list *myop, struct all_atr_list *myend, int dep)
{
    ALIST *attributeList;
    struct all_atr_list *tmp;

    for (attributeList = db[thing].list; attributeList; attributeList = AL_NEXT(attributeList))
    {
        if (AL_TYPE(attributeList) && ((dep == 0) || (AL_TYPE(attributeList)->flags & AF_INHERIT)))
        {
            for (tmp = myop; tmp; tmp = tmp->next)
            {
                if (tmp->type == AL_TYPE(attributeList))
                {
                    break;
                }
            }

            if (tmp)
            {
                continue; // there's already something with this type.
            }

            if (!myop)
            {
                myop = myend = newglurp(sizeof(struct all_atr_list));
                myop->next = NULL;
            }
            else
            {
                struct all_atr_list *foo;
                foo = newglurp(sizeof(struct all_atr_list));
                foo->next = myop;
                myop = foo;
            }

            myop->type = AL_TYPE(attributeList);
            myop->value = AL_STR(attributeList);
            myop->numinherit = dep;
        }
    }

    for (int i = 0; db[thing].parents && db[thing].parents[i] != NOTHING; i++)
    {
        if (myend)
        {
            myop = all_attributes_internal (db[thing].parents[i], myop, myend, dep+1);
        }
        else
        {
            myop = all_attributes_internal (db[thing].parents[i], 0, 0, dep+1);
        }

        while (myend && myend->next)
        {
            myend = myend->next;
        }
    }

    return myop;
}

struct all_atr_list *all_attributes(dbref thing)
{
    return all_attributes_internal (thing, 0, 0, 0); // start with 0 depth 
}

static char* unparse_list(dbref player, char *list)
{
    static char buf[BUF_SIZE];
    int pos = 0;

    while (pos < 9990 && *list)
    {
        if (*list == '#' && list[1] >= '0' && list[1] <= '9')
        {
            int y = atoi(list + 1);
            char *x;

            if ( (y >= HOME) || (y < db_top) )
            {
                x = unparse_object(player,y);

                if ((strlen(x) + pos) < 9900)
                {
                    buf[pos] = ' ';
                    strcpy((buf + pos + 1), x);
                    pos += strlen(buf + pos);
                    list++; // skip the # 

                    while (*list >= '0' && *list <= '9')
                    {
                        list++;
                    }

                    continue;
                }
            }
        }

        buf[pos++] = *list++;
    }

    buf[pos] = '\0';
    
    return ( (*buf) ? (buf + 1) : buf );
}

static void look_atr(dbref player, struct all_atr_list *allatrs)
{
    long cl;
    ATTR *attr;

    attr = allatrs->type;

    if ( attr->flags & AF_DATE )
    {
        cl = atol((allatrs->value));
        send_message(player, "%s: %s", unparse_attr(attr, allatrs->numinherit), mktm(cl, "D", player));
    }
    else
    {
        if (attr->flags & AF_LOCK)
        {
            send_message(player, "%s: %s", unparse_attr(attr, allatrs->numinherit), unprocess_lock(player, glurpdup(allatrs->value)));
        }
        else if (attr->flags & AF_FUNC)
        {
            send_message(player, "%s(): %s", unparse_attr(attr, allatrs->numinherit), (allatrs->value));
        }
        else if (attr->flags & AF_DBREF)
        {
            send_message(player, "%s: %s", unparse_attr(attr, allatrs->numinherit), unparse_list(player,(allatrs->value)));
        }
        else
        {
            send_message(player, "%s: %s", unparse_attr(attr, allatrs->numinherit), (allatrs->value));
        }
    }
		    
}

static void look_atrs(dbref player, dbref thing, int doall)
{
    struct all_atr_list *allatrs;
    ATTR *attr;

    for (allatrs = all_attributes(thing); allatrs; allatrs=allatrs->next)
    {
        if ( (allatrs->type != A_DESC)
          && (attr = allatrs->type)
          && (allatrs->numinherit == 0 || !(db[allatrs->type->obj].flags&SEE_OK) || doall)
          && can_see_atr(player, thing, attr)
           )
        {
            look_atr(player,allatrs);
        }
    }
}

static void look_simple(dbref player, dbref thing, int doatrs)
{
    if (controls(player,thing,POW_EXAMINE) || (db[thing].flags & SEE_OK))
    {
        send_message(player, "%s %s", unparse_object(player,thing), atr_get(thing, A_CAPTION));
    }
          
  /*    if(*Desc(thing)) {
        send_message(player, Desc(thing));
	} else {
        send_message(player, "You see nothing special.");
	}*/

    if (doatrs)
    {
        did_it(player, thing, A_DESC, "What you see is beyond description.", A_ODESC, NULL, A_ADESC);
    }
    else
    {
        did_it(player, thing, A_DESC, "What you see is beyond description.", NULL, NULL, NULL);
    }

    if (Typeof(thing) == TYPE_EXIT)
    {
        if (db[thing].flags & OPAQUE)
        {
            if (db[thing].link != NOTHING)
            {
                send_message(player, "You peer through to %s...", db[db[thing].link].name);
                did_it (player, db[thing].link, A_DESC, "You see nothing on the other side.",doatrs?A_ODESC:NULL,NULL,doatrs?A_ADESC:NULL);
                look_contents(player, db[thing].link, "You also notice:");
            }
        }
    }
}

void look_room(dbref player, dbref loc)
{
    char *s;

    // tell him the name, and the number if he can link to it 
    send_message(player, unparse_object(player, loc));

    if (Typeof(loc)!=TYPE_ROOM)
    {
        did_it(player, loc, A_IDESC, NULL, A_OIDESC, NULL, A_AIDESC);
    }
    // tell him the description 
    //     if(*Desc(loc)) send_message(player, Desc(loc));
    else
    {
        if (!(db[player].flags & PLAYER_TERSE))
        {
            if (*(s=atr_get(get_zone_first(player),A_IDESC)) && !(db[loc].flags & OPAQUE))
            {
                send_message(player,s);
            }

            did_it(player, loc, A_DESC, NULL, A_ODESC, NULL, A_ADESC);
        }
    }

    // tell him the appropriate messages if he has the key 
    if (Typeof(loc) == TYPE_ROOM)
    {
        if (could_doit(player, loc, A_LOCK))
        {
            did_it(player, loc, A_SUCC, NULL, A_OSUCC, NULL, A_ASUCC);
        }
        else
        {
            did_it(player, loc, A_FAIL, NULL, A_OFAIL, NULL, A_AFAIL);
        }
    }

    // tell him the contents 
    look_contents(player, loc, "Contents:");
    look_exits(player, loc, "Obvious exits:");
}

void do_look_around(dbref player)
{
    dbref loc;

    if ((loc = getloc(player)) == NOTHING)
    {
        return;
    }

    look_room(player, loc);
}

void do_look_at(dbref player, char *arg1)
{
    dbref thing, thing2;
    char *s, *p;
    char *name = arg1; // so we don't mangle it

    if (*name == '\0')
    {
        if ((thing = getloc(player)) != NOTHING)
        {
            look_room(player, thing);
        }
    }
    else
    {
        // look at a thing here 
        init_match(player, name, NOTYPE);
        match_exit();
        match_neighbor();
        match_possession();

        if (power(player, POW_REMOTE))
        {
            match_absolute();
            match_player();
        }

        match_here();
        match_me();

        switch(thing = match_result())
        {
            case NOTHING:
                for (s = name; *s && *s != ' '; s++) ;

                if (!*s)
                {
                    // ? a preformatted string -- Belisarius
                    send_message(player, NOMATCH_PATT, arg1);
                    return;
                }

                p = name;

                if ( (*(s - 1) == 's' && *(s - 2) == '\'' && *(s - 3) != 's') 
                  || (*(s - 1) == '\'' && *(s - 2) == 's')
                   )
                {
                    if (*(s - 1) == 's')
                    {
                        *(s - 2) = '\0';
                    }
                    else
                    {
                        *(s - 1) = '\0';
                    }

                    name = s + 1;
                    init_match(player, p, TYPE_PLAYER);
                    match_neighbor();
                    match_possession();

                    switch (thing = match_result())
                    {
                        case NOTHING:
                            // ? a preformatted string? -- Belisarius
                            send_message(player, NOMATCH_PATT, arg1);
                            break;
                        case AMBIGUOUS:
                            send_message(player, AMBIGUOUS_MESSAGE);
                            break;
                        default:
                            init_match(thing, name, TYPE_THING);
                            match_possession();

                            switch(thing2 = match_result())
                            {
                                case NOTHING:
                                    // ? a preformatted string? -- Belisarius
                                    send_message(player, NOMATCH_PATT, arg1);
                                    break;
                                case AMBIGUOUS:
                                    send_message(player, AMBIGUOUS_MESSAGE);
                                    break;
                                default:
                                    if ( (db[thing].flags & OPAQUE)
                                      && (!power(player, POW_EXAMINE))
                                       )
                                    {
                                        // ? a preformatted string? -- Belisarius
                                        send_message(player, NOMATCH_PATT, name);
                                    }
                                    else
                                    {
                                        look_simple(player, thing2, 0);
                                    }
                            }
                    }
                }
                else
                {
                    // ? a preformatted string? -- Belisarius
                    send_message(player, NOMATCH_PATT, arg1);
                }
                break;
            case AMBIGUOUS:
                send_message(player, AMBIGUOUS_MESSAGE);
                break;
            default:
                switch(Typeof(thing))
                {
                    case TYPE_ROOM:
                        look_room(player, thing);
                        break;
                    case TYPE_THING:
                    case TYPE_PLAYER:
                        look_simple(player, thing, 1);
                        //look_atrs(player,thing);

                        if ( controls(player, thing, POW_EXAMINE)
                          || !(db[thing].flags & OPAQUE)
                          ||  power(player, POW_EXAMINE)
                           )
                        {
                            look_contents(player, thing, "Carrying:");
                        }

                        break;
                    default:
                        look_simple(player, thing, 1);
                        //look_atrs(player,thing);
                        break;
                }
        }
    }
}

char *flag_description(dbref thing)
{
    static char buf[MAX_CMD_BUF];

    strcpy(buf, "Type: ");

    switch(Typeof(thing))
    {
        case TYPE_ROOM:
            strcat(buf, "Room");
            break;
        case TYPE_EXIT:
            strcat(buf, "Exit");
            break;
        case TYPE_THING:
            strcat(buf, "Thing");
            break;
        case TYPE_PLAYER:
            strcat(buf, "Player");
            break;
        default:
            strcat(buf, "***UNKNOWN TYPE***");
            break;
    }

    if (db[thing].flags & ~TYPE_MASK)
    {
        // print flags 
        strcat(buf, "      Flags:");
        if(db[thing].flags & GOING) strcat(buf, " going");
        if(db[thing].flags & PUPPET) strcat(buf, " puppet");
        if(db[thing].flags & STICKY) strcat(buf, " sticky");
        if(db[thing].flags & DARK) strcat(buf, " dark");
        if(db[thing].flags & LINK_OK) strcat(buf, " link_ok");
        if(db[thing].flags & HAVEN) strcat(buf, " haven");
        if(db[thing].flags & CHOWN_OK) strcat(buf, " chown_ok");
        if(db[thing].flags & ENTER_OK) strcat(buf, " enter_ok");
        if(db[thing].flags & SEE_OK) strcat(buf, " visible");
        if(db[thing].flags & OPAQUE) strcat(buf, (Typeof(thing) == TYPE_EXIT) ? " transparent" : " opaque");
        if(db[thing].flags & INHERIT_POWERS) strcat(buf," inherit");
        if(db[thing].flags & QUIET) strcat(buf, " quiet");
        if(db[thing].flags & BEARING) strcat(buf, " bearing");
        if(db[thing].flags & CONNECT) strcat(buf, " connected");

        switch(Typeof(thing))
        {
            case TYPE_PLAYER:
                if(db[thing].flags & PLAYER_SLAVE) strcat(buf, " slave");
                if(db[thing].flags & PLAYER_TERSE) strcat(buf, " terse");
                if(db[thing].flags & PLAYER_ANSI) strcat(buf, " ansi");
                if(db[thing].flags & PLAYER_MORTAL) strcat(buf, " mortal");
                if(db[thing].flags & PLAYER_NO_WALLS) strcat(buf, " no_walls");
                if(db[thing].flags & PLAYER_NO_COM) strcat(buf, " no_com");
                if(db[thing].flags & PLAYER_NO_ANN) strcat(buf, " no_ann");
                break;
            case TYPE_EXIT:
                if(db[thing].flags & EXIT_LIGHT) strcat(buf, " light");
                break;
            case TYPE_THING:
                if(db[thing].flags & THING_KEY) strcat(buf, " key");
                if(db[thing].flags & THING_DEST_OK) strcat(buf, " destroy_ok");
                if(db[thing].flags & THING_SACROK) strcat(buf, " x_ok");
                if(db[thing].flags & THING_LIGHT) strcat(buf, " light");
                if(db[thing].flags & THING_DIG_OK) strcat(buf, " dig_ok");
                break;
            case TYPE_ROOM:
                if(db[thing].flags&ROOM_JUMP_OK) strcat(buf, " jump_ok");
                if(db[thing].flags&ROOM_AUDITORIUM) strcat(buf, " auditorium");
                if(db[thing].flags&ROOM_FLOATING) strcat(buf, " floating");
                if(db[thing].flags&ROOM_DIG_OK) strcat(buf, " dig_ok");
                break;
        }

        if (db[thing].i_flags & I_MARKED) strcat(buf, " marked");
        if (db[thing].i_flags & I_QUOTAFULL) strcat(buf, " quotafull");
        if (db[thing].i_flags & I_UPDATEBYTES) strcat(buf, " updatebytes");
    }

    return buf;
}

void do_examine(dbref player, char *name, char *arg2)
{
    dbref thing;
    dbref exit;
    dbref enter;
    char *rq, *rqm, *cr, *crm, buf[10];
    struct all_atr_list attr_entry;

    int pos = 0;

    if (*name == '\0')
    {
        if ((thing = getloc(player)) == NOTHING)
        {
            return;
        }
    }
    else
    {
        if (strchr(name, '/'))
        {
            if (!parse_attrib(player,name,&thing,&(attr_entry.type),0))
            {
                send_message(player, "No match.");
            }
            else if (!can_see_atr(player, thing, attr_entry.type))
            {
	            send_message(player, perm_denied());
            }
            else
            {
                attr_entry.value = atr_get(thing, attr_entry.type);
                attr_entry.numinherit = 0; // could find this, prob'ly not worth it 
                look_atr(player, &attr_entry);
            }

            return;
        }

        // look it up
        init_match(player, name, NOTYPE);
        match_exit();
        match_neighbor();
        match_possession();
        match_absolute();

        // only Wizards can examine other players 
        if (has_pow(player, NOTHING, POW_EXAMINE) || has_pow(player, NOTHING,POW_REMOTE))
        {
            match_player();
        }

        //if(db[thing].flags & UNIVERSAL)
        //{
	//    match_absolute();
        //}
        match_here();
        match_me();

        // get result
        if ((thing = noisy_match_result()) == NOTHING)
        {
            return;
        }
    }

    if ((! can_link(player, thing, POW_EXAMINE)  && ! (db[thing].flags & SEE_OK)))
    {
        char buf2[MAX_CMD_BUF];

        strcpy(buf2, unparse_object(player, thing));
        send_message(player, "%s is owned by %s", buf2, unparse_object(player, db[thing].owner));

        look_atrs(player,thing, *arg2);	// only show osee attributes
        // check to see if there is anything there 
        // as long as he owns what is there 

        look_contents(player, thing, "Contents:");
        return;
    }

    send_message(player, unparse_object(player, thing));

    if (*Desc(thing) && can_see_atr (player, thing, A_DESC))
    {
        send_message(player,Desc(thing));
    }

    rq = rqm = "";
    sprintf(buf, "%d", Pennies(thing));
    cr = buf;
    crm = "  Credits: ";

    if ( Typeof(thing) == TYPE_PLAYER )
    {
        if ( Robot(thing) )
        {
            cr = crm = "";
        }
        else
        {
            if (inf_mon(thing))
            {
                cr = "INFINITE";
            }

            rqm = "  Quota-Left: ";
            rq = atr_get(thing, A_RQUOTA);

            if ( atoi(rq) <= 0 )
            {
	            rq = "NONE";
            }
        }
    }

    send_message(player, "Owner: %s%s%s%s%s", db[db[thing].owner].name, crm, cr, rqm, rq);
    send_message(player, flag_description(thing));

    if (db[thing].zone != NOTHING)
    {
        send_message(player, "Zone: %s", unparse_object(player, db[thing].zone));
    }
    send_message(player, "Created: %s", db[thing].create_time ? mktm(db[thing].create_time, "D", player) : "never");

    send_message(player, "Modified: %s", db[thing].mod_time ? mktm(db[thing].mod_time, "D", player) : "never");

    if (db[thing].parents && *db[thing].parents != NOTHING)
    {
        char obuf[1000], buf[1000];
        int i;

        strcpy(obuf, "Parents:");
        for (i = 0; db[thing].parents[i] != NOTHING; i++)
        {
            sprintf(buf, " %s", unparse_object(player, db[thing].parents[i]));
            if (strlen(buf) + strlen(obuf) > 900)
            {
                send_message(player, obuf);
                strcpy(obuf, buf+1);
            }
            else
            {
                strcat(obuf, buf);
            }
        }
        send_message(player, obuf);
    }

    if (db[thing].atrdefs)
    {
        ATRDEF *k;

        send_message(player, "Attribute definitions:");
        for (k = db[thing].atrdefs; k; k = k->next)
        {
            char buf[1000];
            sprintf(buf, "  %s%s%c", k->a.name, (k->a.flags & AF_FUNC) ? "()" : "", (k->a.flags)?':':'\0');

            if (k->a.flags & AF_WIZARD)
            {
                strcat(buf, " wizard");
            }
            if (k->a.flags & AF_UNIMP)
            {
                strcat(buf, " unsaved");
            }
            if (k->a.flags & AF_OSEE)
            {
	            strcat(buf, " osee");
            }
            if (k->a.flags & AF_INHERIT)
            {
                strcat(buf, " inherit");
            }
            if (k->a.flags & AF_DARK)
            {
                strcat(buf, " dark");
            }
            if (k->a.flags & AF_DATE)
            {
                strcat(buf, " date");
            }
            if (k->a.flags & AF_LOCK)
            {
                strcat(buf, " lock");
            }
            if (k->a.flags & AF_FUNC)
            {
                strcat(buf, " function");
            }
            if (k->a.flags & AF_DBREF)
            {
                strcat(buf, " dbref");
            }
            if (k->a.flags & AF_HAVEN)
            {
                strcat(buf, " haven");
            }

            send_message(player, buf);
        }
    }

    look_atrs(player,thing, *arg2);

    // show him the contents 
    if(db[thing].contents != NOTHING)
    {
        look_contents(player, thing, "Contents:");
    }

    fflush(stdout);
    switch(Typeof(thing))
    {
        case TYPE_ROOM:
            // tell him about entrances 
            for (enter = 0; enter < db_top; enter++)
            {
                if (Typeof(enter) == TYPE_EXIT && db[enter].link == thing)
                {
                    if (pos == 0)
                    {
                        send_message(player, "Entrances:");
                    }
                    pos = 1;
                    send_message(player, unparse_object(player, enter));
                }
            }
            if (pos == 0)
            {
                send_message(player, "No Entrances.");
            }

            // tell him about exits
            if (Exits(thing) != NOTHING)
            {
                send_message(player, "Exits:");

                DOLIST(exit, Exits(thing))
                {
                    send_message(player, unparse_object(player, exit));
                }
            }
            else
            {
                send_message(player, "No exits.");
            }

            // print dropto if present 
            if(db[thing].link != NOTHING)
            {
                send_message(player, "Dropped objects go to: %s", unparse_object(player, db[thing].link));
            }
            break;
        case TYPE_THING:
        case TYPE_PLAYER:
            // print home 
            send_message(player, "Home: %s", unparse_object(player, db[thing].link)); // home 
            // print location if player can link to it 
            if ( (db[thing].location != NOTHING)
              && (controls(player, db[thing].location,POW_EXAMINE) || controls(player,thing,POW_EXAMINE) || can_link_to(player, db[thing].location, POW_EXAMINE))
               )
            {
                send_message(player, "Location: %s", unparse_object(player, db[thing].location));
            }
            if (Typeof(thing) == TYPE_THING)
            {
                for (enter = 0; enter < db_top; enter++)
                {
                    if(Typeof(enter) == TYPE_EXIT && db[enter].link == thing)
                    {
                        if (pos == 0)
                        {
                            send_message(player, "Entrances:");
                        }
                        pos = 1;
                        send_message(player, unparse_object(player, enter));
                    }
                }
            }
            if (Typeof(thing) == TYPE_THING && db[thing].exits != NOTHING)
            {
                send_message(player, "Exits:");

                DOLIST(exit, db[thing].exits)
                {
                    send_message(player, unparse_object(player, exit));
                }
            }
            break;
        case TYPE_EXIT:
            // Print source 
            send_message(player, "Source: %s", unparse_object(player, db[thing].location));

            // print destination 
            switch(db[thing].link)
            {
                case NOTHING:
                    break;
                case HOME:
                    send_message(player, "Destination: *HOME*");
                    break;
                default:
                    send_message(player, "Destination: %s", unparse_object(player, db[thing].link));
                    break;
            }
            break;
        default:
            // do nothing
            break;
    }
}

void do_score(dbref player)
{
    send_message(player, "You have %d %s.",
		 Pennies(player),
		 Pennies(player) == 1 ? "Credit" : "Credits");
}

void do_inventory(dbref player)
{
    dbref thing;

    if ((thing = db[player].contents) == NOTHING)
    {
        send_message(player, "You aren't carrying anything.");
    }
    else
    {
        send_message(player, "You are carrying:");

        DOLIST(thing, thing)
        {
            send_message(player, "%s %s", unparse_object(player, thing), atr_get(thing, A_CAPTION));
        }
    }

    do_score(player);
}

void do_find(dbref player,char *name)
{
    dbref i;

    if (!payfor(player, find_cost))
    {
        send_message(player, "You don't have enough Credits.");
    }
    else
    {
        for (i = 0; i < db_top; i++)
        {
            if ((Typeof(i) != TYPE_EXIT)
             && (power(player,POW_EXAMINE) || db[i].owner==db[player].owner)
             && (!*name || string_match(db[i].name, name))
               )
            {
                send_message(player, unparse_object(player, i));
            }
        }

        send_message(player, "***End of List***");
    }
}

static void print_atr_match(dbref thing, dbref player, char *str)
{
    struct all_atr_list *ptr;

    for (ptr = all_attributes(thing); ptr; ptr = ptr->next)
    {
        if ((ptr->type!=0) && !(ptr->type->flags & AF_LOCK) && ((*ptr->value=='!') || (*ptr->value=='$')))
        {
            // decode it 
            char buff[BUF_SIZE];
            char *s;

            strncpy(buff,(ptr->value),BUF_SIZE);

            // search for first un escaped : 

            for (s = buff+1; *s && (*s!=':'); s++) ;

            if (!*s)
            {
                continue;
            }

            *s++=0;

            if (wild_match(buff+1,str))
            {
                if (controls(player, thing, POW_SEEATR))
                {
                    send_message(player, " %s/%s: %s", unparse_object(player, thing), unparse_attr(ptr->type, ptr->numinherit), buff+1);
                }
                else
                {
                    send_message(player, " %s", unparse_object(player, thing));
                }
            }
        }
    }
}

// check the current location for bugs 
void do_sweep(dbref player, char *arg1)
{
    dbref zon;

    if (*arg1)
    {
        dbref i;
        send_message(player, "All places that respond to %s:",arg1);

        for (i = db[db[player].location].contents; i != NOTHING; i=db[i].next)
        {
            if (Typeof(i) != TYPE_PLAYER || i == player)
            {
          	print_atr_match(i, player, arg1);
            }
        }
        for (i=db[player].contents; i != NOTHING; i=db[i].next)
        {
            if (Typeof(i) != TYPE_PLAYER || i == player)
            {
                print_atr_match(i, player, arg1);
            }
        }

        print_atr_match(db[player].location, player, arg1);

        for (i = db[db[player].location].exits; i != NOTHING; i=db[i].next)
        {
            if (Typeof(i) != TYPE_PLAYER || i == player)
            {
              	print_atr_match(i, player, arg1);
            }
        }

        print_atr_match(db[player].zone, player, arg1);

        if (db[player].zone != db[0].zone)
        {
            print_atr_match(db[0].zone, player, arg1);
        }
    }
    else
    {
        dbref here = db[player].location;

        dbref test;
        test = here;

        if (here == NOTHING)
        {
            return;
        }

        if (Dark(here))
        {
            send_message(player, "Sorry it is dark here; you can't search for bugs");
            return;
        }

        send_message(player, "Sweeping...");

        DOZONE(zon, player)
        {
            if (Hearer(zon))
            {
          	send_message(player, "Zone:");
          	break;
            }
        }

        DOZONE(zon, player)
        {
            if (Hearer(zon))
            {
                send_message(player, "Zone:");
                send_message(player, "  %s =%s.",db[zon].name, eval_sweep(db[here].zone));
            }
        }
			     
        if (Hearer(here))
        {
            send_message(player, "Room:");
            send_message(player, "  %s =%s.",db[here].name, eval_sweep(here));
        }

        for (test = db[test].contents; test != NOTHING; test = db[test].next)
        {
            if (Hearer(test))
            {
                send_message(player, "Contents:");
                break;
            }
        }

        test = here;

        for (test = db[test].contents; test != NOTHING; test = db[test].next)
        {
            if (Hearer(test))
            {
                send_message(player, "  %s =%s.",db[test].name, eval_sweep(test));
            }
        }

        test = here;

        for(test = db[test].exits; test != NOTHING; test = db[test].next)
        {
            if (Hearer(test))
            {
                send_message(player, "Exits:");
                break;
            }
        }

        test = here;

        for (test = db[test].exits; test != NOTHING; test = db[test].next)
        {
            if (Hearer(test))
            {
                send_message(player, "  %s =%s.",db[test].name, eval_sweep(test));
            }
        }

        for (test = db[player].contents; test != NOTHING; test = db[test].next)
        {
            if (Hearer(test))
            {
                send_message(player, "Inventory:");
                break;
            }
        }

        for (test=db[player].contents;test!=NOTHING;test=db[test].next)
        {
            if (Hearer(test))
            {
                send_message(player, "  %s =%s.",db[test].name, eval_sweep(test));
            }
        }

        send_message(player, "Done.");
    }
}

void do_whereis(dbref player, char *name)
{
    dbref thing;

    if(*name == '\0')
    {
        send_message(player, "You must specify a valid player name.");
        return;
    }

    if ((thing = lookup_player(name)) == NOTHING)
    {
        send_message(player, "%s does not seem to exist.", name);
        return;
    }

    /*
    init_match(player, name, TYPE_PLAYER);
    match_player();
    match_exit();
    match_neighbor();
    match_possession();
    match_absolute();
    match_me();
    */

    if (db[thing].flags & DARK)
    {
        send_message(player, "%s wishes to have some privacy.",db[thing].name);

        if (!could_doit(player,thing,A_LPAGE))
        {
            send_message(thing, "%s tried to locate you and failed.", unparse_object(thing,player));
        }
		    
        return;
    }

    send_message(player, "%s is at: %s.", db[thing].name, unparse_object(player,db[thing].location));

    if (!could_doit(player,thing,A_LPAGE))
    {
        send_message(thing, "%s has just located your position.", unparse_object(thing,player));
    }

    return;

}

void do_laston(dbref player, char *name)
{
    char *attr;
    dbref thing;
    long tim;

    if (*name == '\0')
    {
        send_message(player, "You must specify a valid player name.");
        return;
    }

    if ((thing = lookup_player(name)) == NOTHING || Typeof(thing)!=TYPE_PLAYER)
    {
        send_message(player, "%s does not seem to exist.",name);
        return;
    }

    attr = atr_get(thing, A_LASTCONN);
    tim = atol(attr);

    if (!tim)
    {
        send_message(player, "%s has never logged on.", db[thing].name);
    }
    else
    {
        send_message(player, "%s was last logged on: %s.", db[thing].name, mktm(tim, "D", player));
    }

    attr = atr_get(thing, A_LASTDISC);
    tim = atol(attr);

    if (tim)
    {
        send_message(player, "%s last logged off at: %s.", db[thing].name, mktm(tim, "D", player));
    }

    return;
}

static char *eval_sweep(dbref thing)
{
    static char sweep_str[30];

    strcpy(sweep_str, "\0");

    if (Live_Player(thing)) strcat(sweep_str, " player");
    if (Live_Puppet(thing)) strcat(sweep_str, " puppet");
    if (Commer(thing))      strcat(sweep_str, " commands");
    if (Listener(thing))    strcat(sweep_str, " messages");

    return sweep_str;
}
