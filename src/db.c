// db.c 
// $Id: db.c,v 1.34 1994/02/18 22:40:43 nils Exp $ 
#include <stdio.h>
#include <ctype.h>

#include "tinymuse.h"
#include "credits.h"
#include "db.h"
#include "config.h"
#include "externs.h"
#include "interface.h"
#include "hash.h"

#define DB_MSGLEN 1040

#define DB_LOGICAL 0x15

#define TYPE_GUEST 0x8
#define TYPE_TRIALPL 0x9
#define TYPE_MEMBER 0xA
#define TYPE_ADMIN 0xE
#define TYPE_DIRECTOR 0xF

//#define getstring(x) alloc_string(getstring_noalloc(x))
#define getstring(x,p) {p = NULL; SET(p, getstring_noalloc(x));}

#define getstring_compress(x,p) getstring(x,p)

// External declarations
extern int user_limit;
extern int restrict_connect_class;

// Globals
struct object *db = 0;
dbref db_top = 0;
#ifdef TEST_MALLOC
int malloc_count = 0;
#endif // TEST_MALLOC 

dbref update_bytes_counter = (-1);
dbref db_size  = 100;
int db_version = 1;  // database version defaults to 1 
int db_init    = 0;
int db_loaded  = FALSE;

int dozonetemp;
char *b;

// single attribute cache 
static dbref atr_obj= NOTHING;
static ATTR *atr_atr= NULL;
static char *atr_p;

extern char ccom[];
static ALIST *AL_MAKE P((ATTR *, ALIST *, char *));
static dbref *getlist P((FILE *));
//static void putlist P((FILE *, dbref *));
static void putlist(FILE *f, dbref *list);
static char *atr_fgets P((char *, int, FILE *));

static int db_read_object P((dbref, FILE *));

static FILE *db_read_file = NULL;

ALIST *AL_MAKE();

void putstring P((FILE *, char *));

struct builtinattr {
  ATTR definition;
  int number;
};

struct builtinattr attr[] = {
#define DOATTR(var, name, flags, num) \
  {{name, flags, NOTHING, 1}, num},
#include "attrib.h"
#undef DOATTR
  {{0,        0                ,0,0}     , 0}

};
#define NUM_BUILTIN_ATTRS ((sizeof(attr)/sizeof(struct builtinattr))-1)
#define MAX_ATTRNUM 512 // less than this, but this will do. 

static ATTR *builtin_atr(int num)
{
    static int initted = 0;
    static ATTR *numtable[MAX_ATTRNUM];

    if (!initted)
    {
        // we need to init. 
        int i;

        initted = 1;

        for (i = 0; i < MAX_ATTRNUM; i++)
        {
            int a = 0;

            while ((a < NUM_BUILTIN_ATTRS)&&(attr[a].number != i))
            {
                a++;
            }

            if (a < NUM_BUILTIN_ATTRS)
            {
                numtable[i] = &(attr[a].definition);
            }
            else
            {
                numtable[i] = NULL;
            }
        }
    }

    if (num >= 0 && num < MAX_ATTRNUM)
    {
        return numtable[num];
    }
    else
    {
        return NULL;
    }
}

static char *attr_disp(void *atr)
{
    //char buf[BUF_SIZE];   // cannot return address of local variable
    char *buf = malloc(BUF_SIZE); // Needs to be free()'d later

    sprintf(buf, "#%d, flags #%d", ((struct builtinattr *)atr)->number, ((struct builtinattr *)atr)->definition.flags);

    return buf; 
}

ATTR *builtin_atr_str(char *str)
{
    static struct hashtab *attrhash = NULL;

    if (!attrhash)
    {
        // should modify make_hashtab to free() what attr_disp returns -- Belisarius
        attrhash = make_hashtab(207, attr, sizeof(struct builtinattr), "attr", attr_disp);
    }

    //  for (a = 0; a < NUM_BUILTIN_ATTRS; a++)
    //  {
    //      if (!string_compare (str, attr[a].definition.name))
    //      {
    //          return &(attr[a].definition);
    //      }
    //  }
    //  return NULL;

    return &((struct builtinattr *)lookup_hash (attrhash, hash_name(str), str))->definition;
}

static ATTR *atr_defined_on_str (dbref obj, char *str)
{
    ATRDEF *k;

    if (obj < 0 || obj >= db_top)
    {
        // sanity check 
        return NULL;
    }

    for (k = db[obj].atrdefs; k; k = k->next)
    {
        if (!string_compare (k->a.name, str))
        {
            return &(k->a);
        }
    }

    return NULL;
}

static ATTR *atr_find_def_str(dbref obj, char *str)
{
    ATTR *k;
    int i;

    if ((k = atr_defined_on_str (obj, str)))
    {
        return k;
    }

    for (i = 0; db[obj].parents && db[obj].parents[i] != NOTHING; i++)
    {
        if ((k = atr_find_def_str (db[obj].parents[i], str)))
        {
            return k;
        }
    }

    return NULL;
}

