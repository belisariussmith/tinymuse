// $Header: /u/cvsroot/muse/src/db/inherit.c,v 1.6 1993/03/21 00:49:57 nils Exp $ 

#include <stdio.h>
#include "tinymuse.h"
#include "externs.h"
#include "config.h"
#include "db.h"

void put_atrdefs(FILE *file, ATRDEF *defs)
{
    extern void putstring P((FILE *, char *));

    for (; defs; defs = defs->next)
    {
        fputc('/', file);

        putref(file, defs->a.flags);
        putref(file, defs->a.obj);

        putstring(file, defs->a.name);
    }

  fputs("\\\n", file);
}

///////////////////////////////////////////////////////////////////////////////
// get_atrdefs()
///////////////////////////////////////////////////////////////////////////////
//     Used when reading in an object from the database. This typically reads
// in three lines, the first being the attribute flags, the object id, and then
// the name.
//     Continues reading in atrdefs until it catches the termination char '\'
///////////////////////////////////////////////////////////////////////////////
ATRDEF *get_atrdefs(FILE *file, ATRDEF *olddefs)
{
    extern char *getstring_noalloc P((FILE *));
    char k;
    ATRDEF *ourdefs = NULL;
    ATRDEF *endptr  = NULL;
    int attribute   = 0;
  
    for (;;)
    {
        k = fgetc(file);

        switch (k)
        {
            case '\\':
                if (fgetc(file) != '\n')
                {
                    log_error ("No atrdef newline.");
                }

                return ourdefs;
                break;
            case '/':
                // An ATRDEF

                // Get the attribute first 
                attribute = getref(file);
                // Is it a valid one?
                if (attribute > 8191)
                {
                    // Invalid attribute
                    attribute = getref(file);  // Read to ignore
                    attribute = getref(file);  // Read to ignore

                    // Hop to next one
                    break;
                }

                if (!endptr)
                {
                    if (olddefs)
                    {
                        // Previous atrdefs defined, so add to linked list
                        endptr  = ourdefs = olddefs;
                        olddefs = olddefs->next;
                    }
                    else
                    {
                        // No previous atrdef defined, so allocate for a new one
                        endptr = ourdefs = malloc( sizeof(ATRDEF));
                    }
                }
                else
                {
                    if (olddefs)
                    {
                        endptr->next = olddefs;
                        olddefs = olddefs->next;
                        endptr = endptr->next;
                    }
                    else
                    {
                        // No atrdef defined for next in linked list, so allocate for a new one
                        endptr->next = malloc( sizeof(ATRDEF));
                        endptr = endptr->next;
                    }
                }

                endptr->a.flags = attribute;
                endptr->a.obj   = getref(file);
                endptr->next    = NULL;
                endptr->a.name  = NULL;

                SET (endptr->a.name, getstring_noalloc(file));
                break;
            default:
                log_error ("Illegal character in get_atrdefs");
                return 0;
        }
    }
}

static void remove_attribute(dbref obj, ATTR *atr)
{
    int i;
    atr_add (obj, atr, "");

    for (i = 0; db[obj].children && db[obj].children[i] != NOTHING; i++)
    {
        remove_attribute (db[obj].children[i], atr);
    }
}

void do_undefattr(dbref player, char *arg1)
{
    dbref obj;
    ATTR *atr;
    ATRDEF *d;
    ATRDEF *prev = NULL;
  
    if (!parse_attrib(player, arg1, &obj, &atr, POW_MODIFY))
    {
        send_message(player, "No match.");
        return;
    }

    for (d = db[obj].atrdefs; d; prev = d, d = d->next)
    {
        if (&(d->a) == atr)
        {
            if (prev)
            {
	        prev->next = d->next;
            }
            else
            {
                db[obj].atrdefs = d->next;
            }

            remove_attribute (obj, atr);

            if (0 == --d->a.refcount)
            {
                // this should always happen, but... 
                free(d->a.name);
                free(d);
            }

            send_message(player, "Deleted.");
            return;
        }
    }

    send_message(player, "No match.");
}

