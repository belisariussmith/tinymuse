///////////////////////////////////////////////////////////////////////////////
// net.h 
///////////////////////////////////////////////////////////////////////////////
// $Id: net.h,v 1.3 1993/08/22 04:54:06 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#ifndef _NET_H
#define _NET_H
#ifndef __sys_types_h
#include <sys/types.h>
#endif
#ifndef IPPROTO_IP
#include <netinet/in.h>
#endif

struct text_block {
  int nchars;
  struct text_block *nxt;
  char *start;
  char *buf;
};

struct text_queue {
  struct text_block *head;
  struct text_block **tail;
};

enum descriptor_state {
  WAITCONNECT, WAITPASS, CONNECTED, RELOADCONNECT
};

struct descriptor_data {
  int descriptor;
  enum descriptor_state state;
  int concid;
  int cstatus;
#define C_CCONTROL 1
#define C_REMOTE 2
  struct descriptor_data *parent; // for C_REMOTE stuff
  char addr[51];
  dbref player;
  char *output_prefix;
  char *output_suffix;
  int output_size;
  struct text_queue output;
  struct text_queue input;
  char *raw_input;
  char *raw_input_at;
  long connected_at;
  long last_time;
  int quota;
  struct sockaddr_in address;
  struct descriptor_data *next;
  struct descriptor_data **prev;
  char *charname;		// for non-echoing passwords
  char *user;			// identd username.
};

extern struct descriptor_data *descriptor_list;
#endif
