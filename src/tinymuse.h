////////////////////////////////////////////////////////////////////////////////
// tinymuse.h                                                      TinyMUSE 2.0
////////////////////////////////////////////////////////////////////////////////
// Used for generic / universal definitions, and external declarations.
////////////////////////////////////////////////////////////////////////////////
#define NONE    0

#define FALSE   0
#define TRUE    1

// MUSE
#define MUD_PORT     4201

// String Buffers
#define BUF_SIZE        1024                  // Default string buffer
#define MAX_BUF         10000                 // Maximum size allowed
#define MAX_COMMAND_LEN 1000                  // Len of cmd arg to process_command()
#define MAX_CMD_BUF     ((MAX_COMMAND_LEN)*8) // Pointless? Use MAX_BUF?

// Server Exit Status
#define STATUS_NONE   0                       // "Normal" termination of MUD
#define STATUS_REBOOT 1                       // Reboot the MUD

// Lock Types/Status
#define LOCKED        0
#define UNLOCKED      1
#define COMPLICATED   2

// Sexes
#define NUM_GENDERS 3                         // Number of genders

#define NEUTRAL 0                             // Genderless
#define MALE    1                             // Male
#define FEMALE  2                             // Female

// Movement
#define NUM_MOVES 3                           // Number of movement types

#define MOVE_HOME     2                       // Recall / Go Home
#define MOVE_WALK     0                       // Walking
#define MOVE_TELEPORT 1                       // Teleporting

// Quota
#define NO_QUOTA      1                       // Turn off the quota system

// Typedefs
typedef int dbref;                            // offset into database
typedef int dbref_t;                          // FUTURE: replacement for dbref

// External declarations
extern void send_message(dbref player, const char* format, ...);

