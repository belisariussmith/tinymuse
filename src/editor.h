///////////////////////////////////////////////////////////////////////////////
// editor.h 
///////////////////////////////////////////////////////////////////////////////
// $Id: editor.h,v 1.2 1992/10/11 15:15:00 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#ifndef __sys_types_h
#include <sys/types.h>
#endif

// make sure we have these definitions 
#define MALLOC(result, type, number) do {				\
	if (!((result) = (type *) malloc ((number) * sizeof (type))))	\
		panic("Out of memory");					\
	} while (0)


extern struct descriptor_data *descriptor_list;
extern struct descriptor_data *dsc;
#define EPROMPT "<edit>\r\n"
#define COMMAND 0
#define INSERT  1
#define CHANGE  2
#define ADD     3
#define DELETING 4
#define QUITTING 5
