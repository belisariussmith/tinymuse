// $Id: com.c,v 1.3 1993/08/22 04:53:48 nils Exp $
// with many changes from Redone
// Dutchie Branch 2.0

#include "tinymuse.h"
#include "config.h"
#include "externs.h"

static char *get_com_title P((dbref)); // 100% Redone

static char *get_com_title(dbref player)
{
    static char buf[BUF_SIZE];

    strcpy(buf,":");

    if ((*atr_get(player,A_CTITLE)))
    {
	sprintf(buf," %s:",atr_get(player,A_CTITLE));
    }

    return buf;
}

// chan can be either an alias or a chan name <Redone>
extern int is_on_channel(dbref player, char *chan)
{
    char *clist;
    char *clbegin;
    char *calias;
    int l=0; 
    static char buf[BUF_SIZE+1];

    sprintf(buf,"%s ",atr_get(player,A_CHANNEL));
    clbegin=clist=buf;

    while(*clist)
    {
        l=0;

        if ((calias=strchr(clist,' ')))
            *calias ='\0';

        if (strchr(clist,';')) { // check for an alias
            calias=strchr(clist,';');
            *calias ='\0';
            calias++;   
            l=strlen(calias)+1;
        }
        if ((!strcmp(clist,chan)) || (!strcmp(calias,chan))) {
           return (clist-clbegin);
        }
        clist+=strlen(clist)+1+l;
    }

    return -1;
}

// chan can be only a chan name <Redone>
extern int pure_is_on_channel(dbref player, char *chan)
{
    char *clist;
    char *clbegin;
    char *calias;
    int l=0; 
    static char buf[BUF_SIZE+1];

    sprintf(buf,"%s ", atr_get(player, A_CHANNEL));
    clbegin = clist = buf;

    while(*clist)
    {
        l = 0;

        if ((calias=strchr(clist,' ')))
        {
            *calias ='\0';
        }

        // check for an alias
        if (strchr(clist,';'))
        {
            calias=strchr(clist,';');
            *calias ='\0';
            calias++;   
            l=strlen(calias)+1;
        }

        if (!strcmp(clist,chan))
        {
           // only difference with is_on_channel()
           return (clist-clbegin);
        }
        clist += strlen(clist)+1+l;
    }

    return -1;
}

// 100% Redone
extern char *unparse_com_alias(dbref player, char *chan)
{
    static char buf[BUF_SIZE];
    char *clist;
    int i=0; // the default channel

    strcpy(buf,atr_get(player,A_CHANNEL));
    clist = buf;

    if ((*chan)) 
    {
       i = is_on_channel(player,chan); 
    }
 
    if (i >= 0)
    {
       clist = buf+i;  

       if (strchr(clist,' '))
       {
          *strchr(clist,' ')='\0';
       }
       if (strchr(clist,';'))
       {
          *strchr(clist,';')='\0';
       }
    }
    else
    {
        *clist='\0';
    }
    
    return clist;    
}

extern void com_send(char *channel, char *message)
{	
    struct descriptor_data *d;
 
    for (d = descriptor_list; d; d=d->next)
    {
        if (d->state==CONNECTED && d->player>0 && pure_is_on_channel(d->player, channel)>=0)
        {
            queue_string(d, message); 
            queue_string(d, "\n");
        }
    }
}

void com_send2(dbref sender, char *channel, char *message)
{
    static char buf[BUF_SIZE];
    static char buf2[BUF_SIZE];
    struct descriptor_data *d;
 
    strcpy(buf, unparse_com_alias(sender, channel));

    if (buf[0] == '\0')
    {
        return;
    }
 
    for (d = descriptor_list; d; d=d->next)
    {
        if (d->state == CONNECTED && d->player > 0 && pure_is_on_channel(d->player, buf) >= 0)
        {
            sprintf(buf2, "[%s] %s", buf, spname(sender));
            queue_string(d, buf2); 
            queue_string(d, message); 
            queue_string(d, "\n");
        }
    }
}

static void com_who P((dbref, char *));
static void com_who(dbref player, char *channel)
{
    static char buf[BUF_SIZE];
    struct descriptor_data *d;
    int h = 0;
 
    strcpy(buf, unparse_com_alias(player, channel));

    if (buf[0]=='\0') strcpy(buf,channel); // if yur not on it yurself
  
    for (d = descriptor_list; d; d=d->next)
    {
        if (d->state == CONNECTED && d->player>0 && pure_is_on_channel(d->player, buf)>=0)
        {
            if (!could_doit(player,d->player,A_LHIDE))
            {
                if (controls(player,d->player,POW_WHO))
                {
                    if (controls(player,d->player,POW_EXAMINE) || (db[d->player].flags & SEE_OK))
                    {
                        send_message(player, "%s is on channel %s (HIDDEN)", unparse_object(player, d->player), buf);
                    }
                    else
                    {
                        send_message(player, "%s is on channel %s (HIDDEN)", spname(d->player), buf);
                    }  
                } 
	        h++; 
            }
            else
            {
                if (controls(player, d->player,POW_EXAMINE) || (db[d->player].flags & SEE_OK))
                {
                    send_message(player, "%s is on channel %s.", unparse_object(player,d->player), buf);
                }
                else
                {
                    send_message(player, "%s is on channel %s.", spname(d->player), buf);
                }  
            } 
        }
    }

    if (h == 0)
    {
        send_message(player, "*** There are no hidden players on this channel.");
    }
    else if (h == 1)
    {
        send_message(player, "*** There is one hidden player on this channel.");
    }
    else
    {
        send_message(player, "*** There are %d hidden players on this channel.", h);
    }

    send_message(player, "--- %s ---", buf);
}


