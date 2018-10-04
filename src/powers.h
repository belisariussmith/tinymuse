///////////////////////////////////////////////////////////////////////////////
// powers.h                                                   TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: powers.h,v 1.8 1993/05/27 23:17:30 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#ifndef __POWERS_H__
#define __POWERS_H__

#define CLASS_GUEST 	1
#define CLASS_VISITOR 	2
#define CLASS_GROUP 	3
#define CLASS_CITIZEN 	4
#define CLASS_PCITIZEN	5
#define CLASS_GUIDE 	6
#define CLASS_OFFICIAL 	7
#define CLASS_BUILDER 	8
#define CLASS_ADMIN 	9
#define CLASS_DIR 	10

#define NUM_CLASSES 11
#define NUM_LIST_CLASSES 10
extern struct pow_list {
  char *name;                    // name of power 
  ptype num;                     // number of power 
  char *description;             // description of what the power is 
  int init[NUM_LIST_CLASSES];
  int max[NUM_LIST_CLASSES];
} powers[];

#define PW_NO 1
#define PW_YESLT 2
#define PW_YESEQ 3
#define PW_YES 4


#define POW_ALLQUOTA     1
#define POW_ANNOUNCE     2
#define POW_BOOT         3
#define POW_BROADCAST    4
#define POW_CHOWN        5
#define POW_CLASS        6
#define POW_DB           7
#define POW_EXAMINE      8
#define POW_FREE         9
#define POW_WHO          10
#define POW_OUTGOING     11
#define POW_JOIN         12
#define POW_MEMBER       13
#define POW_MODIFY       14
#define POW_MONEY        15
#define POW_NEWPASS      16
#define POW_NOSLAY       17
#define POW_NOQUOTA      18
#define POW_NUKE         19
#define POW_PCREATE      20
#define POW_POOR         21
#define POW_QUEUE        22
#define POW_EXEC         23
#define POW_SEEATR       24
#define POW_SETPOW       25
#define POW_SLAY         26
#define POW_SHUTDOWN     27
#define POW_SUMMON       28
#define POW_SLAVE        29
#define POW_SPOOF        30
#define POW_STATS        31
#define POW_STEAL        32
#define POW_TELEPORT     33
#define POW_WATTR        34
#define POW_WFLAGS       35
#define POW_REMOTE       36
#define POW_SECURITY     37
#define POW_INCOMING     38
#define POW_FUNCTIONS    39
#define POW_DBTOP        40
#define POW_SETQUOTA     41
#define POW_HOST         42
#define POW_BACKSTAGE    43
#define POW_BEEP         44
#define POW_GHOST	 45
#define NUM_POWS	 45
#define MAX_POWERNAMELEN 16
#endif // __POWERS_H_ 
