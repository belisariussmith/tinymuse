///////////////////////////////////////////////////////////////////////////////
// interface.h                                               TinyMUSE
///////////////////////////////////////////////////////////////////////////////
// $Id: interface.h,v 1.2 1992/10/11 15:15:09 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include "db.h"

// these symbols must be defined by the interface 
extern void notify();
extern int shutdown_flag;           // if non-zero, interface should shut down
extern void emergency_shutdown();
extern int boot_off();              // remove a player

// the following symbols are provided by game.c 

extern void process_command();

extern dbref create_player();
extern dbref connect_player();
extern void do_look_around();

extern int init_game();
extern void dump_database();
extern void panic();
extern int depth;
