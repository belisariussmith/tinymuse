///////////////////////////////////////////////////////////////////////////////
// attrib.h                                                     TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// if you're going to add new attributes, use numbers 1000 and up. that way
// you won't have to go to renumbering pains when upgrading. make sure
// you change max_atrnum in db.c. 
///////////////////////////////////////////////////////////////////////////////

#define OSEE AF_OSEE
#define SDARK AF_DARK
#define WIZARD AF_WIZARD
#define UNIMP AF_UNIMP
#define NOMOD AF_NOMOD
#define DATE AF_DATE
#define INH AF_INHERIT
#define LOCK AF_LOCK
#define FUNC AF_FUNC
#define DBREF AF_DBREF
#define NOMEM AF_NOMEM
#define AHAVEN AF_HAVEN

DOATTR(A_OSUCC,		  "Osucc",	     INH,			1)
DOATTR(A_OFAIL,		  "Ofail",	     INH,			2)
DOATTR(A_FAIL,		  "Fail",	     INH,			3)
DOATTR(A_SUCC,		  "Succ",	     INH,			4)
DOATTR(A_PASS,		  "Password",	     WIZARD|SDARK|NOMOD,	5)
DOATTR(A_DESC,		  "Desc",	     INH,			6)
DOATTR(A_SEX,		  "Sex",	     INH|OSEE,			7)
DOATTR(A_ODROP,		  "Odrop",	     INH,			8)
DOATTR(A_DROP,		  "Drop",	     INH,			9)
DOATTR(A_INCOMING,	  "Incoming",	     INH,			10)
DOATTR(A_COLOR,		  "Color",	     INH|OSEE,			11)
DOATTR(A_ASUCC,		  "Asucc",	     INH,			12)
DOATTR(A_AFAIL,		  "Afail",	     INH,			13)
DOATTR(A_ADROP,		  "Adrop",	     INH,			14)
DOATTR(A_DOES,		  "Does",	     INH,			16)
DOATTR(A_CHARGES,	  "Charges",	     INH,			17)
DOATTR(A_RUNOUT,	  "Runout",	     INH,			18)
DOATTR(A_STARTUP,	  "Startup",	     INH|WIZARD,		19)
DOATTR(A_ACLONE,	  "Aclone",	     INH,			20)
DOATTR(A_APAY,		  "Apay",	     INH,			21)
DOATTR(A_OPAY,		  "Opay",	     INH,			22)
DOATTR(A_PAY,		  "Pay",	     INH,			23)
DOATTR(A_COST,		  "Cost",	     INH,			24)
DOATTR(A_LASTDISC,	  "Lastdisc",	     WIZARD|DATE|OSEE,		25)
DOATTR(A_LISTEN,	  "Listen",	     INH,			26)
DOATTR(A_AAHEAR,	  "Aahear",	     INH,			27)
DOATTR(A_AMHEAR,	  "Amhear",	     INH,			28)
DOATTR(A_AHEAR,		  "Ahear",	     INH,			29)
DOATTR(A_LASTCONN,	  "Lastconn",	     WIZARD|DATE|OSEE,		30)
DOATTR(A_QUEUE,		  "Queue",	     WIZARD|UNIMP,		31)
DOATTR(A_IDESC,		  "IDesc",	     INH,			32)
DOATTR(A_ENTER,		  "Enter",	     INH,			33)
DOATTR(A_OENTER,	  "Oenter",	     INH,			34)
DOATTR(A_AENTER,	  "Aenter",	     INH,			35)
DOATTR(A_ADESC,		  "Adesc",	     INH,			36)
DOATTR(A_ODESC,		  "Odesc",	     INH,			37)
DOATTR(A_RQUOTA,	  "Rquota",	     SDARK|NOMOD|WIZARD,	38)
DOATTR(A_IDLE,		  "Idle",	     INH,			39)
DOATTR(A_AWAY,		  "Away",	     INH,			40)
DOATTR(A_MAILK,		  "Mailk",	     SDARK|NOMOD|WIZARD|UNIMP|NOMEM,41)
DOATTR(A_ALIAS,		  "Alias",	     OSEE,			42)
DOATTR(A_EFAIL,		  "Efail",	     INH,			43)
DOATTR(A_OEFAIL,	  "Oefail",	     INH,			44)
DOATTR(A_AEFAIL,	  "Aefail",	     INH,			45)
DOATTR(A_IT,		  "It",		     UNIMP|DBREF|NOMEM,		46)
DOATTR(A_LEAVE,		  "Leave",	     INH,			47)
DOATTR(A_OLEAVE,	  "Oleave",	     INH,			48)
DOATTR(A_ALEAVE,	  "Aleave",	     INH,			49)
DOATTR(A_CHANNEL,	  "Channel",	     WIZARD,			50)
DOATTR(A_QUOTA,		  "Quota",	     SDARK|NOMOD|UNIMP|WIZARD,	51)
DOATTR(A_PENNIES,	  "Pennies",	     WIZARD|SDARK|NOMOD|NOMEM,	52)
DOATTR(A_HUHTO,		  "Huhto",	     WIZARD,			53)
DOATTR(A_HAVEN,		  "Haven",	     INH,			54)
DOATTR(A_TZ,		  "TZ",		     INH,			57)
DOATTR(A_DOOMSDAY,	  "Doomsday",	     WIZARD,			58)
DOATTR(A_EMAIL,		  "EMail",	     WIZARD,			59)

