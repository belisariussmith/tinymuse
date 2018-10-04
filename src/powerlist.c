///////////////////////////////////////////////////////////////////////////////
// powerlist.c 
///////////////////////////////////////////////////////////////////////////////
// $Id: powerlist.c,v 1.13 1993/05/27 23:17:43 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "db.h"
#include "powers.h"
#include "externs.h"

#define NO	PW_NO
#define YES	PW_YES
#define YESLT	PW_YESLT
#define YESEQ	PW_YESEQ
  
//  The first line of each power structure defines the level of power the
//  player of said class will have when initially @classed.  The second 
//  line shows how powerful that power can get for that class. 
//
//  Old classes / standard classes:
//
//  Drctr  Admin  Cnstr  Offcl  JrOff  Pctzn  Citzn  Group  Vistr  Guest
//  Admrl  Genrl  Captn  1Mate  2Mate  Offcr  Sailr   Crew  Lnlbr  Stwwy
struct pow_list powers[] =
{
  {
    "Allquota", POW_ALLQUOTA, "Ability to alter everyone's quota at once",
    {NO,     NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Announce", POW_ANNOUNCE, "Ability to @announce for free",
    {YES,    YES,   YES,   YES,   YES,    NO,   NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   NO,    NO,    NO,    NO }},
  {
    "Backstage", POW_BACKSTAGE, "Ability to see numbers on all objects",
    {YES,    YES,   YES,   NO,    YES,   NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   NO,    NO,    NO,    NO }},
  {
    "Beep", POW_BEEP, "Ability to use the %a (beep) substitution",
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   YES,   NO,    NO }},
  {
    "Boot", POW_BOOT, "Ability to @boot players off the game",
    {YESLT,  YESLT, NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YESEQ,  YESEQ, YESEQ, YESEQ, YESEQ, YESEQ, NO,    NO,    NO,    NO }},
  {
    "Broadcast", POW_BROADCAST, "Ability to @broadcast a message",
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Chown", POW_CHOWN, "Ability to change ownership of an object",
    {YESEQ,  YESEQ, NO,    YESLT, NO,    NO,    NO,    NO,    NO,    NO },
    {YESEQ,  YESEQ, YESEQ, YESEQ, YESLT, YESEQ, NO,    NO,    NO,    NO }},
  {
    "Class", POW_CLASS, "Ability to re@classify somebody",
    {YESLT,  NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YESLT,  NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Database", POW_DB, "Ability to use @dbck and other database utilities",
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Dbtop", POW_DBTOP, "Abililty to do a @dbtop",
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   NO,    NO,    NO }},
  {
    "Examine", POW_EXAMINE, "Ability to see people's homes and locations",
    {YES,    YESEQ, YESLT, YESEQ, YESEQ, NO,     NO,   NO,    NO,    NO },
    {YES,    YESEQ, YESEQ, YESEQ, YESEQ, YESEQ,  NO,   NO,    NO,    NO }},
  {
    "Exec", POW_EXEC, "Power to execute external programs",
    {NO,     NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Free", POW_FREE, "Ability to build, etc. for free (unused)",
    {YES,    YES,   YES,   NO,    NO,    NO,     NO,   NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,    NO,   YES,   NO,    NO }},
  {
    "Functions", POW_FUNCTIONS, "Ability to get correct results from all functions.",
    {YES,    YES,   YES,   NO,    NO,    NO,     NO,   NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,    NO,   YES,   NO,    NO }},
  {
    "GuestHost", POW_GHOST, "Ability to see hostnames of guests.",
    {YES,    YES,   YES,   YES,   YES,   NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   NO,    NO,    NO,    NO }},
  {
    "Hostnames", POW_HOST, "Ability to see hostnames on the WHO list",
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YESEQ, YESLT, YES,   NO,    NO,    NO,    NO }},
  {
    "Incoming", POW_INCOMING, "Ability to connect net to non-players",
    {YES,    YES,   YES,   NO,    NO,    NO,     NO,   NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,    NO,   YES,   NO,    NO }},
  {
    "Join", POW_JOIN, "Ability to 'join' players",
    {YES,    YES,   YESLT, YESLT, YES,   NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YESEQ, YES,   YES,   NO,    YES,   NO,    NO }},
  {
    "Member", POW_MEMBER, "Ability to change your name and password",
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   NO,     NO,   NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   NO,     NO,   NO }},
  {
    "Modify", POW_MODIFY, "Ability to modify other people's objects",
    {YESEQ,  YESEQ, YESLT, YESLT, NO,    NO,    NO,    NO,    NO,    NO },
    {YESEQ,  YESEQ, YESEQ, YESEQ, YESEQ, YESEQ, NO,    NO,    NO,    NO }},
  {
    "Money", POW_MONEY, "Power to have INFINITE money",
    {YES,    YES,  NO,     NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,  YES,    NO,    NO,    YES,   NO,    YES,   NO,    NO }},
  {
    "Newpassword", POW_NEWPASS, "Ability to use the @newpassword command",
    {YESLT,  YESLT, NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YESEQ,  YESLT, YESLT, NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Noslay", POW_NOSLAY, "Power to not be killed",
    {YES,    YES,   YES,   YES,   YES,   NO,    NO,    NO,    NO,    YES},
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   YES,   YES,   YES}},
  {
    "Noquota", POW_NOQUOTA, "Power to have INFINITE quota (unused)",
    {YES,    YES,  NO,     NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,  YES,    NO,    NO,    NO,    NO,    YES,   NO,    NO }},
  {
    "Nuke", POW_NUKE, "Power to @nuke other characters",
    {YESLT,  NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YESLT,  YESLT, NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Outgoing", POW_OUTGOING, "Ability to initiate net connections.",
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   YES,   NO,    NO }},
  {
    "Pcreate", POW_PCREATE, "Power to create new characters",
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Poor", POW_POOR, "Power to use the @poor command",
    {NO,     NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Queue", POW_QUEUE, "Power to see everyone's commands in the queue",
    {YES,    YES,   YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   NO,    NO,    NO }},
  {
    "Remote", POW_REMOTE, "Ability to do remote whisper, @pemit, etc.",
    {YES,    YES,   YES,   YES,   YES,   NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   YES,   YES,   NO,    NO }},
  {
    "Security", POW_SECURITY, "Ability to do various security-related things",
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Seeatr", POW_SEEATR, "Ability to see attributes on other people's things",
    {YESEQ,  YESEQ, YESLT, YESLT, YESLT, NO,    NO,    NO,    NO,    NO },
    {YESEQ,  YESEQ, YESEQ, YESEQ, YESEQ, YESLT, NO,    NO,    NO,    NO }},
  {
    "Setpow", POW_SETPOW, "Ability to alter people's powers",
    {YESLT,  NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YESLT,  NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Setquota", POW_SETQUOTA, "Ability to change people's quotas",
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YESLT, NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Shutdown", POW_SHUTDOWN, "Ability to @shutdown the game",
    {YES,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO }},
  {
    "Slave", POW_SLAVE, "Ability to set the slave flag.", 
    {YESLT,  YESLT, NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YESLT,  YESLT, YESLT, YESLT, YESLT, NO,    NO,    NO,    NO,    NO }},
  {
    "Slay", POW_SLAY, "Ability to use the 'slay' command",
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   NO,    NO,    YESLT, NO,    YES,   NO,    NO }},
  {
    "Spoof", POW_SPOOF, "Ability to do unlimited @emit etc",
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   NO,    YES,   NO,    YES,   NO,    NO }},
  {
    "Stats", POW_STATS, "Ability to @stat other ppl",
    {YES,    YES,   YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   NO,    NO,    NO,    NO }},
  {
    "Steal", POW_STEAL, "Ability to give negative amounts of credits",
    {YES,    YES,   YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YES,   YES,   NO,    YES,   NO,    NO }},
  {
    "Summon", POW_SUMMON, "Ability to 'summon' other players",
    {YES,    YES,   YESLT, YESLT, YESLT, NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YESEQ, YESEQ, YESLT, NO,    NO,    NO,    NO }},
  {
    "Teleport", POW_TELEPORT, "Ability to use unlimited @tel",
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   NO,    NO,    NO,    NO,    YES,   NO,    NO }},
  {
    "Who", POW_WHO, "Ability to see classes and hidden players on the WHO list",
    {YES,    YES,   YESLT, NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   YESEQ, NO,    NO,    NO,    NO,    NO }},
  {
    "WizAttributes", POW_WATTR, "Ability to set Last, Queue, etc",
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   YES,   NO,    NO,    NO,    YES,   NO,    NO }},
  {
    "WizFlags", POW_WFLAGS, "Ability to set Temple, etc",
    {YES,    YES,   NO,    NO,    NO,    NO,    NO,    NO,    NO,    NO },
    {YES,    YES,   YES,   NO,    NO,    NO,    NO,    YES,   NO,    NO }},
};
