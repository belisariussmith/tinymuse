///////////////////////////////////////////////////////////////////////////////
// hash.h                                                    TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: hash.h,v 1.2 1993/04/19 20:58:44 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// declarations for generic hash table functions and structures 
///////////////////////////////////////////////////////////////////////////////

// this is used for the entries to declare the hash table. when we first
// go, we transfer all these into struct hashent 
struct hashdeclent {
  char *name;
};                              // more may come after this in memory.

struct hashent {
  char *name;                   // null signals end of list
  void *value;
  int hashnum;
};

typedef struct hashent *hashbuck;

struct hashtab {
  int nbuckets;
  hashbuck *buckets;
  char *name;
  char *(*display) P((void *));
  struct hashtab *next;
};

extern struct hashtab *make_hashtab P((int nbuck, void *ents, int entsize, char *, char *(*)(void *)));
extern void *lookup_hash P((struct hashtab *tab, int hashvalue, char *name));
extern int hash_name P((char *name));