DOATTR(A_RACE,		  "Race",	     INH|WIZARD|OSEE,		99)
#ifndef DECLARE_ATTR
DOATTR(A_V[0],		  "Va",		     INH,			100+0)
DOATTR(A_V[1],		  "Vb",		     INH,			100+1)
DOATTR(A_V[2],		  "Vc",		     INH,			100+2)
DOATTR(A_V[3],		  "Vd",		     INH,			100+3)
DOATTR(A_V[4],		  "Ve",		     INH,			100+4)
DOATTR(A_V[5],		  "Vf",		     INH,			100+5)
DOATTR(A_V[6],		  "Vg",		     INH,			100+6)
DOATTR(A_V[7],		  "Vh",		     INH,			100+7)
DOATTR(A_V[8],		  "Vi",		     INH,			100+8)
DOATTR(A_V[9],		  "Vj",		     INH,			100+9)
DOATTR(A_V[10],		  "Vk",		     INH,			100+10)
DOATTR(A_V[11],		  "Vl",		     INH,			100+11)
DOATTR(A_V[12],		  "Vm",		     INH,			100+12)
DOATTR(A_V[13],		  "Vn",		     INH,			100+13)
DOATTR(A_V[14],		  "Vo",		     INH,			100+14)
DOATTR(A_V[15],		  "Vp",		     INH,			100+15)
DOATTR(A_V[16],		  "Vq",		     INH,			100+16)
DOATTR(A_V[17],		  "Vr",		     INH,			100+17)
DOATTR(A_V[18],		  "Vs",		     INH,			100+18)
DOATTR(A_V[19],		  "Vt",		     INH,			100+19)
DOATTR(A_V[20],		  "Vu",		     INH,			100+20)
DOATTR(A_V[21],		  "Vv",		     INH,			100+21)
DOATTR(A_V[22],		  "Vw",		     INH,			100+22)
DOATTR(A_V[23],		  "Vx",		     INH,			100+23)
DOATTR(A_V[24],		  "Vy",		     INH,			100+24)
DOATTR(A_V[25],		  "Vz",		     INH,			100+25)
#else /* declare_attr */
DOATTR(A_V[26],		  "v attributes",    -1,			   -1);
#endif /* declare_attr */
DOATTR(A_MOVE,		  "Move",	     INH,			126)
DOATTR(A_OMOVE,		  "Omove",	     INH,			127)
DOATTR(A_AMOVE,		  "Amove",	     INH,			128)
DOATTR(A_LOCK,		  "Lock",	     INH|LOCK,			129)
DOATTR(A_ELOCK,		  "LEnter",	     INH|LOCK,			130)
DOATTR(A_ULOCK,		  "LUse",	     INH|LOCK,			131)
DOATTR(A_UFAIL,		  "Ufail",	     INH,			132)
DOATTR(A_OUFAIL,	  "Oufail",	     INH,			133)
DOATTR(A_AUFAIL,	  "Aufail",	     INH,			134)
DOATTR(A_OCONN,		  "Oconnect",	     INH,			135)
DOATTR(A_ACONN,		  "Aconnect",	     INH,			136)
DOATTR(A_ODISC,		  "Odisconnect",     INH,			137)
DOATTR(A_ADISC,		  "Adisconnect",     INH,			138)
DOATTR(A_COLUMNS,	  "Columns",	     INH,			139)
DOATTR(A_WHOFLAGS,	  "who_flags",	     INH,			140)
DOATTR(A_WHONAMES,	  "who_names",	     INH,			141)
DOATTR(A_APAGE,		  "Apage",	     INH,			142)
DOATTR(A_APEMIT,	  "Apemit",	     INH,			143)
DOATTR(A_AWHISPER,	  "Awhisper",	     INH,			144)
DOATTR(A_USE,		  "Use",	     INH,			155)
DOATTR(A_OUSE,		  "OUse",	     INH,			156)
DOATTR(A_AUSE,		  "AUse",	     INH,			157)
DOATTR(A_LHIDE,		  "LHide",	     INH|LOCK,			158)
DOATTR(A_LPAGE,		  "LPage",	     INH|LOCK,			159)
DOATTR(A_LLOCK,		  "LLeave",	     INH|LOCK,			161)
DOATTR(A_LFAIL,		  "Lfail",	     INH,			162)
DOATTR(A_OLFAIL,	  "Olfail",	     INH,			163)
DOATTR(A_ALFAIL,	  "Alfail",	     INH,			164)
DOATTR(A_SLOCK,		  "LSound",	     INH|LOCK,			165)
DOATTR(A_SFAIL,		  "Sfail",	     INH,			166)
DOATTR(A_OSFAIL,	  "Osfail",	     INH,			167)
DOATTR(A_ASFAIL,	  "Asfail",	     INH,			168)
DOATTR(A_DOING,		  "Doing",	     INH|OSEE,			169)
DOATTR(A_USERS,		  "Users",	     WIZARD|DBREF,		170)
DOATTR(A_DEFOWN,	  "Defown",	     INH|DBREF,			171)
DOATTR(A_WARNINGS,	  "Warnings",	     INH,			172)
DOATTR(A_WINHIBIT,	  "WInhibit",	     INH,			173)
DOATTR(A_ANEWS,		  "ANews",	     INH,			174)
DOATTR(A_LASTSITES,	  "LastSites",	     SDARK|WIZARD|NOMOD,	175)
DOATTR(A_LASTCLASS,	  "LastClass",	     SDARK|WIZARD|NOMOD,	176)
DOATTR(A_FIRST,		  "First",	     OSEE|WIZARD,		177)
DOATTR(A_WHOIS,		  "WhoIs",	     INH|OSEE,			178)
DOATTR(A_RNAME,		  "RName",	     WIZARD,			179)
DOATTR(A_CONNECTS,	  "Connects",	     OSEE|WIZARD|NOMOD,		180)
DOATTR(A_LEADER,	  "Leader",	     LOCK|WIZARD,		181)
DOATTR(A_AIDESC,          "AIDesc",          INH,                       182)
DOATTR(A_OIDESC,          "OIDesc",          INH,                       183)
DOATTR(A_CAPTION,	  "Caption",	     INH|OSEE,			184)
DOATTR(A_CREATOR,	  "Creator",	     WIZARD|NOMOD,		185)
DOATTR(A_PARENT,	  "Parent",	     INH,			186)
DOATTR(A_APARENT,	  "AParent",	     INH,			187)
DOATTR(A_OPARENT,	  "OParent",	     INH,			188)
DOATTR(A_UNPARENT,	  "UnParent",	     INH,			189)
DOATTR(A_AUNPARENT,	  "AUnParent",	     INH,			190)
DOATTR(A_OUNPARENT,	  "OUnParent",	     INH,			191)
DOATTR(A_FOLLOW,	  "Follow",	     DBREF,			192)
DOATTR(A_LFOLLOW,	  "LFollow",	     LOCK,			193)
DOATTR(A_BYTESUSED,	  "Bytesused",	     UNIMP|WIZARD|NOMOD|NOMEM,	194)
DOATTR(A_BYTESMAX,	  "Bytesmax",	     WIZARD,			195)
DOATTR(A_KILL,		  "Kill",	     INH,			196)
DOATTR(A_AKILL,		  "AKill",	     INH,			197)
DOATTR(A_OKILL,		  "OKill",	     INH,			198)
DOATTR(A_TRACE,		  "Trace",	     WIZARD|NOMOD|SDARK,        199)
DOATTR(A_BOARDINFO,	  "BoardInfo",       OSEE|WIZARD,		200)
DOATTR(A_CTITLE,	  "ComTitle",	     WIZARD,			201)
DOATTR(A_LOCATION,	  "Location",	     AF_BUILTIN|DBREF,		256)
DOATTR(A_LINK,		  "Link",	     AF_BUILTIN|DBREF,		257)
DOATTR(A_CONTENTS,	  "Contents",	     AF_BUILTIN|DBREF,		258)
DOATTR(A_EXITS,		  "Exits",	     AF_BUILTIN|DBREF,		259)
DOATTR(A_CHILDREN,	  "Children",	     AF_BUILTIN|DBREF,		260)
DOATTR(A_PARENTS,	  "Parents",	     AF_BUILTIN|DBREF,		261)
DOATTR(A_OWNER,		  "Owner",	     AF_BUILTIN|DBREF,		262)
DOATTR(A_NAME,		  "Name",	     AF_BUILTIN,		263)
DOATTR(A_FLAGS,		  "Flags",	     AF_BUILTIN,		264)
DOATTR(A_ZONE,		  "Zone",	     AF_BUILTIN|DBREF,		265)
DOATTR(A_NEXT,		  "Next",	     AF_BUILTIN|DBREF,		266)
DOATTR(A_MODIFIED,	  "Modified",	     AF_BUILTIN|DATE,		267)
DOATTR(A_CREATED,	  "Created",	     AF_BUILTIN|DATE,		268)
DOATTR(A_LONGFLAGS,       "Longflags",       AF_BUILTIN,                269)

// Start new attribute declarations with numbers 1000 and higher.
//   Example:
//            DOATTR(A_VARIABLE, "Variable", WIZARD|SDARK, 1000)                       

#undef OSEE
#undef SDARK
#undef WIZARD
#undef UNIMP
#undef NOMOD
#undef DATE
#undef INH
#undef LOCK
#undef FUNC
#undef DBREF
#undef NOMEM
#undef AHAVEN