void do_com(dbref player, char *arg1, char *arg2)
{
    static char buf[BUF_SIZE];

    if (!*arg1)
    {
        // default channel
        char *s;
        strcpy(buf,atr_get(player,A_CHANNEL));

        if ((s = strchr(buf,' ')))
        {
            *s = '\0';
        }

        if ((s = strchr(buf,';')))
        {
            // ignore the alias
            *s='\0';
        }

        // the first chan in the list  
        arg1 = buf;
    } 

    if (!*arg1)
    {
        send_message(player, "no channel.");
        return;
    }

    if (strchr(arg1,' '))
    {
        send_message(player, "you're spacey.");
        return;
    }

    if (strchr(arg1,';'))
    {
        send_message(player, "A channel name can not have ';' in it.");
        return;
    }
  
    if (!string_compare(arg2, "who"))
    {
        com_who(player, arg1);
    }
    else
    {
        char buf[BUF_SIZE*2];
        char Marker[3];
    
        if (Typeof(player) != TYPE_PLAYER && !power(player,POW_SPOOF))
        {
            send_message(player, "non-players can not send +com messsages.");
            return;
        }
    
        sprintf(Marker,":");

        if (*arg2 == ':')
        {
            Marker[0]='\0';
        }
        else if (*arg2 == ';')
        {
            sprintf(Marker,"'s");
        }
 
        sprintf(buf, "%s %s", 
                     ((Marker[0] == ':')?(get_com_title(player)):Marker),
                     ((*arg2 == ':')||(*arg2 == ';'))?(arg2+1):arg2);
	    
        com_send2(player, arg1, buf);

        if (is_on_channel(player, arg1) < 0)
        {
            send_message(player, "your +com has been sent! yay!");
        }
    }
}

// 100% Redone 
extern void do_ctitle(dbref player, char *ctitle)
{
    char buf[BUF_SIZE];

    if (Typeof(player) != TYPE_PLAYER)
    {
        send_message(player, "Non-players can not set a com channel title.");
        return;
    }

    if (*ctitle == '\0')
    {
       atr_clr(player, A_CTITLE);
       send_message(player, "Com channel title message removed.");
       return;
    }
  
    sprintf(buf, "<%s>", ctitle);
    atr_add(player, A_CTITLE, buf);
    sprintf(buf, "Com channel title message set to: <%s>", ctitle);
    send_message(player, buf);
}

