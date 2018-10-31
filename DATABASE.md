# TinyMUSE Database

A TinyMUSE database contains a collection of 4 different types of objects: rooms, exits, things, and players.

#### _This document is currently incomplete_ and written by Belisarius Smith

## Object Formats
<pre>
    Object
    {
        &&lt;objectid:dbref&gt;
        &lt;name:string&gt;
        &lt;location:integer&gt;
        &lt;zone:integer&gt;
        &lt;contents:integer&gt;
        &lt;exits:integer&gt;
        &lt;link:integer&gt;
        &lt;next:integer&gt;
        &lt;owner:integer&gt;
        <strong>&lt;powers:string&gt;**</strong>
        Attribute List
        {
            {
                >Attribute ID#
                &lt;objectid:dbref&gt;
                &lt;attribute:string&gt;
            }
            { ... }
            <
        }
        &lt;parents:list&gt;
        &lt;children:list&gt;
        Attribute Definitions
        {
            {
                /#
                integer
                string
            } 
            { ... }
        }
        \
    }
</pre>

_<strong>**</strong> This line only exists in <strong>Player</strong> Objects_

#### dbref

Consider this the ID# of the database object. It will be the position in the 
array that contains all of the objects in the MUD.

#### name

The name of the of the object

#### location

The object containing this object

_Rooms should be located in themselves, in other words the location of a Room is always itself_

#### zone

The object's zone

#### contents

#### exits

#### link

-1 indicates that there is no link

#### next

-1 indicates that there is no next

#### owner

The owner of the object. 

_Players should own themselves._

#### flags
<pre>
* Contains the object type
** TYPE_ROOM
** TYPE_THING
** TYPE_EXIT
** NOTYPE
** TYPE_PLAYER
</pre>
##### Universal
<pre>
* CHOWN OK        0x20
* DARK            0x40       // contents of room are not printed
* STICKY          0x100      // this object goes home when dropped
* HAVEN           0x400      // object can't execute commands
* INHERIT POWERS  0x2000     // gives object powers of its owner
* GOING           0x4000     // object is available for recycling
* FUCKEDUP        0x8000     // problem reported with object.
* PUPPET          0x20000
* LINK OK         0x40000    // anybody can link to this room
* ENTER OK        0x80000
* SEE OK          0x100000
* CONNECT         0x200000
* OPAQUE          0x800000
* QUIET           0x1000000
* BEARING         0x8000000
</pre>
Each object type has its own bitflags 

##### Things
<pre>
* THING KEY       0x10
* THING LIGHT     0x80
* THING DEST OK   0x200
* THING SACROK    0x1000
* THING DIG OK    0x2000000
</pre>
##### Exits
<pre>
* EXIT LIGHT      0x10
</pre>
##### Players
<pre>
* PLAYER NEWBIE   0x10
* PLAYER SLAVE    0x80
* PLAYER MORTAL   0x800
* PLAYER ANSI     0x1000
* PLAYER TERSE    0x400000
* PLAYER NO WALLS 0x2000000
* PLAYER NO COM   0x20000000
* PLAYER NO ANN   0x40000000
</pre>
##### Rooms
<pre>
* ROOM JUMP OK    0x200
* ROOM AUDITORIUM 0x800
* ROOM FLOATING   0x1000
* ROOM DIG OK     0x2000000
</pre>
#### modtime

This is the time of the object's last modification in unix time (UNIX epoch time)

#### createtime

This is the time of the object's creation modification in unix time (UNIX epoch time)

#### powers

This is an optional line that should only exist for Player objects 

#### attributes

