///////////////////////////////////////////////////////////////////////////////
// hash.c                                            TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: hash.c,v 1.3 1993/07/25 19:06:50 nils Exp $
///////////////////////////////////////////////////////////////////////////////
// generic hash table functions
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "externs.h"
#include "hash.h"

static struct hashtab *hashtabs = NULL;

void free_hash()
{
    struct hashtab *i;
    struct hashtab *next;

    for (i = hashtabs; i; i = next)
    {
        int j;

        next = i->next;

        for (j = 0; j < i->nbuckets; j++)
        {
//          int k;
//          for (k = 0; j->buckets[j][k].name; k++)
//      	free(i->buckets[j][k].name);
            free(i->buckets[j]);
        }

        free(i->buckets);
        free(i->name);
        free(i);
    }
}

void do_showhash(dbref player, char *arg1)
{
    struct hashtab *i;

    if (!*arg1)
    {
        for (i = hashtabs; i; i = i->next)
        {
            send_message(player, i->name);
        }

        send_message(player, "done.");
    }
    else
    {
        char buf[BUF_SIZE];

        for (i = hashtabs; i; i = i->next)
        {
            if (!string_compare(arg1, i->name))
            {
                int j;
                send_message(player, "%s (%d buckets):", i->name, i->nbuckets);

                for (j = 0; j < i->nbuckets; j++)
                {
                    int k;

                    send_message(player, " bucket %d:", j);

                    for (k=0; i->buckets[j][k].name; k++)
                    {
                        sprintf(buf,"   %s: %s", i->buckets[j][k].name, (*i->display)(i->buckets[j][k].value));
                        send_message(player, buf);
                    }
                }
                return;
            }
        }

        send_message(player,"couldn't find that.");

        return;
    }
}

struct hashtab *make_hashtab(int nbuck, void *ents, int entsize, char *name, char *(*displayfunc)(void *))
{
    struct hashtab *op;
    int i;
  
    op = malloc (sizeof (struct hashtab));
    op->nbuckets = nbuck;
    op->buckets = malloc (sizeof (hashbuck) * nbuck);
    op->name = malloc (strlen(name)+1);
    strcpy(op->name, name);
    op->display = displayfunc;
    op->next = hashtabs;
    hashtabs = op;

    for (i = 0; i < nbuck; i++)
    {
        int numinbuck=1;
        struct hashdeclent *thisent;
    
        for (thisent = ents; thisent->name; thisent = (struct hashdeclent *)(((char *)thisent)+entsize))
        {
            int val = hash_name(thisent->name);

            if ((val%nbuck) == i)
            {
            	numinbuck++;
            }
        }

        op->buckets[i] = malloc (sizeof (struct hashent) * numinbuck);
        op->buckets[i][--numinbuck].name = NULL; // end of list marker 

        for (thisent = ents; thisent->name; thisent = (struct hashdeclent *)(((char *)thisent)+entsize))
        {
              int val = hash_name(thisent->name);

              if ((val%nbuck) == i)
              {
                  op->buckets[i][--numinbuck].name = thisent->name;
                  op->buckets[i][numinbuck].hashnum = val;
                  op->buckets[i][numinbuck].value = thisent;
              }
        }
    }

    return op;
}


void *lookup_hash(struct hashtab *tab, int hashvalue, char *name)
{
    int i;

    hashbuck z = tab->buckets[hashvalue%tab->nbuckets];

    for (i = 0; z[i].name; i++)
    {
        if (z[i].hashnum == hashvalue)
        {
            if (!string_compare (z[i].name, name))
            {
                return z[i].value;
            }
        }
    }

    return NULL;
}

int hash_name(char *name)
{
    int i;
    int j;
    int shift=0;
    unsigned short op=0;
  
    for (i = 0; name[i]; i++)
    {
        j = to_lower(name[i]);	// case insensitive 
        op ^= j<<(shift);
        op ^= j>>(16-shift);
        shift = (shift+11)%16;
    }

    return op;
}