// Redone of course
void do_channel(dbref player, char *arg1)
{
    int i;
    ptype k;
    char chan[BUF_SIZE];
    char buf[BUF_SIZE];
    char buf2[BUF_SIZE];	
    char buf3[BUF_SIZE];
    char *calias;
    char *end;
    char *old_chan;

    if (*arg1 == '\0')
    {
        if (*atr_get(player, A_CHANNEL))
        {
            strcpy(buf, atr_get(player, A_CHANNEL) );
            calias = buf;

            while ( ((calias=strchr(calias,' '))!=0) && (*(calias+1)!='\0') )
            {
 	        *calias='\n';
                calias++;
            }

            send_message(player, "You are currently on the following channels:");
            send_message(player, "---- Channel list ----");
            send_message(player, buf);	
            send_message(player, "--- <chan>;<alias> ---");
            send_message(player, "Your default channel is: %s", unparse_com_alias(player, ""));
        }
        else
        {
            send_message(player, "You aren't currently on any channels. For a general chatting channel, turn to");
            send_message(player, "channel 'public'");
        }
    }
    else if (*arg1 == '+')
    {
        arg1++;

        if (Typeof(player) != TYPE_PLAYER)
        {
            send_message(player, "As of yet, non-players cannot talk on channels.");
            return;
        }

        if (!db[player].pows) return;
        k= *db[player].pows;
        if (*arg1 == '*' && !(k == CLASS_OFFICIAL || k == CLASS_DIR))
        {
            send_message(player, perm_denied());
            return;
        }

        if (*arg1 == '.' && !(k == CLASS_DIR || k == CLASS_ADMIN || k == CLASS_BUILDER))
        {
            send_message(player, perm_denied());
            return;
        }
        if (*arg1 == '_' && !(k == CLASS_DIR || k == CLASS_ADMIN || k == CLASS_BUILDER || k == CLASS_OFFICIAL || k == CLASS_GUIDE))
        {
            send_message(player, perm_denied());
            return;
        }

        if (strchr(arg1, ' '))
        {
            send_message(player, "You can not use spaces in a channel name");
            return;
        }    
    
        buf[0]='\0';
        strcpy(chan, arg1);

        if ((calias = strchr(chan, ';')))
        {
            *calias='\0';
            calias++;
            if (strchr(calias,';'))
            {
                send_message(player, "You can not have more than one alias per channel name");
	        return;
            }
            strcpy(buf, calias);
        }    

        if (chan[0] == '\0')
        {
            send_message(player, "what channel?");
            return;
        }

        i = is_on_channel(player, chan);
     
        if (buf[0]!='\0')
        { 
            if (is_on_channel(player, buf) >= 0)
            {
                send_message(player, "You are already using that com alias.");
                return;
            }   
        }
        else
        {
             send_message(player, "You did not include a com alias: +ch +<channelname>;<alias>");
             return;
        }
    
        if (i >= 0)
        {
            // change alias
            //strcpy(buf2, tprintf("%s;%s", unparse_com_alias(player, chan), buf));
            //strcpy(chan, buf2); 
            sprintf(chan, "%s;%s", unparse_com_alias(player, chan), buf);
            strcpy(buf, atr_get(player, A_CHANNEL));
            if (*buf)
            {
                old_chan=buf+i;
                end = strchr(buf+i,' ');
                if (!end)
                {
                   end = strchr(buf+i, '\0');
                }
                else
                {
                   *end = '\0';
                   end++;
                }
            } 

            strcpy (buf2, atr_get(player,A_CHANNEL));
            strcpy(buf2+i, end);

            if ((end=strchr(buf2,'\0')) && *--end==' ')
            {
                *end='\0';
            }

            if (buf2[0]=='\0')
            {
                atr_add (player, A_CHANNEL, arg1);
            }
            else
            {  
                if (i == 0)
                {
                    sprintf(buf3, "%s %s", chan, buf2);
                    atr_add(player, A_CHANNEL, buf3);
                }
                else
                {
                    sprintf(buf3, "%s %s", buf2, chan);
                    atr_add(player, A_CHANNEL, buf3);
                }
            } 
            send_message(player, "channel alias changed: %s", chan);

        }
        else
        {
            // add new channel 
            if (!(*atr_get(player, A_CHANNEL))) 
            {
                atr_add(player, A_CHANNEL, arg1);
            }
            else
            {
                sprintf(buf3, "%s %s", atr_get(player, A_CHANNEL), arg1);
                atr_add(player, A_CHANNEL, buf3);
            }
    
            sprintf(buf3, "[%s] %s has joined this channel.", chan, spname(player));
            com_send(chan, buf3);
	  
            if (buf[0] != '\0')
            {
                send_message(player, "%s added to your channel list with alias: %s", chan, buf);    
            }
            else 
            {
                send_message(player, "%s added to your channel list with no alias", chan);
            }
            
            send_message(player, "Your default channel is: %s", unparse_com_alias(player, ""));
             
        } // end if-else
    }
    else
    {
        // leave a channel
        int i, Minus; 

        Minus=0; 
        if (*arg1 == '-') { Minus=1; arg1++; }
    
        if (strchr(arg1,' ') || !*arg1)
        {
            send_message(player, "sigh. such sillyness. try a less spacey channel name.");
            return;
        }

        i = is_on_channel(player, arg1);
        if ( i < 0 )
        {
            send_message(player, "you aren't on that channel.");
            return;
        }

        strcpy(buf, atr_get(player, A_CHANNEL));

        if (*buf)
        {
            old_chan = buf+i;
            end = strchr(buf+i, ' ');
            if (!end)
            {
                end = strchr(buf+i, '\0');
            }
            else
            {
                *end='\0';
                 end++;
            }
        } 

        strcpy (buf2, atr_get(player, A_CHANNEL));
        strcpy(buf2+i, end);

        if ((end=strchr(buf2,'\0')) && *--end==' ') *end='\0';

        if (Minus == 1)
        { 
            static char chan[BUF_SIZE];

            strcpy(chan,unparse_com_alias(player, arg1));
 
            send_message(player, "%s has been deleted from your channel list.", chan);

            sprintf(buf, "[%s] %s has left this channel.", chan, spname(player));

            com_send(chan, buf);
            atr_add (player, A_CHANNEL, buf2);

            if (buf2[0] == '\0')
            {
                send_message(player, "You are currently on no com channel.");
            }
            else
            {  
                send_message(player, "Your default channel is: %s", unparse_com_alias(player, ""));
            } 
        }
        else
        {
            if ((!*atr_get(player, A_CHANNEL)) || (*buf2 == '\0'))
            {
                atr_add(player, A_CHANNEL, old_chan); 
            }
            else
            {
                sprintf(buf, "%s %s", old_chan, buf2);
                atr_add(player, A_CHANNEL, buf); 
            }
               
            send_message(player, "Your default channel is now: %s", unparse_com_alias(player, arg1));
        }
    }
//
//  send_message(player, "Usage:");
//  send_message(player, "  +channel +<channel>    :adds a channel");
//  send_message(player, "  +channel -<channel>    :deletes a channel");
//  send_message(player, "  +channel <channel>     :change current channel");
//  send_message(player, "  +channel               :lists your channels.");
//  send_message(player, "For a general chatting channel, try channel 'public'.");
//
}

