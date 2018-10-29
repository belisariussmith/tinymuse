///////////////////////////////////////////////////////////////////////////////
// unparse.c 
///////////////////////////////////////////////////////////////////////////////
// $Id: unparse.c,v 1.7 1993/09/18 19:04:14 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "db.h"

#include "externs.h"
#include "config.h"
#include "interface.h"

char *unparse_flags(dbref thing)
{
    static char buf[MAX_CMD_BUF];
    int c;
    char *p;
    char *type_codes = "RTE-----PPPPPPPP";

    p = buf;
    c = type_codes[Typeof(thing)];

    if ( c != '-' )
    {
        *p++ = c;
    }

    if (db[thing].flags & ~TYPE_MASK)
    {
        // print flags 
        if(db[thing].flags & GOING)
        {
            p    = buf;
            *p++ = 'G';
        }
        if(db[thing].flags & PUPPET)
        {
            *p++ = 'p';
        }
        if(db[thing].flags & STICKY)
        {
            *p++ = 'S';
        }
        if(db[thing].flags & DARK)
        {
            *p++ = 'D';
        }
        if(db[thing].flags & LINK_OK)
        {
            *p++ = 'L';
        }
        if(db[thing].flags & HAVEN)
        {
            *p++ = 'H';
        }
        if(db[thing].flags & CHOWN_OK)
        {
            *p++='C';
        }
        if(db[thing].flags & ENTER_OK)
        {
            *p++='e';
        }
        if(db[thing].flags & SEE_OK)
        {
            *p++='v';
        }
        // if(db[thing].flags & UNIVERSAL) *p++='U';
        if(db[thing].flags & OPAQUE)
        {
            if (Typeof(thing) == TYPE_EXIT)
            {
                *p++ = 'T';
            }
            else
            {
                *p++='o';
            }
        }
        if(db[thing].flags & INHERIT_POWERS)
        {
            *p++='I';
        }
        if(db[thing].flags & QUIET)
        {
            *p++='q';
        }
        if(db[thing].flags & BEARING)
        {
            *p++='b';
        }
        if (db[thing].flags & CONNECT)
        {
            *p++='c';
        }

        switch(Typeof(thing))
        {
            case TYPE_PLAYER:
                if (db[thing].flags & PLAYER_SLAVE)
                    *p++='s';
                if (db[thing].flags & PLAYER_TERSE)
                    *p++='t';
                if (db[thing].flags & PLAYER_ANSI)
                    *p++='@';
                if (db[thing].flags & PLAYER_MORTAL)
                    *p++='m';
                if (db[thing].flags & PLAYER_NEWBIE)
                    *p++='n';
                if (db[thing].flags & PLAYER_NO_WALLS)
                    *p++='N';
                if (db[thing].flags & PLAYER_NO_ANN)
                    *p++='a';
                if (db[thing].flags & PLAYER_NO_COM)
                    *p++='+';
                break;
            case TYPE_EXIT:
                if(db[thing].flags & EXIT_LIGHT)
                    *p++='l';
                break;
            case TYPE_THING:
                if (db[thing].flags & THING_KEY)
                    *p++='K';
                if (db[thing].flags & THING_DEST_OK)
                    *p++='d';
                if (db[thing].flags & THING_SACROK)
                    *p++='X';
                if (db[thing].flags & THING_LIGHT)
                    *p++='l';
                if (db[thing].flags & THING_DIG_OK)
                    *p++='!';
                break;
            case TYPE_ROOM:
                if (db[thing].flags & ROOM_JUMP_OK)
                    *p++='J';
                if (db[thing].flags & ROOM_AUDITORIUM)
                    *p++='A';
                if (db[thing].flags & ROOM_FLOATING)
                    *p++='f';
                if (db[thing].flags & ROOM_DIG_OK)
                    *p++='!';
                break;
        }
    }

    *p = '\0';

    return buf;
}

char *unparse_object_a(dbref player, dbref loc)
{
    char *xx;
    char *zz = unparse_object(player, loc);

    xx = newglurp(strlen(zz) + 1);
    strcpy(xx, zz);

    return xx;
}

char *unparse_object(dbref player, dbref loc)
{
    static char buf[MAX_CMD_BUF];
    dbref flags;

    switch(loc)
    {
        case NOTHING:
            return "*NOTHING*";
        case HOME:
            return "*HOME*";
        default:
            if (loc < 0 || loc >= db_top)
            {
                sprintf(buf, "<invalid #%d>", loc);
                return buf;
            }
            else if ((IS(player, TYPE_PLAYER, PLAYER_NEWBIE))
                    ?(db[loc].owner == player)
                    :(controls(player, loc, POW_BACKSTAGE) ||
                      can_link_to(player, loc, POW_BACKSTAGE) ||
                      (IS(loc, TYPE_ROOM, ROOM_JUMP_OK)) ||
                      (db[loc].flags & CHOWN_OK) ||
                      (db[loc].flags & SEE_OK) ||
                      power(player, POW_BACKSTAGE)))
            {
                // show everything 
                flags = db[loc].flags;

                if (!controls(player, loc, POW_WHO) && !could_doit(player, loc, A_LHIDE))
                {
                    db[loc].flags &= ~(CONNECT);
                }
       
                sprintf(buf, "%s <#%d%s>", db[loc].name, loc, unparse_flags(loc));

                db[loc].flags = flags;
                return buf;
            }
            else
            {
                // show only the name 
                return db[loc].name;
            }
    }
}
