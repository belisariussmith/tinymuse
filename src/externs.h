///////////////////////////////////////////////////////////////////////////////
// externs.h 
///////////////////////////////////////////////////////////////////////////////
// $Id: externs.h,v 1.36 1994/01/26 22:28:39 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// for doing prototypes 
///////////////////////////////////////////////////////////////////////////////

#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#endif

#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include "db.h"
// for the text_queue and descriptor_list and stuff 
#include "net.h"
// for struct timeval
#include <sys/time.h>
// for FIFO
#include "fifo.h"
// for struct sockaddr
#ifndef SOCK_STREAM
#include <sys/socket.h>
#endif
// for struct stat
#ifndef S_IFMT
#include <sys/stat.h>
#endif

#ifndef FD_SETSIZE
#include <sys/types.h>
#endif

#include "log.h"

#if defined(ultrix) || defined(linux)
#define void_signal_type
#endif

#ifdef void_signal_type
#define signal_type void
#else
#define signal_type int
#endif

// and all the definitions
// From ansi.c
extern char *color P((char *, int));

// From admin.c
extern void do_su P((dbref, char *, char *));
extern void do_swap P((dbref, char *, char *));
extern dbref match_controlled P((dbref, char *, int));
extern void calc_stats P((dbref, int *, int *, int *));
extern void do_allquota P((dbref, char *));
extern void do_boot P((dbref, char *));
extern void do_cboot P((dbref, char *));
extern void do_chownall P((dbref, char *, char *));
extern void do_force P((dbref, char *, char *));
extern void do_join P((dbref, char *));
extern void do_newpassword P((dbref, char *, char *));
extern void do_poor P((dbref, char *));
extern void do_pstats P((dbref, char *));
extern void do_search P((dbref, char *, char *));
extern void do_stats P((dbref, char *));
extern void do_summon P((dbref, char *));
extern void do_teleport P((dbref, char *, char *));
extern void do_wipeout P((dbref, char *, char *));
extern int owns_stuff P((dbref));
extern int try_force P((dbref, char *));
extern void do_gethost P((dbref, char *));

// From boolexp.c
extern int eval_boolexp P((dbref, dbref, char *, dbref));
extern char *process_lock P((dbref, char *));
extern char *unprocess_lock P((dbref, char *));

// From bsd.c
extern void outgoing_setupfd P((dbref, int));
extern int db_loaded; 
extern time_t now;
extern void do_ctrace P((dbref));
extern void do_outgoing P((dbref, char *, char *));
extern void announce_connect P((dbref));
extern void announce_disconnect P((dbref));
extern int boot_off P((dbref));
extern void dump_users P((dbref, char *, char *, struct descriptor_data *));
extern void emergency_shutdown P((void));
extern int getdtablesize P((void));
extern int queue_string P((struct descriptor_data *,char *));
extern void raw_notify P((dbref, char *));
extern void remove_muse_pid P((void));
extern void shutdownsock P((struct descriptor_data *));
extern char *time_format_1 P((time_t));
extern char *time_format_2 P((time_t));
extern void welcome_user P((struct descriptor_data *));
extern int user_limit;
extern int user_count;
extern int restrict_connect_class;
extern int nologins;
extern void do_lockout P((dbref, char *));
extern void do_usercap P((dbref, char *));
extern void do_nologins P((dbref, char *));
extern void notify_group P((dbref, char *));

// From comm.c 
extern void com_send P((char *,char *));
extern void do_com P((dbref, char *, char *));
extern void do_channel P((dbref, char *));
extern void do_ctitle P((dbref, char *));
extern int is_on_channel P((dbref, char *));

// From conf.c 
extern void info_config P((dbref));

// From config.c 
extern char *guest_fail P((void));
extern char *perm_denied P((void));
extern void do_config P((dbref, char *, char *));
#define DO_NUM(nam,var) extern int var;
#define DO_STR(nam,var) extern char *var;
#define DO_REF(nam,var) extern dbref var;
#include "conf.h"
#undef DO_NUM
#undef DO_STR
#undef DO_REF

// From cque.c
extern void do_haltall P((dbref));
extern void do_halt P((dbref, char *));
extern void do_queue P((dbref));
extern void do_second P((void));
extern int do_top P((void));
extern void parse_que P((dbref, char *, dbref));
extern int test_top P((void));
extern void wait_que P((dbref, int, char *, dbref));

