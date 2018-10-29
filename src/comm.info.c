#include "tinymuse.h"
#include "config.h"
#include "externs.h"

void do_info(dbref player, char *arg1)
{
    if (!string_compare(arg1, "config"))
    {
        info_config(player);
    }
    else if (!string_compare(arg1, "db"))
    {
        info_db(player);
    }
    else if (!string_compare(arg1, "funcs"))
    {
        info_funcs(player);
    }
    else
    {
        send_message(player, "usage: @info config|db|funcs");
    }
}