void do_defattr(dbref player, char *arg1, char *arg2)
{
    ATTR *atr;
    char *flag_parse;
    int atr_flags = 0;
    dbref thing;
    char *attribute;
    int i;

    if (!(attribute = strchr(arg1, '/')))
    {
        send_message(player, "no match");
        return;
    }
    *(attribute++) = '\0';
  
    thing = match_controlled(player, arg1, POW_MODIFY);

    if (thing == NOTHING)
    {
        return;
    }

    if (!ok_attribute_name (attribute))
    {
        send_message(player,"Illegal attribute name.");
        return;
    }
  
    while ((flag_parse = parse_up(&arg2,' ')))
    {
        if (flag_parse == NULL)
        {
            // empty if, so we can do else(s): 
        }
        else if (!string_compare(flag_parse, "wizard"))
        {
            atr_flags |= AF_WIZARD;
        }
        else if (!string_compare(flag_parse, "osee"))
        {
            atr_flags |= AF_OSEE;
        }
        else if (!string_compare(flag_parse, "dark"))
        {
            atr_flags |= AF_DARK;
        }
        else if (!string_compare(flag_parse, "inherit"))
        {
            atr_flags |= AF_INHERIT;
        }
        else if (!string_compare(flag_parse, "unsaved"))
        {
            atr_flags |= AF_UNIMP;
        }
        else if (!string_compare(flag_parse, "date"))
        {
            atr_flags |= AF_DATE;
        }
        else if (!string_compare(flag_parse, "lock"))
        {
            atr_flags |= AF_LOCK;
        }
        else if (!string_compare(flag_parse, "function"))
        {
            atr_flags |= AF_FUNC;
        }
        else if (!string_compare(flag_parse, "dbref"))
        {
            atr_flags |= AF_DBREF;
        }
        else if (!string_compare(flag_parse, "haven"))
        {
            atr_flags |= AF_HAVEN;
        }
        else
        {
            send_message(player, "Unknown attribute option: %s",flag_parse);
        }
    }
  
    atr = atr_str (thing, thing, attribute);

    if (atr && atr->obj == thing)
    {
        atr->flags = atr_flags;
        send_message(player, "Options set.");
        return;
    }
    {
        ATRDEF *attributes;
    
        for (attributes = db[thing].atrdefs, i = 0; attributes; attributes = attributes->next, i++);

        if (i > 90 && !power(player, POW_SECURITY))
        {
            send_message(player, "Sorry, you can't have that many attribute defs on an object.");
            return;
        }
        if (atr)
        {
            send_message(player, "Sorry, attribute shadows a builtin attribute or one on a parent.");
            return;
        }

        // Instantiate / Initialize the attributes
        attributes = malloc( sizeof(ATRDEF));
        attributes->a.name     = NULL;
        SET (attributes->a.name, attribute);
        attributes->a.flags    = atr_flags;
        attributes->a.obj      = thing;
        attributes->a.refcount = 1;
        attributes->next = db[thing].atrdefs;

        // Now assign it
        db[thing].atrdefs = attributes;
    }

    send_message(player, "Attribute defined.");
}


static int is_a_internal(dbref thing, dbref parent, int dep)
{
    if (thing == parent)
    {
        return TRUE;
    }

    if (dep < 0)
    {
        return TRUE;
    }

    if (!db[thing].parents)
    {
        return FALSE;
    }

    for (int j = 0; db[thing].parents[j] != NOTHING; j++)
    {
        if (is_a_internal (db[thing].parents[j], parent, dep-1))
        {
            return TRUE;
        }
    }

    return FALSE;
}

int is_a(dbref thing, dbref parent)
{
    if (thing == NOTHING)
    {
        return TRUE;
    }

    return (is_a_internal(thing, parent, 20)); // 20 is the max depth 
}

void do_delparent(dbref player, char *arg1, char *arg2)
{
    dbref thing;
    dbref parent;
    int can_doit = FALSE;

    thing = match_controlled(player, arg1 , POW_MODIFY);

    if (thing == NOTHING)
    {
        return;
    }

    mark_hearing(thing);
    parent = match_thing (player, arg2);

    if (parent == NOTHING)
    {
        return;
    }

    if (!(db[parent].flags&BEARING) && !controls(player,parent,POW_MODIFY))
    {
        send_message(player, "Sorry, you can't unparent from that.");
        can_doit = TRUE;
    }
      
  
    for (int i = 0; db[thing].parents && db[thing].parents[i] != NOTHING; i++)
    {
        if (db[thing].parents[i] == parent)
        {
            can_doit |= 2;
        }
    }
  
    if (!(can_doit&2))
    {
        send_message(player, "Sorry, it doesn't have that as its parent.");
    }

    if (can_doit != 2)
    {
        return;
    }

    REMOVE_FIRST_L (db[thing].parents, parent);
    REMOVE_FIRST_L (db[parent].children, thing);
    send_message(player, "%s is no longer a parent of %s.", unparse_object_a (player, parent), unparse_object_a (player, thing));
			
    check_hearing();

    did_it(thing, parent, A_UNPARENT, NULL, A_OUNPARENT, NULL, A_AUNPARENT);
    did_it(player, parent, A_UNPARENT, NULL, A_OUNPARENT, NULL, A_AUNPARENT);
}

void do_addparent(dbref player, char *arg1, char *arg2)
{
    dbref thing;
    dbref parent;
    int i;
    int can_doit = FALSE;

    thing = match_controlled(player, arg1 , POW_MODIFY);

    if (thing == NOTHING)
    {
        return;
    }

    mark_hearing(thing);
    parent = match_thing(player, arg2);

    if (parent == NOTHING)
    {
        return;
    }

    if (is_a(parent, thing))
    {
        send_message(player, "but %s is a descendant of %s!", unparse_object_a (player, parent), unparse_object_a (player, thing));
        can_doit |= 4;
    }

    if (!(db[parent].flags&BEARING) && !controls(player,parent,POW_MODIFY))
    {
        send_message(player, "Sorry, you can't parent to that.");
        can_doit |= 1;
    }
      
  
    for (i = 0; db[thing].parents && db[thing].parents[i] != NOTHING; i++)
    {
        if (db[thing].parents[i] == parent)
        {
      	    can_doit |= 2;
        }
    }
  
    if (can_doit & 2)
    {
        send_message(player, "Sorry, it already has that as its parent.");
    }

    if (can_doit != 0)
    {
        return;
    }

    PUSH_L (db[thing].parents, parent);
    PUSH_L (db[parent].children, thing);

    send_message(player, "%s is now a parent of %s.", unparse_object_a (player, parent), unparse_object_a (player, thing));

    check_hearing();

    did_it(thing, parent, A_PARENT, NULL, A_OPARENT, NULL, A_APARENT);
    did_it(player, parent, A_PARENT, NULL, A_OPARENT, NULL, A_APARENT);
}