// From create.c 
extern void do_clone P((dbref, char *,char *));
extern void do_create P((dbref, char *, int));
extern void check_spoofobj P((dbref, dbref));
extern void do_dig P((dbref, char *, char **));
extern void do_link P((dbref, char *, char *));
extern void do_zlink P((dbref, char *, char *));
extern void do_unzlink P((dbref, char *));
extern void do_ulink P((dbref, char *));
extern void do_open P((dbref, char *, char *, dbref));
extern void do_robot P((dbref, char *, char *));

// From db.c
extern void update_bytes P((void));
extern dbref get_zone_first P((dbref));
extern dbref get_zone_next P((dbref));
extern void db_set_read P((FILE *));
extern void load_database P((void));
extern void free_database P((void));
extern void init_attributes P((void));
extern char *unparse_attr P((ATTR *, int dep));
extern void atr_add P((dbref, ATTR *, char *));
extern void atr_clr P((dbref, ATTR *));
extern ATTR *builtin_atr_str P((char *));
extern void atr_collect P((dbref));
extern void atr_cpy_noninh P((dbref, dbref));
extern void atr_free P((dbref));
extern char *atr_get P((dbref, ATTR *));
extern ATTR *atr_str P((dbref, dbref, char *));
extern dbref db_write P((FILE *));
extern dbref getref P((FILE *));
extern dbref new_object P((void));
extern dbref parse_dbref P((char *));
extern void putref P((FILE *, dbref));

// From dbtop.c
extern void do_dbtop P((dbref, char *));

// From decompress.c

// From destroy.c
extern void info_db P((dbref));
extern void do_check P((dbref, char *));
extern void do_incremental P((void));
extern void do_dbck P((dbref));
extern void do_empty P((dbref));
extern void fix_free_list P((void));
extern dbref free_get P((void));
extern void do_undestroy P((dbref,char *));
extern void do_upfront P((dbref, char *));

// From eval.c
extern void info_funcs P((dbref));
extern char *wptr[10];
extern void exec P((char **, char *, dbref, dbref, int));
extern void func_zerolev P((void));
extern dbref match_thing P((dbref, char *));
extern char *parse_up P((char **, int));
extern int mem_usage P((dbref));

// From game.c
extern void exit_nicely P((int));
extern void dest_info P((dbref, dbref));
extern int Live_Player P((dbref));
extern int Live_Puppet P((dbref));
extern int Listener P((dbref));
extern int Commer P((dbref));
extern int Hearer P((dbref));
extern void dump_database P((void));
extern void fork_and_dump P((void));
extern int init_game P((char *, char *));
extern void notify P((dbref, char *));
extern void panic P((char *));
extern void process_command P((dbref, char *, dbref));
extern void report P((void));

// From hash.c
extern void do_showhash P((dbref, char *));
extern void free_hash P((void));

// From help.c
extern void do_text P((dbref, char *, char *, ATTR *));

// From info.c
extern void do_info P((dbref, char *));

// From inherit.c 
extern void do_undefattr P((dbref, char *));
extern int is_a P((dbref, dbref));
extern void do_defattr P((dbref, char *, char *));
extern void do_addparent P((dbref, char *, char *));
extern void do_delparent P((dbref, char *, char *));
extern void put_atrdefs P((FILE *, ATRDEF *));
extern ATRDEF *get_atrdefs P((FILE *, ATRDEF *));

// From look.c
extern char *flag_description P((dbref));
extern struct all_atr_list *all_attributes P((dbref));
extern void do_examine P((dbref, char *, char *));
extern void do_find P((dbref, char *));
extern void do_inventory P((dbref));
extern void do_laston P((dbref, char *));
extern void do_look_around P((dbref));
extern void do_look_at P((dbref, char *));
extern void do_score P((dbref));
extern void do_sweep P((dbref, char *));
extern void do_whereis P((dbref, char *));
extern void look_room P((dbref, dbref));

// From mail.c
extern void check_mail P((dbref));
extern void do_mail P((dbref, char *, char *));
extern void init_mail P((void));
extern void free_mail P((void));
extern void write_mail P((FILE *));
extern void read_mail P((FILE *));
extern void clear_old_mail P((void));

// From match.c 
extern void init_match P((dbref, char *, int));
extern void init_match_check_keys P((dbref, char *, int));
extern dbref last_match_result P((void));
extern void match_absolute P((void));
extern void match_everything P((void));
extern void match_exit P((void));
extern void match_here P((void));
extern void match_me P((void));
extern void match_neighbor P((void));
extern void match_perfect P((void));
extern void match_player P((void));
extern void match_possession P((void));
extern dbref match_result P((void));
extern dbref noisy_match_result P((void));
extern dbref pref_match P((dbref, dbref, char *));

