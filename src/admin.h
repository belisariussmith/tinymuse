///////////////////////////////////////////////////////////////////////////////
// admin.h                                            TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: admin.h,v 1.2 1992/10/11 15:14:51 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////

// utility functions
int try_force();
void calc_stats();
int owns_stuff();

// command functions 
void do_search();
void do_teleport();
void do_force();
void do_stats();
void do_pstats();
void do_wipeout();
void do_chownall();
void do_poor();
void do_allquota();
void do_newpassword();
void do_boot();
void do_join();
void do_summon();
