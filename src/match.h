///////////////////////////////////////////////////////////////////////////////
// match.h 
///////////////////////////////////////////////////////////////////////////////
// $Id: match.h,v 1.2 1992/10/11 15:15:13 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include "db.h"

// match functions 
// Usage: init_match(player, name, type); match_this(); match_that(); ...
// Then get value from match_result()

// initialize matcher 
extern void init_match();
extern void init_match_check_keys();

// match (LOOKUP_TOKEN)player
extern void match_player();

// match (NUMBER_TOKEN)number
extern void match_absolute();

// match "me"
extern void match_me();

// match "here"
extern void match_here();

// match something player is carrying
extern void match_possession();

// match something in the same room as player
extern void match_neighbor();

// match a name exactly in the same room as player
extern void match_perfect();

// match an exit from player's room
extern void match_exit();

// all of the above, except only Wizards do match_absolute and match_player 
extern void match_everything();

// return match results
extern dbref match_result();      // returns AMBIGUOUS for multiple inexacts 
extern dbref last_match_result(); // returns last result 

//#define NOMATCH_MESSAGE "I don't see that here."
#define NOMATCH_PATT "i don't see %s here."

#define AMBIGUOUS_MESSAGE "I don't know which one you mean!"

extern dbref noisy_match_result(); // wrapper for match_result
                                   // noisily notifies player 
                                   // returns matched object or NOTHING 
