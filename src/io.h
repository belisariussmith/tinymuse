///////////////////////////////////////////////////////////////////////////////
// io.h                                                       TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: io.h,v 1.2 1992/10/11 15:15:11 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#define MAXUSER 100   // should be placed elsewhere, maybe entire headerfile

typedef struct user USER;
typedef struct lock LOCK;

struct user
{
  struct pollfd *fd;
  FIFO in;
  FIFO out;
};

extern USER users[MAXUSER];

void send();