ATTR *atr_str(dbref player, dbref obj, char *s)
{
    ATTR *a;
    char *t;

    if ((t = strchr(s,'.')))
    {
        // definately a user defined attribute. 
        dbref onobj;
        static char mybuf[BUF_SIZE];

        if (t == s)
        {
            // first thing too! make it global. 
            return builtin_atr_str(s+1);
        }

        strcpy(mybuf, s);
        mybuf[t-s] = '\0';
        init_match (player, mybuf, NOTYPE);
        match_everything ();
        onobj = match_result();

        if (onobj == AMBIGUOUS)
        {
            onobj = NOTHING;
        }

        if (onobj != NOTHING)
        {

            a = atr_defined_on_str(onobj, t+1);

            //if (is_a(player, a->obj))
            //{
            //    return a;
            //}

            if (a)
            {
                return a;
            }
        }
    }

    if (player != NOTHING)
    {
        a = atr_find_def_str (player, s);

        if (a && is_a(obj, a->obj))
        {
            return a;
        }
    }

    a = builtin_atr_str (s);

    if (a)
    {
        return a;
    }

    a = atr_find_def_str (obj, s);

    // if everything else fails, use the one on the obj.
    return a;                     
}

// routines to handle object attribute lists 

// clear an attribute in the list 
void atr_clr(dbref thing, ATTR *atr)
{
    ALIST *ptr = db[thing].list;

    atr_obj = NOTHING;

    while(ptr)
    {
        if (AL_TYPE(ptr) == atr)
        {
            if (AL_TYPE(ptr)) unref_atr(AL_TYPE(ptr));
            {
                AL_DISPOSE(ptr);
            }
            return;
        }
        ptr = AL_NEXT(ptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
// atr_add()
///////////////////////////////////////////////////////////////////////////////
// Add attribute of type atr to list 
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void atr_add(dbref thing, ATTR *atr, char *str)
{
    ALIST *ptr;
    char *d;

    if (!(atr->flags & AF_NOMEM))
    {
        db[thing].i_flags |= I_UPDATEBYTES;
    }

    if (!str)
    {
        str = "";
    }

    if (atr == NULL)
    {
        return;
    }

    for (ptr = db[thing].list; ptr && (AL_TYPE(ptr) != atr); ptr = AL_NEXT(ptr))
    ;

    if (!*str)
    {
        if (ptr)
        {
            unref_atr(AL_TYPE(ptr));
            AL_DISPOSE(ptr);
        }
        else
        {
            return;
        }
    }

    if (!ptr || (strlen(str) > strlen(d = (char *)AL_STR(ptr))))
    {
        // allocate room for the attribute
        if (ptr)
        {
            AL_DISPOSE(ptr);
            db[thing].list = AL_MAKE(atr, db[thing].list, str);
        }
        else
        {
            ref_atr(AL_TYPE((db[thing].list = AL_MAKE(atr, db[thing].list, str))));
        }
    }
    else
    {
        strcpy(d, str);
    }

    atr_obj = NOTHING;
}

// used internally by defines
static char *atr_get_internal(dbref thing, ATTR *atr)
{
    int i;
    ALIST *ptr;
    char *k;

    for (ptr = db[thing].list; ptr; ptr = AL_NEXT(ptr))
    {
        if (ptr && (AL_TYPE(ptr) == atr))
        {
            return ((char *)(AL_STR(ptr)));
        }
    }

    for (i = 0; db[thing].parents && db[thing].parents[i] != NOTHING; i++)
    {
        if ((k = atr_get_internal (db[thing].parents[i], atr)))
        {
            if (*k)
            {
                return k;
            }
        }
    }

    return "";
}

char *atr_get(dbref thing, ATTR *atr)
{
    char buf2[BUF_SIZE];
    ALIST *ptr;

    if (thing < 0 || thing >= db_top || !atr)
    {
        // sanity check
        return "";
    }

    if ((thing == atr_obj) && (atr == atr_atr))
    {
        return(atr_p);
    }

    atr_obj = thing;
    atr_atr = atr;

    if (atr->flags & AF_BUILTIN)
    {
        static char buf[BUF_SIZE];

        if (atr == A_LOCATION)
        {
            sprintf(buf, "#%d", db[thing].location);
        }
        else if (atr == A_OWNER)
        {
            sprintf(buf, "#%d", db[thing].owner);
        }
        else if (atr == A_LINK)
        {
            sprintf(buf, "#%d", db[thing].link);
        }
        else if (atr == A_PARENTS || atr == A_CHILDREN)
        {
            dbref *list;
            int i;
            *buf = '\0';
            list = (atr == A_PARENTS) ? db[thing].parents : db[thing].children;

            for (i = 0; list && list[i] != NOTHING; i++)
            {
                if (*buf)
                {
                    sprintf(buf+strlen(buf), " #%d", list[i]);
                }
                else
                {
                    sprintf(buf, "#%d", list[i]);
                }
            }
        }
        else if (atr == A_CONTENTS)
        {
            dbref it;
            *buf = '\0';

            for (it = db[thing].contents; it != NOTHING; it = db[it].next)
            {
                if (*buf)
                {
                    sprintf(buf2, "%s #%d", buf, it);
                    strcpy(buf, buf2);
                }
                else
                {
                    sprintf(buf, "#%d", it);
                }
            }
        }
        else if (atr == A_EXITS)
        {
            dbref it;
            *buf = '\0';

            for (it = db[thing].exits; it != NOTHING; it = db[it].next)
            {
                if (*buf)
                {
                    sprintf(buf2, "%s #%d", buf, it);
                    strcpy(buf, buf2);
                }
                else
                {
                    sprintf(buf, "#%d", it);
                }
            }
        }
        else if (atr == A_NAME)
        {
            strcpy(buf, db[thing].name);
        }
        else if (atr == A_FLAGS)
        {
            strcpy(buf, unparse_flags(thing));
        }
        else if (atr == A_ZONE)
        {
            sprintf(buf, "#%d", db[thing].zone);
        }
        else if (atr == A_NEXT)
        {
            sprintf(buf, "#%d", db[thing].next);
        }
        else if (atr == A_MODIFIED)
        {
            sprintf(buf, "%ld", db[thing].mod_time);
        }
        else if (atr == A_CREATED)
        {
            sprintf(buf, "%ld", db[thing].create_time);
        }
        else if (atr == A_LONGFLAGS)
        {
            sprintf(buf, "%s", flag_description(thing));
        }
        else
        {
            sprintf(buf, "???");
        }

        return atr_p = buf;
    }

    for (ptr = db[thing].list; ptr; ptr = AL_NEXT(ptr))
    {
        if (ptr && AL_TYPE(ptr) == atr)
        {
            return (atr_p = (char *)(AL_STR(ptr)));
        }
    }

    if (atr->flags & AF_INHERIT)
    {
        return (atr_p = atr_get_internal (thing, atr));
    }
    else
    {
        return (atr_p = "");
    }
}

void atr_free(dbref thing)
{
    ALIST *ptr,*next;

    if (thing < 0 || thing >= db_top)
    {
        // sanity check 
        return;
    }

    for (ptr = db[thing].list; ptr; ptr = next)
    {
        next = AL_NEXT(ptr);
        free(ptr);
    }

    db[thing].list = NULL;

    atr_obj = NOTHING;
}

static ALIST *AL_MAKE(ATTR *type, ALIST *next, char *string)
{
    ALIST *ptr;

    ptr = (ALIST *) malloc(sizeof(ALIST) + strlen(string)+1);
    AL_TYPE(ptr) = type;
    ptr->next = next;
    strcpy(AL_STR(ptr), string);

    return(ptr);
}

// garbage collect an attribute list 
void atr_collect(dbref thing)
{
    ALIST *ptr,*next;

    if (thing < 0 || thing >= db_top)
    {
        // sanity check 
        return;
    }

    ptr = db[thing].list;
    db[thing].list = NULL;

    while(ptr)
    {
        if (AL_TYPE(ptr))
        {
            db[thing].list = AL_MAKE(AL_TYPE(ptr), db[thing].list, AL_STR(ptr));
        }

        next = AL_NEXT(ptr);
        free(ptr);
        ptr = next;
    }

    atr_obj = NOTHING;
}

void atr_cpy_noninh(dbref dest, dbref source)
{
    ALIST *ptr;

    ptr = db[source].list;
    db[dest].list = NULL;

    while(ptr)
    {
        if (AL_TYPE(ptr) && !(AL_TYPE(ptr)->flags & AF_INHERIT))
        {
            db[dest].list = AL_MAKE(AL_TYPE(ptr), db[dest].list, AL_STR(ptr));
            ref_atr(AL_TYPE(ptr));
        }

        ptr = AL_NEXT(ptr);
    }
}

static void db_grow(dbref newtop)
{
    struct object *newdb;

    // No reason to change the database size if the new size
    // is smaller than the current one!
    if (newtop > db_top)
    {
        // Check to see if the database even exists 
        if (!db)
        {
            // Instantiate the initial one 

            // Set the size
            db_size = (db_init) ? db_init : 100;

            // Check to see if we can allocate the memory
            if ((db = 5+(struct object *) malloc((db_size + 5) * sizeof(struct object))) == 0)
            {
                // Allocation not possible, abort
                abort();
            }

            // What are we scanning for?
            //memchr (db-5, 0, sizeof(struct object)*(db_size + 5));
        }

        // maybe grow it 
        if (db_top > db_size)
        {
            // make sure it's big enough 
            while (newtop > db_size)
            {
                db_size *= 2;
            }

            newdb = realloc(db-5, (5+db_size)*sizeof(struct object));

            if (!newdb)
            {
                abort();
            }

            newdb += 5;
            // What are we scanning for?
            //memchr((newdb + db_top), 0, sizeof(struct object)*(db_size - db_top));
            db = newdb;
        }

        db_top = newtop;
    }
}

dbref new_object()
{
    dbref newobj;
    struct object *db_obj;

    if ((newobj = free_get()) == NOTHING)
    {
        newobj = db_top;
        db_grow(db_top + 1);
    }

    // clear it out 
    db_obj = db + newobj;
    db_obj->name     = 0;
    db_obj->list     = 0;
    db_obj->location = NOTHING;
    db_obj->contents = NOTHING;
    db_obj->exits    = NOTHING;
    db_obj->parents  = NULL;
    db_obj->children = NULL;
    db_obj->link     = NOTHING;
    db_obj->next     = NOTHING;
    db_obj->owner    = NOTHING;
    // db_obj->penn    = 0;

    // Attribute initialization. Similar to the need for the db_init_object()
    // when reading objects in from the database. If the NULL assignment is
    // not completed then segementation fault is inevtiable.
    // is inevitable.
    db_obj->atrdefs  = NULL;
    // Do not allocate the memory for this yet!

    db_obj->mod_time     = 0;
    db_obj->create_time  = now;
    db_obj->zone         = NOTHING;
    db_obj->i_flags      = I_UPDATEBYTES;
    db_obj->size         = 0;

    return newobj;
}

void putref(FILE *file, dbref ref)
{
    fprintf(file, "%d\n", ref);
}

static int db_write_object(FILE *file, dbref i)
{
    struct object *db_obj;
    ALIST *list;

    db_obj = db + i;
    putstring(file, db_obj->name);
    putref(file, db_obj->location);
    putref(file, db_obj->zone);
    putref(file, db_obj->contents);
    putref(file, db_obj->exits);
    putref(file, db_obj->link);
    putref(file, db_obj->next);
    putref(file, db_obj->owner);
    //    putref(file, Pennies(i));
    putref(file, db_obj->flags);
    putref(file, db_obj->mod_time);
    putref(file, db_obj->create_time);

    if (Typeof(i) == TYPE_PLAYER)
    {
        put_powers(file,i);
    }

    // write the attribute list 
    for (list = db_obj->list; list; list = AL_NEXT(list))
    {
        if (AL_TYPE(list))
        {
            ATTR *x;
            x = AL_TYPE(list);

            if (x && !(x->flags&AF_UNIMP))
            {
                if (x->obj == NOTHING)
                {        //  builtin attribute. 
                    fputc('>',file);
                    putref(file, ((struct builtinattr *)x)->number); // kludgy. fix this. xxxx 
                    putref(file, NOTHING);
                }
                else
                {
                    ATRDEF *m;
                    int j;

                    fputc ('>', file);

                    for (m = db[AL_TYPE(list)->obj].atrdefs, j = 0; m; m = m->next, j++)
                    {
                        if ((&(m->a)) == AL_TYPE(list))
                        {
                            putref(file, j);
                            break;
                        }
                    }

                    if (!m)
                    {
                        putref(file, 0);
                        putref(file, NOTHING);
                    }
                    else
                    {
                        putref(file,AL_TYPE(list)->obj);
                    }
                }

                putstring(file, (AL_STR (list)));
            }
        }
    }

    fprintf(file, "<\n");
    putlist(file, db_obj->parents);
    putlist(file, db_obj->children);

    put_atrdefs(file, db_obj->atrdefs);

    return 0;
}

dbref db_write(FILE *file)
{
    dbref i;

    fprintf(file, "@%d\n", DB_VERSION);
    fprintf(file, "%d\n",  user_limit);
    fprintf(file, "%d\n",  restrict_connect_class);
    fprintf(file, "~%d\n", db_top);

    for (i = 0; i < db_top; i++)
    {
        fprintf(file, "&%d\n", i);
        db_write_object(file, i);
    }

    fputs("***END OF DUMP***\n", file);

    write_mail(file);
    fflush(file);

    return(db_top);
}

dbref parse_dbref(char *s)
{
    char *p;
    long x;

    x = atol(s);

    if (x > 0)
    {
        return x;
    }
    else if (x == 0)
    {
        // check for 0 
        for (p = s; *p; p++)
        {
            if (*p == '0')
            {
                return 0;
            }
            if (!isspace(*p))
            {
                break;
            }
        }
    }

    // else x < 0 or s != 0 
    return NOTHING;
}

/////////////////////////////////////////////////////////////////////
// getref()
/////////////////////////////////////////////////////////////////////
//     This function reads a line from the database file and converts
// it into an int and returns it.
/////////////////////////////////////////////////////////////////////
// See also: getstring_noalloc(), getstring()
/////////////////////////////////////////////////////////////////////
// Returns: (int)
/////////////////////////////////////////////////////////////////////
dbref getref(FILE *file)
{
    static char buf[DB_MSGLEN];

    fgets(buf, sizeof(buf), file);

    return(atoi(buf));
}

char *getstring_noalloc(FILE *file)
{
    static char buf[DB_MSGLEN];

    atr_fgets(buf, sizeof(buf), file);

    if (buf[strlen(buf) - 1] == '\n')
    {
        buf[strlen(buf) - 1] = '\0';
    }

    return buf;
}

/////////////////////////////////////////////////////////////////////
// db_init_object()
/////////////////////////////////////////////////////////////////////
//    Initializes an object
/////////////////////////////////////////////////////////////////////
// Notes:
//     Database objects need to be initialized, specifically any
// pointer references to something that doesn't exist yet, not even
// 'NULL'.
//     This prevents SIGSEGV (Signal 11) crashes from occurring.
/////////////////////////////////////////////////////////////////////
// Returns: -
/////////////////////////////////////////////////////////////////////
void db_init_object(dbref db_obj_id)
{
    db[db_obj_id].atrdefs = NULL;
}

static void db_free()
{
    dbref obj_id;
    struct object *obj;

    if (db)
    {
        for(obj_id = 0; obj_id < db_top; obj_id++)
        {
            obj = &db[obj_id];
            SET(obj->name, NULL);
            atr_free(obj_id);
            // Everything starts off very old 
        }

        free(db - 5);

        db      = 0;
        db_top  = 0;
        db_init = db_top;
    }
}

///////////////////////////////////////////////////////////////////////////////
// get_list()
///////////////////////////////////////////////////////////////////////////////
//    Mainly called from db_read_object() , this is used to read the attribute
// list.
///////////////////////////////////////////////////////////////////////////////
// Returns: Boolean
///////////////////////////////////////////////////////////////////////////////
static int get_list(FILE *db_file, dbref obj_id, int dbVersion)
{
    char buf[BUF_SIZE];
    ATRDEF *attributes;
    dbref attr_obj_id;
    dbref old_db_top;
    int attr_id;
    char *s;
    int read_char;
    int i;

    while(TRUE)
    {
        switch(read_char = fgetc(db_file))
        {
            case '>': // read # then string 
                attr_id     = getref(db_file);
                attr_obj_id = getref(db_file);

                if (attr_obj_id == NOTHING)
                {
                    if (builtin_atr(attr_id) && !(builtin_atr(attr_id)->flags & AF_UNIMP))
                    {
                        atr_add(obj_id, builtin_atr(attr_id), s = getstring_noalloc(db_file));
                    }
                    else
                    {
                        getstring_noalloc(db_file);
                    }
                }
                else
                {
                    if (attr_obj_id >= obj_id)
                    {
                        // ergh, haven't read it in yet.
                        // this references an object which has not yet been read in yet - Belisarius
                        old_db_top = db_top;
                        db_grow(attr_obj_id+1);
                        db_init_object(attr_obj_id);
                        db_top = old_db_top;

                        if (db[attr_obj_id].atrdefs == NULL)
                        {
                            db[attr_obj_id].atrdefs = malloc( sizeof(ATRDEF));
                            db[attr_obj_id].atrdefs->a.name = NULL;
                            db[attr_obj_id].atrdefs->next   = NULL;
                        }

                        attributes = db[attr_obj_id].atrdefs;

                        for (attributes = db[attr_obj_id].atrdefs, i = 0; attributes->next && i < attr_id; attributes = attributes->next, i++)
                        {
                        }
                        ; // check to see if it's already there.

                        while (i < attr_id)
                        {
                            attributes->next         = malloc( sizeof(ATRDEF));
                            attributes->next->a.name = NULL;
                            attributes->next->next   = NULL;
                            attributes = attributes->next;
                            i++;
                        }
                        atr_add(obj_id, &(attributes->a), s = getstring_noalloc(db_file));
                    }
                    else
                    {
                        for (attributes = db[attr_obj_id].atrdefs, i = 0; i < attr_id; attributes = attributes->next, i++)
                        ;
                        atr_add (obj_id, &(attributes->a), s = getstring_noalloc(db_file));
                    }
                }
                break;
            case '<': // end of list
                if ('\n' != fgetc(db_file))
                {
                    sprintf(buf, "no line feed on object %d", obj_id);
                    log_error(buf);
                    return(FALSE);
                }

                return(TRUE);
            default:
                sprintf(buf, "Bad character %c on object %d", read_char, obj_id);
                log_error(buf);
                return(FALSE);
        }
    }
}

static object_flag_type upgrade_flags(int version, dbref player, object_flag_type flags)
{
    long type;
    int iskey, member, chown_ok, link_ok, iswizard;

    // only modify version 1 
    if ( version > 1 )
    {
        return flags;
    }

    // record info from old bits 
    iskey    = (flags & 0x8);       // if was THING_KEY 
    link_ok  = (flags & 0x20);      // if was LINK_OK 
    chown_ok = (flags & 0x40000);   // if was CHOWN_OK 
    member   = (flags & 0x2000);    // if player was a member 
    iswizard = (flags & 0x10);      // old wizard bit flag 
    type     = flags & 0x3;         // yank out old object type 

    // clear out old bits 
    flags &= ~TYPE_MASK;            // set up for 4-bit encoding 
    flags &= ~THING_KEY;            // clear out thing key bit 
    flags &= ~INHERIT_POWERS;       // clear out new spec pwrs bit 
    flags &= ~CHOWN_OK;
    flags &= ~LINK_OK;

    // put these bits in their new positions 
    flags |= (iskey)    ? THING_KEY : 0;
    flags |= (link_ok)  ? LINK_OK   : 0;
    flags |= (chown_ok) ? CHOWN_OK  : 0;
    // encode type under 4-bit scheme 
    // nonplayers 
    if ( type != 3 )
    {
        flags |= type;

        if ( iswizard )
        {
            flags |= INHERIT_POWERS;
        }
    }
    else if ( player == 1 )
    {
        // root
        flags |= TYPE_DIRECTOR;
    }
    else if ( iswizard )
    {
        // wizards 
        flags |= TYPE_ADMIN;
    }
    else if ( member )
    {
        // members
        flags |= TYPE_MEMBER;
    }
    else if ( (flags & PLAYER_MORTAL) )
    {
        // guests 
        flags &= ~PLAYER_MORTAL;
        flags |= TYPE_GUEST;
    }
    else
    {
        // trial players 
        flags |= TYPE_TRIALPL;
    }
#undef TYPE_DIRECTOR
#undef TYPE_GUEST
#undef TYPE_TRIALPL
#undef TYPE_MEMBER
#undef TYPE_ADMIN

    return flags;
}

// put quotas in! 
static void db_check()
{
    char buf[BUF_SIZE];
    dbref i;

    for (i = 0; i < db_top; i++)
    {
        if (Typeof(i) == TYPE_PLAYER)
        {
            dbref j;
            int cnt = (-1);

            for (j = 0; j < db_top; j++)
            {
                if (db[j].owner == i)
                {
                    cnt++;
                }
            }

            sprintf(buf, "%d", atoi(atr_get(i, A_RQUOTA))+cnt);
            atr_add(i, A_QUOTA, buf);
        }
    }
}

static void count_atrdef_refcounts ()
{
    int i;
    ATRDEF *k;
    ALIST *l;

    for (i = 0; i < db_top; i++)
    {
        for (k = db[i].atrdefs; k; k = k->next)
        {
            k->a.refcount = 1;
        }
    }

    for (i = 0; i < db_top; i++)
    {
        for (l = db[i].list; l; l = AL_NEXT(l))
        {
            if (AL_TYPE(l))
            {
                ref_atr(AL_TYPE(l));
            }
        }
    }
}

void db_set_read(FILE *file)
{
    db_read_file = file;
}


static void run_startups()
{
  dbref i;
  struct descriptor_data *d;
  int do_startups = 1;

  for (d = descriptor_list; d; d = d->next)
  {
      if (d->state == RELOADCONNECT)
      {
          db[d->player].flags &= ~CONNECT; // so announce_disconnect here won't get run
      }
  }

  {
      FILE *f;

      f = fopen("nostartup", "r");

      if (f)
      {
          fclose(f);
          do_startups = 0;
      }
  }

  for (i = 0; i < db_top; i++)
  {
      if (*atr_get(i, A_STARTUP) && do_startups)
      {
          parse_que (i, atr_get(i, A_STARTUP), i);
      }
      if (db[i].flags&CONNECT)
      {
          announce_disconnect(i);
      }
  }
}

static void welcome_descriptors()
{
    struct descriptor_data *d;
    char buf[BUF_SIZE];

    sprintf(buf, "MUSE reload complete.  Welcome back!\n");

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == RELOADCONNECT && d->player >= 0)
        {
            d->state = CONNECTED;
            db[d->player].flags |= CONNECT;
            queue_string(d, buf);
        }
    }
}


/////////////////////////////////////////////////////////////////////
// load_database()
/////////////////////////////////////////////////////////////////////
//     Reads in the database from the designated database file. A
// failure to load the database results in the MUD being shutdown.
/////////////////////////////////////////////////////////////////////
// Returns: -
/////////////////////////////////////////////////////////////////////
void
load_database()
{
    static dbref db_obj_id = NOTHING;

    // Check to see if we've already loaded
    if (db_loaded)
    {
        // Database has already been loaded
        return;
    }

    // Initialization
    clear_players();
    db_free();
    db_obj_id = 0;

    // Read every object
    for (int j = 0; j < 123 && db_obj_id >= 0; j++, db_obj_id++)
    {
        if ( ( (db_obj_id % 1000) == 1) && db_init)
        {
            struct descriptor_data *d;
            char buf[BUF_SIZE];

            sprintf(buf, "Loading database object #%d of %d.\n", (db_obj_id - 1), (db_init * 2/3) );

            if ((db_obj_id - 1) != 0)
            {
                for (d = descriptor_list; d; d = d->next)
                {
                    queue_string(d, buf);
                }
            }
        }
        db_obj_id = db_read_object (db_obj_id, db_read_file);
    }

    // if db_read_object() returns (-2) this signals that all objects
    // have been read
    if (db_obj_id == -2)
    {
        char buf[BUF_SIZE];
        struct descriptor_data *d;

        db_loaded = TRUE;

        sprintf(buf, "Loading +mail from the database.\n");
        for (d = descriptor_list; d; d = d->next)
        {
            queue_string(d, buf);
        }
        read_mail(db_read_file);

        sprintf(buf, "Counting attribute references.\n");
        for (d = descriptor_list; d; d = d->next)
        {
            queue_string(d, buf);
        }
        count_atrdef_refcounts();

        sprintf(buf, "Running startups.\n");
        for (d = descriptor_list; d; d = d->next)
        {
            queue_string(d, buf);
        }

        run_startups();
        welcome_descriptors();

        log_important("MUSE online.");

        return;
    }

    if (db_obj_id == -1)
    {
        log_error("Couldn't load database; shutting down the muse.");
        exit_nicely(136);
    }
}

/////////////////////////////////////////////////////////////////////
// db_read_object()
/////////////////////////////////////////////////////////////////////
// Reads the next object inside the database
/////////////////////////////////////////////////////////////////////
// Returns: (int)
/////////////////////////////////////////////////////////////////////
static int db_read_object (dbref db_obj_id, FILE *file)
{
    char buf[BUF_SIZE];
    int c;
    struct object *o;
    char *end;

    c = getc(file);

    switch(c)
    {
        // read version number
        case '@':
            db_version = getref(file);

            sprintf(buf, "DB is version %d", db_version);
            log_important(buf);

            if ( db_version != DB_VERSION )
            {
                sprintf(buf, "Converting DB from v%d to v%d", db_version, DB_VERSION);
                log_important(buf);
            }

            user_limit = getref(file);
            restrict_connect_class = getref(file);

            break;

            // make sure database is at least this big *1.5
        case '~':
            db_init = (getref(file)*3)/2;
            break;
        // TinyMUSH new version object storage definition
        case '!': // non-zone oriented database
        case '&': // zone oriented database
            // make space
            db_obj_id = getref(file);
            db_grow(db_obj_id + 1);
            // read it in
            o = db + db_obj_id;
            getstring(file, o->name);
            o->location = getref(file);

            if ( c == '!' )
            {
                o->zone = NOTHING;
            }
            else
            {
                o->zone = getref(file);
            }

            o->contents = getref(file);
            o->exits    = getref(file);

            o->link = getref(file);
            o->next = getref(file);
            o->list = NULL;

            o->owner = getref(file);

            o->flags = upgrade_flags(db_version, db_obj_id, getref(file));


            o->mod_time    = getref(file);
            o->create_time = getref(file);

            if (Typeof(db_obj_id) == TYPE_PLAYER)
            {
                get_powers(db_obj_id, getstring_noalloc(file));
            }
            else
            {
                o->pows = NULL;
            }

            // read attribute list for item
            if (!get_list (file, db_obj_id, db_version))
            {
                sprintf(buf, "bad attribute list object %d", db_obj_id);
                log_error(buf);
                return -2;
            }

            o->parents  = getlist (file);
            o->children = getlist (file);
            o->atrdefs  = get_atrdefs (file, o->atrdefs);

            // check to see if it's a player
            if (Typeof(db_obj_id) == TYPE_PLAYER)
            {
                // ack!
                add_player(db_obj_id);
            }
            break;
        case '*':
            end = getstring_noalloc(file);

            if (strcmp(end, "**END OF DUMP***"))
            {
                // free((void *) end);
                sprintf(buf, "no end of dump %d", db_obj_id);
                log_error(buf);
                return -2;
            }
            else
            {
                extern void zero_free_list P((void));
                // free((void *) end);
                log_important("done loading database.");
                zero_free_list();
                db_check();
                return -3;
            }
        default:
            sprintf(buf, "failed object %d", db_obj_id);
            log_error(buf);
            return -2;
    } 

    return db_obj_id;
}
///////////////////////////////////////////////////////////////////////////////
// getlist()
///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
// Returns: *dbref
///////////////////////////////////////////////////////////////////////////////
static dbref *getlist(FILE *file)
{
    dbref *op;
    int length;

    length = getref(file);

    if (length == 0)
    {
        return NULL;
    }

    op = malloc( sizeof(dbref)*(length+1));
    op[length] = NOTHING;

    for (length--; length >= 0; length--)
    {
        op[length] = getref(file);
    }

    return op;
}

static void putlist(FILE *f, dbref *list)
{
    int k;

    if ((!list) || (list[0] == NOTHING))
    {
        putref (f, 0);
        return;
    }

    for (k = 0; list[k] != NOTHING; k++);

    putref (f, k);

    for (k--; k >= 0; k--)
    {
        putref (f, list[k]);
    }
}

char *unparse_attr(ATTR *atr, int dep)
{
    static char buf[BUF_SIZE];

    buf[dep] = 0;

    if (dep)
    {
        for (dep--; dep >= 0; dep--)
        {
            buf[dep] = '+';
        }
    }

    if (atr->obj == NOTHING)
    {
        strcat (buf, atr->name);
    }
    else
    {
        sprintf(buf+strlen(buf),"#%d.%s",atr->obj, atr->name);
    }

    return buf;
}

#define DOATTR(var, name, flags, num) ATTR *var;
#define DECLARE_ATTR
#include "attrib.h"
#undef DECLARE_ATTR
#undef DOATTR

void init_attributes()
{
#define DOATTR(var, name, flags, num) var = builtin_atr(num);
#include "attrib.h"
#undef DOATTR
}


void update_bytes()
{
    int difference;
    int newsize;

    if ((++update_bytes_counter) >= db_top)
    {
        update_bytes_counter = 0;
    }

    for (int i = 0; i < 100; i++)
    {
        if (db[update_bytes_counter].i_flags & I_UPDATEBYTES)
        {
            break;
        }

        if ((++update_bytes_counter) >= db_top)
        {
            update_bytes_counter = 0;
        }
    }

    if (!(db[update_bytes_counter].i_flags & I_UPDATEBYTES))
    {
        // couldn't find any
        return;
    }

    newsize = mem_usage(update_bytes_counter);
    difference = newsize-db[update_bytes_counter].size;
    add_bytesused(db[update_bytes_counter].owner, difference); // this has to be done right here in the middle, because it calls recalc_bytes which may reset size and I_UPDATEBYTES. 
    db[update_bytes_counter].size = newsize;
    db[update_bytes_counter].i_flags &= ~I_UPDATEBYTES;
}

/*****************************************************XXX BRG DB CODE XXX*/

///////////////////////////////////////////////////////////////////////////////
// atr_fputs - needed to support %r substitution
// does: outputs string <what> on stream <fp>, quoting newlines
// with a DB_LOGICAL (currently ctrl-U).
// added on 4/26/94 by Brian Gaeke (Roodler)
///////////////////////////////////////////////////////////////////////////////
void atr_fputs(char *what, FILE * fp)
{
        while (*what)
        {
                if (*what == '\n')
                        fputc(DB_LOGICAL, fp);
                fputc(*what++, fp);
        }
}

///////////////////////////////////////////////////////////////////////////////
// atr_fgets - needed to support %r substitution              *
// does: inputs a string, max <size> chars, from stream <fp>, *
// into buffer <buffer>. if a DB_LOGICAL is encountered,      *
// the next character (possibly a \n) won't terminate the     *
// string, as it would in fgets.                              *
// added on 4/26/94 by Brian Gaeke (Roodler)                  */
///////////////////////////////////////////////////////////////////////////////
char *atr_fgets(char *buffer, int size, FILE * fp)
{
    int num_read = 0;
    char ch, *mybuf = buffer;

    for (;;)
    {
        ch = getc(fp);

        if (ch == EOF)
                break;

        if (ch == DB_LOGICAL)
        {
                ch = getc(fp);
                *mybuf++ = ch;
                num_read++;
                continue;
        }

        if (ch == '\n')
        {
                *mybuf++ = ch;
                num_read++;
                break;
        }

        *mybuf++ = ch;
        num_read++;

        if (num_read > size)
                break;
    }

    *mybuf = '\0';

    return buffer;
}

void putstring(FILE *f, char *s)
{
    if (s)
    {
        atr_fputs(s, f);
    }

    fputc('\n', f);
}