// From move.c
extern int can_move P((dbref, char *));
extern void do_drop P((dbref, char *));
extern void do_enter P((dbref, char *));
extern void do_get P((dbref, char *));
extern void do_leave P((dbref));
extern void do_move P((dbref, char *));
extern void enter_room P((dbref, dbref));
extern void moveto P((dbref, dbref));
extern void safe_tel P((dbref, dbref));
extern dbref get_room P((dbref));
extern void do_follow P((dbref, char *));

// From nalloc.c
extern char *glurpdup P((char *));
#define newglurp(size) newglurp_fil(size,__LINE__,__FILE__)
extern void *newglurp_fil P((int, int, char *));
extern void clearglurp P((void));

// From pcmds.c
extern void do_at P((dbref, char *, char *));
extern void do_as P((dbref, char *, char *));
extern void do_version P((dbref));
extern void do_uptime P((dbref));
extern void do_cmdav P((dbref));
extern void inc_pcmdc P((void));
extern void inc_qcmdc P((void));

// From player.c 
extern dbref *match_things P((dbref, char *));
extern ptype name_to_pow P((char *));
extern void do_powers P((dbref, char *));
extern void do_misc P((dbref, char *, char *));
extern void do_empower P((dbref, char *, char *));
extern dbref connect_player P((char *, char *));
extern dbref create_guest P((char *, char *));
extern dbref create_player P((char *, char *, int, dbref));
extern void destroy_guest P((dbref));
extern void do_class P((dbref, char *, char *));
extern void do_nopow_class P((dbref, char *, char *));
extern void do_money P((dbref, char *, char *));
extern void do_nuke P((dbref, char *));
extern void do_password P((dbref, char *, char *));
extern void do_pcreate P((dbref, char *, char *));
extern void do_quota P((dbref, char *, char *));
extern char *get_class P((dbref));
extern dbref *lookup_players P((dbref, char *));

// From player_list.c
extern void add_player P((dbref));
extern void clear_players P((void));
extern void delete_player P((dbref));
extern dbref lookup_player P((char *));

// From predicates.c
extern dbref check_zone P((dbref, dbref, dbref, int));
extern char *safe_name P((dbref));
extern dbref starts_with_player P((char *));
extern int can_see_atr P((dbref, dbref, ATTR *));
extern int can_set_atr P((dbref, dbref, ATTR *));
extern void push_list P((dbref **, dbref));
extern void remove_first_list P((dbref **, dbref));
extern int Levnm P((dbref));
extern int inf_mon P((dbref));
extern int Level P((dbref));
extern void add_quota P((dbref, int));
extern void recalc_bytes P((dbref));
extern void add_bytesused P((dbref, int));
extern int can_link P((dbref, dbref, int));
extern int can_link_to P((dbref, dbref, int));
extern int can_pay_fees P((dbref, int, int));
extern int can_see P((dbref, dbref, int));
extern int controls_a_zone P((dbref, dbref, int));
extern int controls P((dbref, dbref, int));
extern int is_in_zone P((dbref, dbref));
extern dbref def_owner P((dbref));
extern int could_doit P((dbref, dbref, ATTR *));
extern void did_it P((dbref, dbref, ATTR *, char *, ATTR *, char *, ATTR *));
extern void giveto P((dbref, int));
extern int ok_attribute_name P((char *));
extern int ok_object_name P((dbref, char *));
extern int ok_room_name P((char *));
extern int ok_exit_name P((char *));
extern int ok_thing_name P((char *));
extern int ok_player_name P((dbref, char *, char *));
extern int ok_password P((char *));
extern int pay_quota P((dbref, int));
extern int payfor P((dbref, int));
extern int power P((dbref, int));
extern void pronoun_substitute P((char *, dbref, char *, dbref));
extern char *main_exit_name P((dbref));
extern int sub_quota P((dbref, int));

// From cntl.c 
extern void do_switch P((dbref, char *, char **, dbref));
extern void do_foreach P((dbref, char *, char *, dbref));
extern void do_trigger P((dbref, char *, char **));
extern void do_trigger_as P((dbref, char *, char **));
extern void do_decompile P((dbref, char *, char *));
extern void do_cycle P((dbref, char *, char **));

// From rob.c 
extern void do_giveto P((dbref, char *, char *));
extern void do_give P((dbref, char *, char *));
extern void do_slay P((dbref, char *));

