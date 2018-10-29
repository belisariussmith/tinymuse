// rob.c 
// $Id: rob.c,v 1.3 1993/01/30 03:38:43 nils Exp $ 

#include "copyright.h"
#include <ctype.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

void do_giveto(dbref player, char *who, char *amnt)
{
    int amount;
    dbref recipt;

    if (!power(player, POW_MEMBER))
    {
        send_message(player, "Silly, you can't give out money!");
        return;
    }

    init_match(player, who, TYPE_PLAYER);
    match_player();
    match_absolute();
    match_neighbor();
    recipt = noisy_match_result();

    if (recipt == NOTHING)
    {
        return;
    }

    amount=atoi(amnt);

    if (amount < 1)
    {
        send_message(player,"You can only @giveto positive amounts.");
        return;
    }

    if (!payfor(player, amount))
    {
        send_message(player, "You can't pay for that much1");
        return;
    }

    giveto(recipt, amount);

    send_message(player, "Given.");
}

void do_give(dbref player, char *recipient, char *amnt)
{
    dbref who;               
    int amount;
    char buf[MAX_CMD_BUF];
    char *s;
  
    if ( Guest(db[player].owner) ) 
    {
        send_message(player, "Sorry, guests can't do that!");
        return;
    }
  
    // check recipient 
    init_match(player, recipient, TYPE_PLAYER);
    match_neighbor();
    match_me();

    if (power(player, POW_REMOTE))
    {
        match_player();
        match_absolute();
    }
  
    switch(who = match_result())
    {
        case NOTHING:
            send_message(player, "Give to whom?");
            return;
        case AMBIGUOUS:
            send_message(player, "I don't know who you mean!");
            return;
    }
  
    // make sure amount is all digits 
    for (s = amnt; *s && ((isdigit(*s)) || (*s=='-')); s++)
    ;

    // must be giving object 
    if (*s)
    {
        dbref thing;
      
        init_match(player, amnt, TYPE_THING);
        match_possession();
        match_me();

        switch(thing = match_result())
        {
            case NOTHING:
                send_message(player, "You don't have that!");
                return;
            case AMBIGUOUS:
                send_message(player, "I don't know which you mean!");
                return;
            default:     
                if ( ((Typeof(thing) == TYPE_THING) || (Typeof(thing) == TYPE_PLAYER))
                  && (   ((db[who].flags & ENTER_OK) && could_doit(player, thing, A_LOCK))
                      || (controls(player,who, POW_TELEPORT)))
                   )
                {  
	             moveto(thing,who);

                     send_message(who, "%s gave you %s.", db[player].name, db[thing].name);
                     send_message(player, "Given.");
                     send_message(thing, "%s gave you to %s.",db[player].name, db[who].name);
				  
	    }
	  else
	    send_message(player,"Permission denied.");
	}
      return;
    }

    amount = atoi(amnt);

    // do amount consistency check 
    if (amount <= 0 && !power(player, POW_STEAL))
    {
        send_message(player, "You must specify a positive number of Credits.");
        return;
    }  
  
    if (amount > 0)
    {
        if (Pennies(who) + amount > max_pennies)
        {
            send_message(player, "That player doesn't need that many Credits!");
            return;
        }
    }
  
    // try to do the give 
    if (!payfor(player, amount))
    {
        send_message(player, "You don't have that many Credits to give!");
    }
    else
    {
        // objects work differently 
        if (Typeof(who) == TYPE_THING)
        {
            int cost;

            if (amount<(cost=atoi(atr_get(who,A_COST))))
            {
                send_message(player,"Feeling poor today?");
                giveto(player,amount);
                return;
            }

            if (cost < 0)
	        return;

	    if ((amount-cost) > 0 )
            {
                sprintf(buf, "You get %d in change.", amount-cost);
	    }
	    else
            {
	        sprintf(buf, "You paid %d credits.", amount);
	    }
            send_message(player, buf);
            giveto(player, amount-cost);
            giveto(who, cost);
            did_it(player, who, A_PAY, NULL, A_OPAY, NULL, A_APAY);
            return;
        }       
        else
        {
            /* he can do it */
            send_message(player, "You give %d Credits to %s.", amount, db[who].name);

            if (Typeof(who) == TYPE_PLAYER)
            {
                send_message(who, "%s gives you %d Credits.", db[player].name, amount);
            }

            giveto(who, amount);  
            did_it(player, who, A_PAY, NULL, A_OPAY, NULL, A_APAY);
      }
  }
}

void do_slay(dbref player, char *what)
{
    dbref victim;
    char buf[MAX_CMD_BUF+100];
    char buf1[MAX_CMD_BUF+100];

    if ( ! power(player, POW_SLAY))
    {
        send_message(player, "You do not have such power.");
        return;
    }

    init_match(player, what, TYPE_PLAYER);
    match_neighbor();
    match_me();
    match_here();

    if (power(player, POW_REMOTE))
    {
        match_player();
        match_absolute();
    }

    victim = match_result();

    switch(victim)
    {
        case NOTHING:
            send_message(player, "I don't see that player here.");
            break;
        case AMBIGUOUS:
            send_message(player, "I don't know who you mean!");
            break;
        default:
            if ((Typeof(victim) != TYPE_PLAYER) && (Typeof(victim)!=TYPE_THING))
            {
                send_message(player, "Sorry, you can only kill other players.");
                //    } else if (!controls(player,victim)){
            }
            else if (Level(player) < Levnm(victim))
            {
                send_message(player, "Sorry.");
            }
            else if (power(victim, POW_NOSLAY))
            {
                // Check for power_noslay
                send_message(player, "Sorry, that person can't be slain.");
            }
            else
            {
                // go for it 
                // you killed him 
                sprintf(buf, "You killed %s!", db[victim].name);
                sprintf(buf1, "killed %s!", db[victim].name);

                if (Typeof(victim) == TYPE_THING)
                {
                    do_halt(victim, "");
                }

                did_it(player, victim, A_KILL, buf, A_OKILL, buf1, A_AKILL);

                // notify victim 
                sprintf(buf, "%s killed you!", db[player].name);
                send_message(victim, buf);

                // send him home 
                safe_tel(victim, HOME);
            }
            break;
    }
}
