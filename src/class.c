// class.c - class-related functions 

#include "tinymuse.h"
#include "db.h"
#include "powers.h"
#include "externs.h"

static char *classnames[] =
{
  " ?",
  "Stowaway", "Landlubber", "Crew", "Sailor",
  "Officer", "Second_Mate", "First_Mate", "Captain",
  "General", "Admiral",
  NULL
};

char *public_classnames[] =
{
  " ?",
  "Stowaway", "Landlubber", "Crew", "Sailor",
  "Officer", "Second_Mate", "First_Mate", "Captain",
  "General", "Admiral",
  NULL
};

char *short_classnames[] =
{
  " ?",
  "Stwwy", "LnLbr", "Crew",  "Sailr", "Offcr",
  "Nmbr2", "Nmbr1", "Captn", "Genrl", "Admrl",
  NULL
};

char *short_public_classnames[] =
{
  " ?",
  "Stwwy", "LnLbr", "Crew",  "Sailr", "Offcr",
  "Nmbr2", "Nmbr1", "Captn", "Genrl", "Admrl",
  NULL
};

static char *typenames[] =
{
  "Room", "Thing", "Exit", " 0x3", " 0x4", " 0x5", " 0x6", " 0x7", "Player"
};

char *class_to_name(int class)
{
    if (class >= NUM_CLASSES || class <= 0)
    {
        return NULL;
    }

    return classnames[class];
}

char *public_class_to_name(int class)
{
    return public_classnames[class];
}

char *short_class_to_name(int class)
{
    return short_classnames[class];
}

char *short_public_class_to_name(int class)
{
    return short_public_classnames[class];
}

int name_to_class(char *name)
{
    int k;

    for (k = 0; classnames[k]; k++)
    {
        if (!string_compare(name, classnames[k]))
        {
            return k;
        }
        else if (!string_compare(name, short_classnames[k]))
        {
            return k;
        }
    }

    return 0;
}

char *type_to_name(int type)
{
    if (type >= 0 && type < 9)
    {
        return typenames[type];
    }
    else
    {
        return NULL;
    }
}

int class_to_list_pos(int type)
{
    switch(type)
    {
        case CLASS_DIR:             return 0;
        case CLASS_ADMIN:           return 1;
        case CLASS_BUILDER:         return 2;
        case CLASS_OFFICIAL:        return 3;
        case CLASS_GUIDE:           return 4;
        case CLASS_PCITIZEN:        return 5;
        case CLASS_CITIZEN:         return 6;
        case CLASS_GROUP:           return 7;
        case CLASS_VISITOR:         return 8;
        case CLASS_GUEST:           return 9;
        default:                    return 8;
    }
}