// From set.c 
void mark_hearing P((dbref));
void check_hearing P((void));
extern void do_haven P((dbref, char *));
extern void do_away P((dbref, char *));
extern void do_chown P((dbref, char *, char *));
extern void do_describe P((dbref, char *, char *));
extern void destroy_obj P((dbref, int));
extern void do_recycle P((dbref, char *));
extern void do_edit P((dbref, char *, char **));
extern void do_fail P((dbref, char *, char *));
extern void do_hide P((dbref, char *));
extern void do_idle P((dbref, char *));
extern void do_name P((dbref, char *, char *, int));
extern void do_ofail P((dbref, char *, char *));
extern void do_osuccess P((dbref, char *, char *));
extern void do_set P((dbref, char *, char *, int));
extern void do_unhide P((dbref, char *));
extern void do_unlink P((dbref, char *));
extern void do_unlock P((dbref, char *));
extern int parse_attrib P((dbref, char *, dbref *, ATTR **, int));
extern int test_set P((dbref, char *, char *, char *, int));

// From speech.c 
extern char *reconstruct_message P((char *, char *));
extern void do_use P((dbref, char *));
extern void do_announce P((dbref, char *, char *));
extern void do_broadcast P((dbref, char *, char *));
extern void do_emit P((dbref, char *, char *, int));
extern void do_echo P((dbref, char *, char *, int));
extern void do_gripe P((dbref, char *, char *));
extern void do_page P((dbref, char *, char *));
extern void do_general_emit P((dbref, char *, char *, int));
extern void do_pose P((dbref, char *, char *, int));
extern void do_say P((dbref, char *, char *));
extern void do_whisper P((dbref, char *, char *));
extern void notify_in P((dbref, dbref, char *));
extern char *spname P((dbref));
extern void do_cemit P((dbref, char *, char *));
extern void do_wemit P((dbref, char *, char *));
extern void do_cpage P((dbref, char *, char *));
extern void do_to P((dbref, char *, char *));

// From stringutil.c
extern char *str_index P((char *, int));
extern int string_compare P((char *, char *));
extern char *string_match P((char *, char *));
extern int string_prefix P((char *, char *));
extern char to_lower P((int));
extern char to_upper P((int));

// From timer.c
extern void dispatch P((void));
extern void init_timer P((void));

// From topology.c
extern void run_topology P((void));

// From unparse.c
extern char *unparse_object_a P((dbref, dbref));
extern char *unparse_flags P((dbref));
extern char *unparse_object P((dbref, dbref));

// From utils.c
long mkxtime P((char *, dbref, char *));
extern dbref find_entrance P((dbref));
extern int member P((dbref, dbref));
extern dbref remove_first P((dbref, dbref));
extern dbref reverse P((dbref));
extern char *mktm P((time_t, char *, dbref));

// From wild.c
extern int wild_match P((char *, char *));

// From log.c
extern void close_logs P((void));

// From class.c
extern char *class_to_name P((int));
extern char *public_class_to_name P((int));
extern char *short_class_to_name P((int));
extern char *short_public_class_to_name P((int));
extern int name_to_class P((char *));
extern int class_to_list_pos P((int));

// From powers.c
extern void put_powers P((FILE *, dbref));
extern void get_powers P((dbref, char *));
extern int old_to_new_class P((int));
extern void set_pow(dbref player, ptype pow, ptype val);
extern int has_pow(dbref player, dbref recipt, ptype pow);
extern int get_pow(dbref player, ptype pow);

// newconc.c
extern long make_concid P((void));
extern void do_becomeconc P((struct descriptor_data *, char *));
extern void do_makeid P((struct descriptor_data *));
extern void do_connectid P((struct descriptor_data *, long, char *));
extern void do_killid P((struct descriptor_data *, long));

// external to netmud
extern int tolower P((int));
extern time_t time P((time_t *));
extern pid_t getpid P((void));
extern int close P((int));
extern int dup2 P((int, int));
extern int shutdown P((int, int));
extern int socket P((int, int, int));
extern int listen P((int, int));
extern int ffs P((int));
extern void _exit P((int));
extern pid_t fork P((void));
/* */
/* */
#ifdef sun
extern long srandom P((long));
#endif
extern unsigned int alarm P((unsigned int));
extern int pipe P((int *));
#if !defined(HPUX) || !defined(__STDC__)
/*extern int mkdir P((char *, int));*/
#endif
#ifndef getc
extern int getc P((FILE *));
#endif
#ifndef putc
extern void putc P((char, FILE *));
#endif
/*extern int stat P((char *, struct stat *));*/
extern int vfork P((void));

#ifdef __STDC__
#include <stdlib.h>
#endif

#ifdef MALLOCDEBUG
#include "mnemosyne.h"
#endif
#ifdef linux
extern long inet_addr P((char *));
#else
extern void bzero P((char *, size_t));
#endif

extern char *index P((const char *, int));

#ifndef BSD
#define bcopy(x, y, z) memcpy(y, x, z)
#endif
