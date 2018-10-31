///////////////////////////////////////////////////////////////////////////////
// credits.h                                                   TinyMUSE (@) 
///////////////////////////////////////////////////////////////////////////////
//     This code began as TinyMUSH v1.5.  It has been heavily modified
//     since that time by the following people: (from revision 1.0 to 1.8a4)
//
//     shkoo     shkoo@chezmoto.ai.mit.edu
//     Erk       erk@chezmoto.ai.mit.edu
//     Michael   michael@chezmoto.ai.mit.edu
//
//     The following have contributed as well (from revision 1.8a4 to 1.9f3):
//
//     Mark Eisenstat (Morgoth)  - meisen@musenet.org
//     Jason Hula (Power_Shaper) - <currently unavailable>
//     Rick Harby (Tabok)        - rharby@eznet.net
//     Robert Peperkamp (Redone) - rpeperkamp@envirolink.org
//
//     Picking up from 1.9f3 TinyMUSE has been modernized and overhauled 
//     sufficiently to warrant a new major version number to 2.0 by:
//
//     Belisarius Smith (Balr)   - belisarius@psu.edu
//
//     The following variable defines the current version number.
///////////////////////////////////////////////////////////////////////////////
// Notes:
//
//   The following definitions are used to implement an automatic version
//   number calculation system.  While major code changes reflect version
//   number changes in the BASE_VERSION number variable (see below), day to
//   day hacks require too much upkeep of the version number.  To simplify
//   matters, dates of code changes are used to calculate a 4 field version
//   number of the form: 'vX.X.X.X'.
//
///////////////////////////////////////////////////////////////////////////////

// If you make your own modifications to this code, please don't muck with
// our version number system as that might serve to confuse people.
// Instead, please simply uncomment the definition below for this purpose. 
//#define MODIFIED 

// Base version of this code.  In v<maj>.<min>, the major number reflects
// major code changes and major redesign.  The minor number reflects
// important code changes. 
#define BASE_VERSION   "TinyMUSE v2.0"

//#define ALPHA 
//#define BETA
#define FINAL

// These dates must be of the form MM/DD/YY
// BASE_DATE...: Date from last change to the value of BASE_VERSION 
// UPGRADE_DATE: This is the release of this code.
#define BASE_DATE      "11/18/95"
#define UPGRADE_DATE   "10/31/18"

// This is the release number for a particular day.  If this is the third 
// time mods have been released in a day, then this number should be 3
#define DAY_RELEASE    1

// This is the database version number.  Version numbers have been intoduced
// into the database to facilitate automatic database restructuring.  Unless
// you absolutely MUST change this number, avoid doing so at all costs.  When
// new versions of MUSE are released, this number may change, requiring you
// to do some fancy reworking of your database or the new MUSE version's
// code.
#define DB_VERSION    15