##### Attribute Flags
<pre>
* OSee      Players other than owner can see it
* Dark      No one can see it
* Wizard    Only wizards can change it
* UnImp     Not important -- don't save it in the db
* !Mod      Not even wizards can modify this
* Date      Date stored in universal longint form
* Inherit   Value inherited by childern
* Lock      Interpreted as a boolean expression
* Func      This is a user defined function
* Haven     For attributes set HAVEN
* Builtin   Server supplies value. not database.
* DBRef     Value displayed as dbref.
* !Mem      This isn't included in memory calculations
</pre>
##### Attribute List
<pre>
Name                Flags                        Attribute ID #
---------------------------------------------------------------------
* Osucc             Inherit                            1
* Ofail             Inherit                            2
* Fail              Inherit                            3
* Succ              Inherit                            4
* Password          Wizard|Dark|!Mod                   5
* Desc              Inherit                            6
* Sex               Inherit|OSee                       7
* Odrop             Inherit                            8
* Drop              Inherit                            9
* Incoming          Inherit                            10
* Color             Inherit|OSee                       11
* Asucc             Inherit                            12
* Afail             Inherit                            13
* Adrop             Inherit                            14
* Does              Inherit                            16
* Charges           Inherit                            17
* Runout            Inherit                            18
* Startup           Inherit|Wizard                     19
* Aclone            Inherit                            20
* Apay              Inherit                            21
* Opay              Inherit                            22
* Pay               Inherit                            23
* Cost              Inherit                            24
* Lastdisc          Wizard|Date|OSee                   25
* Listen            Inherit                            26
* Aahear            Inherit                            27
* Amhear            Inherit                            28
* Ahear             Inherit                            29
* Lastconn          Wizard|Date|OSee                   30
* Queue             Wizard|UnImp                       31
* IDesc             Inherit                            32
* Enter             Inherit                            33
* Oenter            Inherit                            34
* Aenter            Inherit                            35
* Adesc             Inherit                            36
* Odesc             Inherit                            37
* Rquota            Dark|!Mod|Wizard                   38
* Idle              Inherit                            39
* Away              Inherit                            40
* Mailk             Dark|!Mod|Wizard|UnImp|!Mem        41
* Alias             OSee                               42
* Efail             Inherit                            43
* Oefail            Inherit                            44
* Aefail            Inherit                            45
* It                UnImp|DBRef|!Mem                   46
* Leave             Inherit                            47
* Oleave            Inherit                            48
* Aleave            Inherit                            49
* Channel           Wizard                             50
* Quota             Dark|!Mod|UnImp|Wizard             51
* Pennies           Wizard|Dark|!Mod|!Mem              52
* Huhto             Wizard                             53
* Haven             Inherit                            54
* TZ                Inherit                            57
* Doomsday          Wizard                             58
* EMail             Wizard                             59
* Race              Inherit|Wizard|OSee                99
* Move              Inherit                            126
* Omove             Inherit                            127
* Amove             Inherit                            128
* Lock              Inherit|Lock                       129
* LEnter            Inherit|Lock                       130
* LUse              Inherit|Lock                       131
* Ufail             Inherit                            132
* Oufail            Inherit                            133
* Aufail            Inherit                            134
* Oconnect          Inherit                            135
* Aconnect          Inherit                            136
* Odisconnect       Inherit                            137
* Adisconnect       Inherit                            138
* Columns           Inherit                            139
* who_flags         Inherit                            140
* who_names         Inherit                            141
* Apage             Inherit                            142
* Apemit            Inherit                            143
* Awhisper          Inherit                            144
* Use               Inherit                            155
* OUse              Inherit                            156
* AUse              Inherit                            157
* LHide             Inherit|Lock                       158
* LPage             Inherit|Lock                       159
* LLeave            Inherit|Lock                       161
* Lfail             Inherit                            162
* Olfail            Inherit                            163
* Alfail            Inherit                            164
* LSound            Inherit|Lock                       165
* Sfail             Inherit                            166
* Osfail            Inherit                            167
* Asfail            Inherit                            168
* Doing             Inherit|OSee                       169
* Users             Wizard|DBRef                       170
* Defown            Inherit|DBRef                      171
* Warnings          Inherit                            172
* WInhibit          Inherit                            173
* ANews             Inherit                            174
* LastSites         Dark|Wizard|!Mod                   175
* LastClass         Dark|Wizard|!Mod                   176
* First             OSee|Wizard                        177
* WhoIs             Inherit|OSee                       178
* RName             Wizard                             179
* Connects          OSee|Wizard|!Mod                   180
* Leader            Lock|Wizard                        181
* AIDesc            Inherit                            182
* OIDesc            Inherit                            183
* Caption           Inherit|OSee                       184
* Creator           Wizard|!Mod                        185
* Parent            Inherit                            186
* AParent           Inherit                            187
* OParent           Inherit                            188
* UnParent          Inherit                            189
* AUnParent         Inherit                            190
* OUnParent         Inherit                            191
* Follow            DBRef                              192
* LFollow           Lock                               193
* Bytesused         UnImp|Wizard|!Mod|!Mem             194
* Bytesmax          Wizard                             195
* Kill              Inherit                            196
* AKill             Inherit                            197
* OKill             Inherit                            198
* Trace             Wizard|!Mod|Dark                   199
* BoardInfo         OSee|Wizard                        200
* ComTitle          Wizard                             201
* Location          Builtin|DBRef                      256
* Link              Builtin|DBRef                      257
* Contents          Builtin|DBRef                      258
* Exits             Builtin|DBRef                      259
* Children          Builtin|DBRef                      260
* Parents           Builtin|DBRef                      261
* Owner             Builtin|DBRef                      262
* Name              Builtin                            263
* Flags             Builtin                            264
* Zone              Builtin|DBRef                      265
* Next              Builtin|DBRef                      266
* Modified          Builtin|Date                       267
* Created           Builtin|Date                       268
* Longflags         Builtin                            269
</pre

#### \

This is the terminator

