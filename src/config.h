/* config.h */
/* config-dist,v 1.3 1993/02/06 18:00:09 nils Exp */

// note: some of the options in here may not work/compile correctly.
// please try compiling first without changing things like
// REGISTRATION and RESTRICTED_BUILDING. 

#ifndef _LOADED_CONFIG_
#define _LOADED_CONFIG_

// should be the email address of someone who looks over crash
// logs to try and figure out what the problem is. #undef if
// you don't want to enable emailing of crash logs 
// #define TECH_EMAIL "nils@chezmoto.ai.mit.edu"
#undef TECH_EMAIL

// use the mnemosyne malloc debugging package. this is recommended for
// server developers. 
#undef MALLOCDEBUG

// These are various characters that do special things. I recommend you
// don't change these unless you won't have any people who are used
// to tinymud using the muse.
#define NOT_TOKEN     '!'
#define AND_TOKEN     '&'
#define OR_TOKEN      '|'
#define THING_TOKEN   'x'
#define LOOKUP_TOKEN  '*'
#define NUMBER_TOKEN  '#'
#define AT_TOKEN      '@'
#define ARG_DELIMITER '='
#define IS_TOKEN      '='
#define CARRY_TOKEN   '+'

// These are various tokens that are abbreviations for special commands.
// Again, i recommend you don't change these unless you won't have any people
// who are used to tinymud.
#define SAY_TOKEN  '"'
#define POSE_TOKEN ':'
#define NOSP_POSE  ';'
#define COM_TOKEN  '='
#define TO_TOKEN   '\''

// This is the maximum value of an object. You can @create <object>=a
// value like 505, and it'll turn out to have a value of 100. use the
// two formulas afterwrds.
#define MAX_OBJECT_ENDOWMENT 100
#define OBJECT_ENDOWMENT(cost)  (((cost)-5)/5)
#define OBJECT_DEPOSIT(pennies) ((pennies)*5+5)

// This is the character that seperates different exit aliases. If you change
// this, you'll probably have a lot of work fixing up your database. 
#define EXIT_DELIMITER ';'

// special interface commands. i suggest you don't change these. 
#define QUIT_COMMAND "QUIT"

// commands to set the output suffix and output prefix.
#define PREFIX_COMMAND "OUTPUTPREFIX"
#define SUFFIX_COMMAND "OUTPUTSUFFIX"

// If this is a usg-based system, redefine random and junk 
#if defined(XENIX) || defined(HPUX) || defined(SYSV) || defined(USG)
#define random rand
#define srandom srand
#endif

#define QUOTA_COST 1 // why you would want to change this, i don't know 
#define MAX_ARG 100

// change this if you add or delete a directory in the directory tree.  (deprecated)
//#define MUSE_DIRECTORIES "src src/ident src/hdrs src/comm src/io src/db src/util src/files src/core run run/files run/files/p run/files/p/1 run/db run/msgs run/logs doc bin config rbin"

#endif /* _LOADED_CONFIG_ */
