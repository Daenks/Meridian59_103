Blakod Builtin Functions (C calls)
--------------

This is a list of all the C calls available in kod, i.e. the standard library.
Code examples are given for each, however some may be older examples not
present in the current codebase.

## Table of Contents
* [List Operations](#list-operations)
  * [Cons](#cons)
  * [First](#first)
  * [Rest](#rest)
  * [Last](#last)
  * [Length](#length)
  * [List](#list)
  * [Nth](#nth)
  * [AppendListElem](#appendlistelem)
  * [InsertListElem](#insertlistelem)
  * [GetListElemByClass](#getlistelembyclass)
  * [GetListNode](#getlistnode)
  * [SwapListElem](#swaplistelem)
  * [IsListMatch](#islistmatch)
  * [ListCopy](#listcopy)
  * [SetFirst](#setfirst)
  * [SetNth](#setnth)
  * [DelListElem](#dellistelem)
  * [FindListElem](#findlistelem)
  * [IsList](#islist)
  * [GetAllListNodesByClass](#getalllistnodesbyclass)
* [Communications](#communications)
  * [AddPacket](#addpacket)
  * [SendPacket](#sendpacket)
  * [SendCopyPacket](#sendcopypacket)
  * [ClearPacket](#clearpacket)
* [Class Operations](#class-operations)
  * [Create](#create)
  * [IsClass](#isclass)
  * [GetClass](#getclass)
* [Strings](#strings)
  * [StringEqual](#stringequal)
  * [StringContain](#stringcontain)
  * [StringSubstitute](#stringsubstitute)
  * [StringLength](#stringlength)
  * [StringConsistsOf](#stringconsistsof)
  * [ParseString](#parsestring)
  * [SetString](#setstring)
  * [AppendTempString](#appendtempstring)
  * [ClearTempString](#cleartempstring)
  * [GetTempString](#gettempstring)
  * [CreateString](#createstring)
  * [SetResource](#setresource)
  * [IsString](#isstring)
* [Timers](#timers)
  * [CreateTimer](#createtimer)
  * [DeleteTimer](#deletetimer)
  * [GetTimeRemaining](#gettimeremaining)
  * [IsTimer](#istimer)
* [Room Operations](#room-operations)
  * [RoomData](#roomdata)
  * [LoadRoom](#loadroom)
  * [FreeRoom](#freeroom)
  * [LineOfSightBSP](#lineofsightbsp)
  * [LineOfSightView](#lineofsightview)
  * [CanMoveInRoomBSP](#canmoveinroombsp)
  * [MoveSectorBSP](#movesectorbsp)
  * [ChangeTextureBSP](#changetexturebsp)
  * [GetLocationInfoBSP](#getlocationinfobsp)
  * [BlockerAddBSP](#blockeraddbsp)
  * [BlockerMoveBSP](#blockermovebsp)
  * [BlockerRemoveBSP](#blockerremovebsp)
  * [BlockerClearBSP](#blockerclearbsp)
  * [GetRandomPointBSP](#getrandompointbsp)
  * [GetStepTowardsBSP](#getsteptowardsbsp)
* [Hash Tables](#hash-tables)
  * [AddTableEntry](#addtableentry)
  * [CreateTable](#createtable)
  * [GetTableEntry](#gettableentry)
  * [DeleteTableEntry](#deletetableentry)
  * [DeleteTable](#deletetable)
  * [IsTable](#istable)
* [Message Passing](#message-passing)
  * [Send](#send)
  * [Post](#post)
  * [SendList](#sendlist)
  * [SendListByClass](#sendlistbyclass)
  * [SendListBreak](#sendlistbreak)
  * [SendListByClassBreak](#sendlistbyclassbreak)
* [Logging Functions](#logging-functions)
  * [GodLog](#godlog)
  * [Debug](#debug)
  * [RecordStat](#recordstat)
* [Timing Functions](#timing-functions)
  * [GetTime](#gettime)
  * [GetTickCount](#gettickcount)
  * [GetInactiveTime](#getinactivetime)
  * [GetTimeZoneOffset](#gettimezoneoffset)
* [Miscellaneous](#miscellaneous)
  * [Random](#random)
  * [Abs](#abs)
  * [Sqrt](#sqrt)
  * [Bound](#bound)
  * [IsObject](#isobject)
  * [GetSessionIP](#getsessionip)
  * [SaveGame](#savegame)
  * [LoadGame](#loadgame)
  * [RecycleUser](#recycleuser)
  * [SetClassVar](#setclassvar)
  * [MiniGameNumberToString](#minigamenumbertostring)
  * [MiniGameStringToNumber](#minigamestringtonumber)
  * [DumpStack](#dumpstack)

## List Operations
#### Cons
`Cons(expr, list)`

Cons creates a a list node. The node is added to the beginning of the list.

```
   AssessDamage(what = $,damage = $,atype = 0, aspell = 0,bonus = 0)
   "This is called when something causes damage to us"
   {
      local i;

      foreach i in plAttackers
      {
         if i = what
         {
            propagate;
         }
      }

      plAttackers = Cons(what,plAttackers);

      propagate;
   }
```

#### First
`First(list node)`

First returns the first part of a list node.
```
   liClient_cmd = First(client_msg);

   // NOTE: Should arrange in decreasing order of frequency

   if liClient_cmd = BP_REQ_MOVE
   {
      // Fix: speed, row, col, room
      iRow = Nth(client_msg,2);
      iCol = Nth(client_msg,3);
      iSpeed = Nth(client_msg,4);
      oRoom = Nth(client_msg,5);
```

#### Rest
`Rest(list node)`

Rest returns the second part of a list node.

```
   poOffer_who = what;
   plOffer_items = $;

   lNumbers = number_list;
   foreach i in item_list
   {
      if IsClass(i,&NumberItem)
      {
         oOffer_num = Create(GetClass(i),#number=Bound(First(lNumbers),
                           0,Send(i,@GetNumber)));
         plOffer_items = Cons(oOffer_num,plOffer_items);
         lNumbers = Rest(lNumbers);
      }
      else
      {
         plOffer_items = Cons(i,plOffer_items);
      }
   }
```

#### Last
`Last(list)`

Returns the last node of the specified list.

```
   // Move tombstone at end of list to the beginning.
   oTombstone = Last(plTombstones);
   plTombstones = DelListElem(plTombstones,oTombstone);
   plTombstones = Cons(oTombstone,plTombstones);
```

#### Length
`Length(list)`

Return the length of the given list.

```
   Killed(what=$, resetScenario=FALSE, guildDisbandDeath=FALSE)
   {
      local oSpell, oRoom, NumAttackers, i, lichActivated;

      NumAttackers = Length(plAttackers);

      foreach i in plAttackers
      {
         if i = what
         {
            Send(i,@MsgSendUser,#message_rsc=lich_death_blow);
         }

         if NumAttackers = 2 and i <> what
         {
            Send(i,@MsgSendUser,#message_rsc=lich_double_killer);
         }

         if NumAttackers > 2 and i <> what
         {
            Send(i,@MsgSendUser,#message_rsc=lich_helper_killer);
         }
      }
```

#### List
`List(expr1, expr2, ...)`

Create a list of its arguments. A list is a sequence of list nodes, where the
first element of the n th node contains the value of the n th expression, and
the second element contains a reference to the (n + 1) st node. The second
element of the last node contains nil. A call to List can be abbreviated by
the syntactic sugar [ expr1, expr2, ...].

```
   Constructed()
   {
      plResistances = [ [-ATCK_SPELL_FIRE, 110 ],
                        [-ATCK_SPELL_COLD, -20 ],
                        [ATCK_WEAP_MAGIC, -10 ],
                        [-ATCK_SPELL_QUAKE, 100 ]
                     ];

      piColor_Translation = Send(SYS,@EncodeTwoColorXLAT,#color1=XLAT_TO_RED);

      plSpellBook = [ [SID_FIREBALL,2,60], [SID_FIREWALL,10,70],
                      [SID_DEMENT,3,100] ];

      propagate;
   }
```

#### Nth
`Nth(list, n)`

Return the first element of the nth node in the list.

```
   lPhrases = [ TosWatcher_ad_need_champion1,
                TosWatcher_ad_need_champion2,
                TosWatcher_ad_need_champion3,
                TosWatcher_ad_need_champion4
         ];
   rand = Random(1,Length(lPhrases));

   Send(self,@Say,#message_rsc=Nth(lPhrases,rand));
```

#### AppendListElem
`AppendListElem(expr, list)`

Similar to Cons, creates a list node from expr but adds the node to the end of
the list, and returns the original list (which has the node added to the end,
so the initial list node eventually points to it). Note that as this function
adds to the end of the list it must first traverse the list, so unless an
ordered list is necessary, [Cons](#cons) should always be used to add
an item to a list.

```
   foreach i in lSortInfo
   {
      if StringEqual(String,First(i))
      {
         if plClassOrderPreferences <> $
            AND FindListElem(plClassOrderPreferences,Nth(i,2)) <> 0
         {
            Send(self,@MsgSendUser,#message_rsc=inventory_already_in_list);

            return;
         }

         plClassOrderPreferences = AppendListElem(Nth(i,2),plClassOrderPreferences);

         Send(self,@MsgSendUser,#message_rsc=inventory_class_added,
               #parm1=First(i));

         return;
      }
   }
```

#### InsertListElem
`InsertListElem(list, n, expr)`

Takes a list, a position and an expression, creates a list node out of the
expression and adds it at the nth position of the given list. Returns the
modified list's first list node (which may be new, if the element was inserted
in the first position).

```
   plHonor = InsertListElem(plHonor,iPosition,string);
```

#### GetListElemByClass
`GetListElemByClass(list, class)`

Takes a list and a class, returns the first instance in the list that matches
the class. Used to replace list iterations where a single element was the
target, and each element was being compared via IsClass. Returns $ if no
element of the class was found.
```
   GetReagentBag()
   {
      return GetListElemByClass(plPassive,&ReagentBag);
   }
```

#### GetListNode
`GetListNode(list, n, object)`

Used exclusively for iterating through a list that contains lists. Takes a
list, an integer and an object, checks the nth position of each sub-list for
the object and if found, returns the sub-list itself. Returns $ if no sub-list
in the list contains the object at the nth position.
```
   GetEnchantmentState(what = $)
   "If enchanted by <what>, return enchantment state."
   {
      local lEnchantment;

      lEnchantment = GetListNode(plEnchantments, 2, what);
      if lEnchantment <> $
      {
         return Nth(lEnchantment,3);
      }

      return $;
   }
```

#### SwapListElem
`SwapListElem(list, n1, n2)`

Takes a list and two integers, and swaps the data in the two list nodes.
Returns NIL (as the original list is modified). Not used for anything yet.

#### IsListMatch
`IsListMatch(list1, list2)`

Returns TRUE if the two lists have identical contents, FALSE otherwise.
If the lists contain other lists, the contents are also compared.

```
   lIP = Send(i,@GetIP);

   foreach j in lUserIPList
   {
      if IsListMatch(lIP,j)
      {
         bFound = TRUE;
      }
   }
```

#### ListCopy
`ListCopy(list)`

Creates a copy of the list and returns it. If necessary it will recurse on
any lists held by the original list and copy those also. NOTE: does not create
copies of objects or tables.
```
   plPlayerRestrict2 = ListCopy(Send(oQE,@GetQuestPlayerRestrictions2,
                                    #index=piQuestTemplateIndex));
```

#### SetFirst
`SetFirst(list)`

Mutate the first element of the first node of list in place, that is, without
creating any new list nodes.

```
   if (percent <> $)
   {
      SetFirst(lSpeechBranch,percent);
   }
```

#### SetNth
`SetNth(list, n , expr)`

Mutate the first element of the n th node of list in place, that is, without
creating any new list nodes.

```
   ResetParliament()
   {
      local lSenator, oParliament;

      if plSenate <> $
      {
         for lSenator in plSenate
         {
            SetNth(lSenator,3,Send(self,@GetInitialState,#councilor_num=First(lSenator)));
            SetNth(lSenator,5,Send(self,@GetInitialTimeval,#token_num=First(lSenator)));
            SetNth(lSenator,6,0);
         }
      }

      oParliament = Send(SYS,@GetParliament);

      if oParliament <> $
      {
         Send(oParliament,@RedoPower);
      }

      return;
   }
```

#### DelListElem
`DelListElem(list, item)`

Returns the list with the first occurrence of the specified value removed
from the list.

```
   foreach i in plTrail
   {
      if Send(i,@GetOwner) = $
      {
         plTrail = DelListElem(plTrail,i);

         continue;
      }
      if (Send(i,@GetRow) = new_row)
         AND (Send(i,@GetCol) = new_col)
      {
         Send(i,@Delete);
         plTrail = DelListElem(plTrail,i);

         continue;
      }
```

#### FindListElem
`FindListElem(list, expr)`

```
   RestrictToResourceList(res = $, res_list = $)
   "If res is in res_list, return it.  Otherwise return the first element "
   "of res_list."
   {
      if FindListElem(res_list, res)
      {
         return res;
      }

      return Nth(res_list, 1);
   }
```

#### IsList
`IsList(object)`

Return true if the object is a list node.

```
   AddToList(thePerson = $)
   "Adds the people to the list.  thePerson can be an object or list of objects."
   {
      local aPerson;

      if thePerson <> $
      {
         if NOT IsList(thePerson)
         {
            thePerson = [thePerson];
         }
         foreach aPerson in thePerson
         {
            plPeople = Cons(aPerson,plPeople);
         }
      }
      return;
   }
```

#### GetAllListNodesByClass
`GetAllListNodesByClass(list, list_position, class)`

Copies all objects in a list matching the given class into a new list. If the
list_position given is 0, copies directly from the list. If list_position is 1,
copies from the first element of each sublist contained in the original list.
Has the potential to create a lot of extra list nodes, and is not currently
used.

## Communications
#### AddPacket
`AddPacket(expr1, expr2, ...)`

Add to the interpreter's communication queue. The first argument gives the
length of the second argument in bytes, the third argument gives the length
of the fourth argument, etc. The communication queue contains everything
passed in via calls to AddPacket since the last call to SendPacket.

```
   ToCliSpellSchools()
   {
      local num_schools;

      AddPacket(1,BP_USERCOMMAND, 1,UC_SPELL_SCHOOLS);
      num_schools = 6;
      if viDM
      {
         num_schools = num_schools + 1;
      }

      AddPacket(1,num_schools);
      AddPacket(4,user_school_shallile);
      AddPacket(4,user_school_qor);
      AddPacket(4,user_school_kraanan);
      AddPacket(4,user_school_faren);
      AddPacket(4,user_school_riija);
      AddPacket(4,user_school_jala); 

      if viDM
      {
         AddPacket(4,user_school_dm);
      }

      SendPacket(poSession);

      return;
   }
```

#### SendPacket
`SendPacket(session)`

Send the contents of the communication queue to the client identified by
session, and then clear the queue. The interpreter passes session numbers to
Blakod when users log on.

```
   ToCliInventory()
   {
      local i,each_obj;

      AddPacket(1,BP_INVENTORY, 2,Length(plActive)+Length(plPassive));

      foreach i in plActive
      {
         Send(self,@ToCliObject,#what=i,#show_type=SHOW_INVENTORY);
      }

      i = Length(plPassive);
      while i > 0
      {
         each_obj = Nth(plPassive,i);
         Send(self,@ToCliObject,#what=each_obj,#show_type=SHOW_INVENTORY);
         i = i - 1;
      }

      SendPacket(poSession);

      return;
   }
```

#### SendCopyPacket
`SendCopyPacket(session)`

Send the contents of the communication queue to the client identified by
session, without clearing the queue.

```
   if IsClass(each_obj,&User)
   {
      // we don't send turns to non-users
      if not packet_built
      {
         Send(each_obj,@BuildPacketSomethingTurned,#what=what,
               #new_angle=new_angle,#cause=cause);
         packet_built = TRUE;
      }

      if each_obj <> what or cause <> CAUSE_USER_INPUT
      {
         // Seventh element will be a user's session
         SendCopyPacket(Nth(i,7));
      }
   }
```

#### ClearPacket
`ClearPacket( )`

Manually clears the communications queue.

```
   if each_obj = what
   {
      // People need to know they moved, they store the coords
      // However, sending this message could do addpackets... so we
      //  need to reset
      ClearPacket();
      packet_built = FALSE;
      Send(each_obj,@SomethingMoved,#what=what,
            #new_row=new_row,#new_col=new_col,
            #fine_row=fine_row,#fine_col=fine_col,
            #cause=cause,#speed=speed);
   }
```

## Class Operations
#### Create
`Create(&Class, #param1 = val1, #param2 = val2, ...)`

Create and return a new object of the given class. The parameters are passed
to the class's constructor.

```
   CreateDeadBody(killer=$)
   {
      local oBody;

      oBody = $;

      if vrDead_icon <> $
      {
         oBody = Create(&DeadBody,#icon=vrDead_icon,#name=vrDead_name,
                     #mob=TRUE,#playername=Send(killer,@GetTrueName),
                     #monstername=vrName,#drawfx=viDead_drawfx);
      }

      return oBody;
   }
```

#### IsClass
`IsClass(object, &class)`

Return true if the object is a subclass of the given class.

```
   // Jig prevents combat between players and monsters.
   if IsClass(what,&Player)
      AND Send(what,@IsAffectedByRadiusEnchantment,#byClass=&Jig)
   {
      return FALSE;
   }
```

#### GetClass
`GetClass(object)`

Return the class of the given object.

```
   oRoom = Send(self,@FindRoomByNum,#num=iRoom);
   if oRoom = $
   {
      Debug("Chest",oChest,"in non-existent room num",iRoom);
   }
   else
   {
      for j in Send(oRoom,@GetHolderActive)
      {
         if GetClass(First(j)) = GetClass(oChest)
            AND iRow = Send(First(j),@GetRow)
            AND iCol = Send(First(j),@GetCol)
            AND iFineRow = Send(First(j),@GetFineRow)
            AND iFineCol = Send(First(j),@GetFineCol)
         {
            Send(First(j),@Delete);
            Send(oRoom,@NewHold,#what=oChest,#new_row=iRow,#new_col=iCol,
                  #fine_row=iFineRow,#fine_col=iFineCol);
         }
      }
```

## Strings
#### StringEqual
`StringEqual(string1, string2)`

Return true if the two strings contain the same ASCII string; the comparison
is case insensitive.

```
   SomeoneSaid(what=$,string=$)
   {
      local oPlayer, oGuild;

      if StringEqual(string,GuildCreator_rent_command)
         OR StringEqual(string,"rent")
      {
         Send(self,@ReportRent,#who=what);
      }
```

#### StringContain
`StringContain(string1, string2)`

Return true if the string1 contains the ASCII string in string2; the
comparison is case insensitive.

```
   if StringContain(String,"autocombine on")
      AND NOT Send(self,@IsAutoCombining)
   {
      Send(self,@SetAutoCombine,#value=TRUE);
      Send(self,@MsgSendUser,#message_rsc=autocombine_on_msg);

      return;
   }
```

#### StringSubstitute

```
   CleanseString(string = $)
   {
      local rBadWord, bContinue;

      for rBadWord in plNaughtyWords
      {
         bContinue = TRUE;
         while bContinue
         {
            bContinue = StringSubstitute(string,rBadWord,system_swear_symbols);

            if bContinue = $
            {
               // Something bad happened, just bail.
               return;
            }
         }

      }

      return string;
   }
```

#### StringLength

```
   if StringLength(Nth(client_msg, 2)) > MAX_GUILD_NAME_LEN
   {
      Debug(self, "had guild name too long");

      return FALSE;
   }
```

#### StringConsistsOf

```
   if NOT StringConsistsOf(sName, 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_ '!@$^&*()+=:[]{};/?|<>")
   {
      Debug("Got char name with illegal characters for user ", oUser);
      bLegal = FALSE;
   }
```

#### ParseString
`ParseString(string, separators, message)`

Parse the given string into a set of substrings, separated by characters
in the string separators.

```
   UserLookupNames(amount = $,string = $)
   "Sent by client to resolve a string of mail destination into object IDs."
   {
      AddPacket(1,BP_LOOKUP_NAMES);
      AddPacket(2,amount);

      ParseString(string,",",@UserLookupEachName);
      SendPacket(poSession);

      return;
   }
```

#### SetString
`SetString(var, text)`

Set var to the given text. If passed a kod string, text will be placed in it.
If passed `$` for var, SetString will create a new kod string to place the text
into. This negates the need for a prior [CreateString](#createstring) call.
SetString can accept a message for the second parameter in place of a string,
in which case it will set the string to the name of that message.
```
   if (string = GetTempString())
   {
      psHonor = SetString($, string);
   }
```

or
```
   sString = SetString($,"string text");
```

or
```
   sString = SetString($,@StartQormas);
```

#### AppendTempString
`AppendTempString(data)`

Appends data to a temp string used by Blakserv, which can be used in the future
by calling GetTempString(). NOTE that this is being deprecated in favor of
using resources, and unless absolutely necessary no new AppendTempString calls
will be added.

```
   IntToString(num=0)
   {
      local retVal;

      ClearTempString();
      AppendTempString(num);
      retVal = CreateString();
      SetString(retVal,GetTempString());

      return retVal;
   }
```

#### ClearTempString
`ClearTempString()`

Clears all data stored in the kod temp string.
```
   GetNecromancerRoster()
   {
      ClearTempString();
      Send(&NecromancerAmulet,@NecromancerRosterRequest,#olich=self);
      if StringContain(GetTempString(),lich_comma)
      {
         AppendTempString(lich_and);
      }
      AppendTempString(psTempRosterString);
      psTempRosterString = $;

      return SetString($,GetTempString());;
   }
```

#### GetTempString
`GetTempString()`

Get the ID of the kod temp string so it can be used in other C calls, e.g.
for creating a kod string from the contents of the temp string.
```
   GetNecromancerRoster()
   {
      ClearTempString();
      Send(&NecromancerAmulet,@NecromancerRosterRequest,#olich=self);
      if StringContain(GetTempString(),lich_comma)
      {
         AppendTempString(lich_and);
      }
      AppendTempString(psTempRosterString);
      psTempRosterString = $;

      return SetString($,GetTempString());;
   }
```

#### CreateString
`CreateString()`

Create a new, empty string. Note: this is somewhat deprecated as
[SetString](#setstring) can create its own empty string to add to.

```
      local sString, sString2, oBook;

      ClearTempString();
      AppendTempString(barloque_clerk_indulgence_1);
      AppendTempString(Send(who,@GetTrueName));
      AppendTempString(barloque_clerk_indulgence_2);
      sString = CreateString();
      SetString(sString,GetTempString());
```

#### SetResource
`SetResource(resource, string)`

Changes the string value of a dynamic resource. Used for changing player names.
```
   MakeQ()
   {
      local i, lTemplate;

      SetResource(vrName,dm_q);
      prToupee = player_toupee_q_rsc;
```

#### IsString
`IsString(string)`

Returns TRUE if string is a valid string with data tag TAG_STRING, FALSE
otherwise.
```
   if NOT IsString(message)
   {
      message = SetString($,message);
   }
```

## Timers
#### CreateTimer
`CreateTimer(object, message, time)`

Create and return a new timer. The timer will go off in time milliseconds,
when the given message will be sent to the given object.

```
   local oTimer,lNew_enchantment;

   if time = $ OR time = SPELL_INDEFINITE_DURATION
   {
      oTimer = $;
   }
   else
   {
      oTimer = CreateTimer(self,@EnchantmentTimer,time);
   }
```

#### DeleteTimer
`DeleteTimer(timer)`

Delete the given timer. Should be preceded by a null ($) check, as deleting a
non-existent timer will log an error (e.g.  [guild.bof (2705)] DeleteTimer
can't find timer 190077).

```
   ClearBasicTimers()
   {
      if ptRandom <> $
      {
         DeleteTimer(ptRandom);
         ptRandom = $;
      }

      if ptSpasm <> $
      {
         DeleteTimer(ptSpasm);
         ptSpasm = $;
      }

      if ptCancelOffer <> $
      {
         DeleteTimer(ptCancelOffer);
         ptCancelOffer = $;
      }

      poCustomer = $;

      return;
   }
```

#### GetTimeRemaining
`GetTimeRemaining(timer)`

Return the number of milliseconds remaining on the given timer

```
   // Check the time currently left on the dipose timer.  Reset the timer
   //  to a minute if we have less than a minute left.  This should cut
   //  down on cases of someone dropping an object and it being cleaned up
   //  immediately afterwards.
   iTimeLeft = GetTimeRemaining(ptDispose);
   if iTimeLeft < 60000
   {
      DeleteTimer(ptDispose);
      ptDispose = CreateTimer(self,@DisposeTimer,60000);
   }
```

#### IsTimer
`IsTimer(timer)`

Returns TRUE if timer is a valid timer with data tag TAG_TIMER, FALSE otherwise.

```
   if IsClass(Nth(i,2),&Boon) = FALSE
   {

      // Convert timer to integer if we have a timer,
      // otherwise log for debugging.
      if IsTimer(First(i))
      {
         iTime = GetTimeRemaining(First(i));
         DeleteTimer(First(i));

         // If it's a negative enchantment, add a bit to the time in
         // order to prevent people from just "flashing" on and off
         // to wait out an enchantment and be relatively unhittable.
         if Send(Nth(i,2),@IsHarmful)
         {
            // Time is measured in milliseconds, 3000 = 3 seconds.
            iTime = iTime + 3000;
         }
      }
```

## Room Operations
#### RoomData

```
   LoadRoomData()
   {
      local lRoom_data;

      if prRoom = $
      {
         Debug("Problems with",vrName,piRoom_num);
      }

      prmRoom = LoadRoom(prRoom);
      lRoom_data = RoomData(prmRoom);
      piRows = First(lRoom_data);
      piCols = Nth(lRoom_data,2);
      piSecurity = Nth(lRoom_data,3);

      return;
   }
```

#### LoadRoom
`LoadRoom(expr)`

Return a reference to the room description given by the resource number expr.
This function is used to load the room's description file when the room is
created. The data that BlakServ loads about a room is nearly permanently
stored in memory. It is only unloaded upon a reload sys command, which unloads
the data about every loaded room or if FreeRoom is called with the roomdata.
The Blakod for every game must reload rooms after every system reload, and also
erase all references to the room data that no longer exists.

```
   LoadRoomData()
   {
      local lRoom_data;

      if prRoom = $
      {
         Debug("Problems with",vrName,piRoom_num);
      }

      prmRoom = LoadRoom(prRoom);
      lRoom_data = RoomData(prmRoom);
      piRows = First(lRoom_data);
      piCols = Nth(lRoom_data,2);
      piSecurity = Nth(lRoom_data,3);

      return;
   }
```

#### FreeRoom
`FreeRoom(roomdata)`

Frees the memory associated with a room.

```
   FreeRoom(prmRoom);
```

#### LineOfSightBSP
`LineOfSightBSP(roomdata, startrow, startcol, startfinerow, startfinecol, endrow, endcol, endfinerow, endfinecol)`

Checks if a source location has line of sight with a destination, uses the BSP tree.

```
   LineOfSight(obj1 = $, obj2 = $)
   "Returns TRUE if there is a line of sight between obj1 and obj2"
   {
      local start_row, start_col, start_finerow, start_finecol,
            end_row, end_col, end_finerow, end_finecol, los;

      % get coordinates of source (obj1) and target (obj2)
      start_row = Send(obj1, @GetRow);
      start_col = Send(obj1, @GetCol);
      start_finerow = Send(obj1, @GetFineRow);
      start_finecol = Send(obj1, @GetFineCol);
      end_row = Send(obj2, @GetRow);
      end_col = Send(obj2, @GetCol);
      end_finerow = Send(obj2, @GetFineRow);
      end_finecol = Send(obj2, @GetFineCol);

      % call BSP LoS in C code  
      return LineOfSightBSP(prmRoom,
         start_row, start_col, start_finerow, start_finecol,
         end_row, end_col, end_finerow, end_finecol);
   }
```

#### LineOfSightView
`LineOfSightView(kod_angle, row, col, finerow, finecol, end_row, end_col, end_finerow, end_finecol)`

Checks if the object at the first set of coordinates facing the given kod angle
(4096 degrees) can see the object at the second set of coordinates. Returns
TRUE or FALSE.
```
LineOfSightView(Send(self,@GetAngle), piRow, piCol, piFine_row,
   piFine_col, Send(oTarget,@GetRow), Send(oTarget,@GetCol),
   Send(oTarget,@GetFineRow), Send(oTarget,@GetFineCol))
```

#### CanMoveInRoomBSP
`CanMoveInRoomBSP(roomdata, startrow, startcol, startfinerow, startfinecol, endrow, endcol, endfinerow, endfinecol)`

Checks if you can walk along a straight line from the source to the destination.

```
   CanMoveInRoom(obj1 = $, obj2 = $)
   "Returns TRUE if obj1 can walk along a straight line to obj2"
   {
      local start_row, start_col, start_finerow, start_finecol,
            end_row, end_col, end_finerow, end_finecol, los;

      % get coordinates of source (obj1) and target (obj2)
      start_row = Send(obj1, @GetRow);
      start_col = Send(obj1, @GetCol);
      start_finerow = Send(obj1, @GetFineRow);
      start_finecol = Send(obj1, @GetFineCol);
      end_row = Send(obj2, @GetRow);
      end_col = Send(obj2, @GetCol);
      end_finerow = Send(obj2, @GetFineRow);
      end_finecol = Send(obj2, @GetFineCol);

      % call BSP CanMoveInRoom in C code  
      return CanMoveInRoomBSP(prmRoom,
         start_row, start_col, start_finerow, start_finecol,
         end_row, end_col, end_finerow, end_finecol);
   }
```

#### MoveSectorBSP
`MoveSectorBSP(roomdata, sector, animation, height, speed)`

Adjusts the height, speed and animation type of a sector in the room's BSP tree.

```
   SetSector(sector=$,animation=$,height=$,speed=$)
   "Call this to change the height of a sector (affects clients only, not "
   "move grid)."
   {
      local i,each_obj;

      % Update BSP room data
      MoveSectorBSP(prmRoom, sector, animation, height, speed);
```

#### ChangeTextureBSP
`ChangeTextureBSP(roomdata, room_wall_id, texture_id, flags)`

Adjusts the texture id (bitmap) and flags (above, below, normal wall) for a
texture in the server's BSP tree. Necessary so that texture removals can
affect line of sight.

```
   ChangeTexture(id = $,new_texture = $,flags = 0)
   {
      local i,each_obj;

      % update BSP room data
      ChangeTextureBSP(prmRoom, id, new_texture, flags);
```

#### GetLocationInfoBSP
`GetLocationInfoBSP(room_data, query_flags, row, col, finerow, finecol, *return_flags, *floor_height, *floor_depth_height, *ceiling_height, *sector_id)`

One of the most complicated C calls - used for getting information about the
given location (row, col, finerow, finecol) from the data stored for the room.
The query_flags parameter specifies which data is being requested, and the last
five parameters are local variables (passed by ID) in which to place the query
results. Returns TRUE if the query was successful, FALSE if it was not.
```
   GetHeightFloorAtObjectBSP()
   {
      local iQflags, iRflags, iHeightF, iHeightFWD, iHeightC, iServerID;

      // set query-flags, here we're only interested in the sectorinfo
      iQflags = LIQ_GET_SECTORINFO;

      // query data from bsp in c-function
      if NOT GetLocationInfoBSP(
                  Send(poOwner,@GetRoomData), iQflags,
                  Send(self,@GetRow), Send(self,@GetCol),
                  Send(self,@GetFineRow), Send(self,@GetFineCol),
                  *iRflags, *iHeightF, *iHeightFWD, *iHeightC, *iServerID)
      {
         Debug("Failed to get location info in ", Send(poOwner,@GetRoomData));
         return 0;
      }

      % we're only using the floor-height with depth modifier here
      return iHeightFWD;
   }
```

#### BlockerAddBSP
`BlockerAddBSP(room_data, object, row, col, finerow, finecol)`

Adds a 'blocker' to the room's list of objects which block movement for
monsters or players. The room must keep track of all blockable objects to
correctly disallow movement for other objects.

```
   if NOT BlockerAddBSP(prmRoom, what, Nth(new_pos,2),Nth(new_pos,3),
               Nth(new_pos,4),Nth(new_pos,5))
   {
      Debug("Failed to add BSP blocker ", what, " to ", prmRoom);
   }
```

#### BlockerMoveBSP
`BlockerAddBSP(room_data, object, row, col, finerow, finecol)`

Called whenever an object that can block movement moves to a new location, to
update the roomdata's position stored for that object.
```
   if (Send(what,@GetMoveOnType) = MOVEON_NO)
   {
      if NOT BlockerMoveBSP(prmRoom, what, new_row, new_col, fine_row, fine_col)
      {
         % debug("Failed to move blocker ", what);
      }
   }
```
#### BlockerRemoveBSP
`BlockerRemoveBSP(room_data, object)`

Called whenever an object that blocks movement leaves a room.
```
   LeaveHold(what = $)
   {
      local i, each_obj;

      // Make sure to unregister it as a blocker.
      BlockerRemoveBSP(prmRoom, what);
```

#### BlockerClearBSP
`BlockerClearBSP(room_data)`

Removes all blockers saved in the roomdata. Not currently in use.

#### GetRandomPointBSP
`GetRandomPointBSP(room_data, max_tries, *row, *col, *finerow, *finecol)`

Used for finding a free, valid space to place a monster or any other object.
Max_tries acts as a safety to avoid running an infinite loop, and if the call
is successful the coordinates to use are placed into the local vars row, col,
finerow and finecol.
```
   // query random point from C (into local vars)
   // this point is guaranteed to have the following properties:
   // (a) inside ThingsBox  (b) inside a sector with a texture set
   // (c) not blocked by another object
   if NOT GetRandomPointBSP(prmRoom, 15, *iRow, *iCol, *iFineRow, *iFineCol)
   {
      Debug("failed to place monster ",oMonster,Send(oMonster,@GetName),
            " in room ",self,Send(self,@GetName));
      Send(oMonster,@Delete);

      return FALSE;
   }
```

#### GetStepTowardsBSP
`GetStepTowardsBSP(room_data, row, col, finerow, finecol, *end_row, *end_col, *end_finerow, *end_finecol, flags, object)`

Used for calculating the next step an object can take towards the destination
given by the first set of coordinates. If unsuccessful, this call will return
`$`, otherwise it will return flags for movement (which way the object is
moving). If successful, it will also place the valid destination coordinates
into the second set of coordinate variables.
```
   % query where to step next
   % note: these need some persistent info across
   % calls which are stored in piState (flags from old KOD code)
   iState = GetStepTowardsBSP(
               Send(poOwner, @GetRoomData),
               piRow, piCol, piFine_row, piFine_col,
               *iRow, *iCol, *iFineRow, *iFineCol,
               piState | iMoveFlags, self);
```

## Hash Tables
#### AddTableEntry
`AddTableEntry(table, key, value)`

Add the (key, value) pair to the given hash table. If the number of elements
in the table exceeds 50% the size of the table, the table is resized at twice
the current size.

```
   CreateUserTable()
   {
      local i;

      phUsers = CreateTable();

      foreach i in plUsers
      {
         AddTableEntry(phUsers,Send(i,@GetUserName),i);
      }

      return;
   }
```

#### CreateTable
`CreateTable(size)`

Return a new, empty hash table. Size is an optional positive integer parameter
between 23 and 19463. Values outside this range are set to the default, 73.
The tables use open hashing, a very common hashing system which is quite
simple. Each entry in a hash table is actually a linked list of elements that
have the same hash value. In this way, the hash table is never full, because
it is not of a fixed size, just a fixed number of hash values. Tables will
resize themselves if the number of elements exceeds half the table size. If
this happens (while adding a table entry) all entries will be readded after
the resize.

```
   CreateRoomTable()
   {
      local i;

      phRooms = CreateTable();

      foreach i in plRooms
      {
         AddTableEntry(phRooms,Send(i,@GetRoomNum),i);
      }

      return;
   }
```

#### GetTableEntry
`GetTableEntry(table, key)`

Return the value matching the given key in the hash table, or nil if the key
is not present in the table.

```
   CheckUserTable()
   {
      local i, r;

      r = system_success_rsc;
      foreach i in plUsers
      {
         if GetTableEntry(phUsers,Send(i,@GetUserName)) <> i
         {
            Debug("CheckUserTable: hash error on",i,Send(i,@GetUserName));
            r = system_failure_rsc;
         }
      }

      return r;
   }
```

#### DeleteTableEntry
`DeleteTableEntry(table, key)`

Remove the entry in the hash table with the given key, if any.

```
   SystemUserDelete(what = $)
   "Removes a user from the system-wide list of all users."
   {
      local i;

      DeleteTableEntry(phUsers,Send(what,@GetTrueName));

      foreach i in plUsers
      {
         if what = i
         {
            plUsers = DelListElem(plUsers,i);

            return TRUE;
         }
      }

      Debug("Tried to delete object that is not in plUsers.");

      return FALSE;
   }
```

#### DeleteTable
`DeleteTable(table)`

Free all memory associated with the given hash table, and delete the table.
This C call is deprecated, and will not delete anything as table memory
deallocation is handled during GC.

```
No examples in code.
```

#### IsTable
`IsTable(table)`

Returns TRUE if the parameter is a valid table (tag and ID checked), FALSE
otherwise.

```
   if phUsers = $ OR NOT IsTable(phUsers)
   {
      Send(self,@CreateUserTable);
   }
```

## Message Passing
#### Send
The most important call is Send, which calls an object's message handler.
The syntax is

`Send(object, @Message, #param1 = val1, #param2 = val2, ...)`

Object gives the object whose message handler is to be called; message is the
name of the message handler to call. The parameters are matched by name, so
they can appear in any order. Execution immediately passes to the object's
message handler, and returns to the caller when a return statement is reached
in the handler. All parameters do not need to be supplied. Any parameters not
passed to a message handler are initialized to the value specified in the
message handler definition before the message handler code gets control.

A message name can also be passed via kod string (TAG_STRING).
```
   SetForSale()
   {
      local i, oJunk1, oJunk2, oJunk3, lForSale;

      oJunk1 = Create(&Junk);
      oJunk2 = Create(&Junk);
      oJunk3 = Create(&Junk);

      plFor_sale = [ [oJunk1,oJunk2,oJunk3 ],$,$ ];

      Send(self,@NewHold,#what=oJunk1);
      Send(self,@NewHold,#what=oJunk2);
      Send(self,@NewHold,#what=oJunk3);

      return;
   }
```

#### Post
`Post(object, @Message, #param1 = val1, #param2 = val2, ...)`

Post has the same syntax as Send, but the call is not made until after the
current message (and all of the callers, back to the original client message
forwarded by the server) is complete. This is useful when something should
be done after the current call, such as responding to speech said by a user.

Can also pass a message name via kod string (TAG_STRING).

```
   % Quickly, lets check to see if it's a signet ring.  If so, give a
   %  special message.
   if IsClass(obj,&SignetRing)
   {
      Post(poOwner,@SomeoneSaid,#what=self,
            #string=LS_Signet_wrong,#type=SAY_RESOURCE,
            #parm1=Send(Send(obj,@GetRingOwner),@GetDef),
            #parm2=Send(Send(obj,@GetRingOwner),@GetName));

      return;
   }
```

#### SendList
`SendList(list, list_position, @Message, #param1 = val1, ...)`

Calls @Message on every object in the list. If list_position is 0, the objects
are in the list itself. If list_position is 1, the objects are in the first
position in each sub-list of the parent list etc. Returns FALSE if any called
objects returns FALSE from the message, TRUE otherwise.
```
   UserLogonHook()
   {
      local i, oObj, iBonus, iDefaultBonus, iTimespan;

      SendList(plPassive, 0, @UserLogon);
      SendList(plActive, 0, @UserLogon);
```

#### SendListByClass
`SendListByClass(list, list_position, &Class, @Message, #param1 = val1, ...)`

Calls @Message on every object in the list matching the given class. Works the
same as SendList.
```
   SomethingWaveRoom(what = 0,wave_rsc = $)
   "Don't send <what> if not from any particular object"
   {
      SendListByClass(plActive,1,&User,@WaveSendUser,
            #wave_rsc=wave_rsc,#source_obj=what);

      return;
   }
```

#### SendListBreak
`SendListBreak(list, list_position, @Message, #param1 = val1, ...)`

Calls @Message on every object in the list. If list_position is 0, the objects
are in the list itself. If list_position is 1, the objects are in the first
position in each sub-list of the parent list etc. If any object in the list
returns FALSE, the remaining objects will not be called and SendListBreak
itself will return FALSE. Returns TRUE otherwise.
```
   if NOT SendListBreak(plActive,1,@ReqSomethingAttack,#what=what,
               #victim=victim,#use_weapon=use_weapon)
   {
      return FALSE;
   }
```

#### SendListByClassBreak
`SendListByClassBreak(list, list_position, &Class, @Message, #param1 = val1, ...)`

Calls @Message on every object in the list matching the given class. Works the
same as SendListBreak.
```
No examples in code.
```

## Logging Functions
#### GodLog
`GodLog(expr1, expr2, ...)`

Print given values on the server's terminal.  Also saves to server log god.txt.
```
   if bAdminPort
   {
      % Log the teleport.
      GodLog("Admin ",Send(self,@GetTrueName)," teleported to ",
            Send(oRoom,@GetName));
   }
```

#### Debug
`Debug(expr1, expr2, ...)`

Print given values on the server's terminal.  Also saves to server logfile:
debug.txt.
```
   if poTarget = $ OR NOT IsClass(poTarget,&Battler)
   {
      Debug("Unreachable. Null poTarget","monster",self);
      poTarget = $;
      Send(self,@EnterStateWait); 

      return;
   }
```

#### RecordStat

Allows data to be saved to a MySQL database. Takes a STAT_TYPE parameter, and
the data to be added.
```
   GarbageCollecting()
   {
      Send(self,@CalculateTotalMoney);
      RecordStat(STAT_TOTALMONEY,piPlayerMoneyTotal);
      RecordStat(STAT_MONEYCREATED,piMoneyCreated);
      piMoneyCreated = 0;

      return;
   }
```

## Timing Functions
#### GetTime
`GetTime( )`

Return the current wall clock time in seconds.
```
   RecalcLightAndWeather()
   {
      local i, iTime, iDay, iMinutes;

      % This assumption is no longer valid, but it's how we figure things out.
      % Remember that GetTime() returns the UDT time in seconds since
      %  January 1, 1996.  Assume we're 5 hours behind that.

      iTime = GetTime() - 5 * HOUR;

      iDay = (iTime / (24 * HOUR)) MOD 365;

      % Our day is 2 hours long now, so get minutes into the four hours
      iMinutes = (iTime MOD (2 * HOUR)) / 60;

      % That number ranges from 0 to 119, so div by 5 is our game hour
      piHour = iMinutes / 5;

      Send(self,@NewGameHour);

      return;
   }
```

#### GetTickCount

Like GetTime(), but in milliseconds and with a precision of +- 1ms.
Now has both Windows and Linux implementations. Windows uses
QueryPerformanceCounter, Linux uses gettimeofday.
```
   UserLogonHook()
   {
      local i;
     
      foreach i in Send(SYS,@GetBackgroundObjects)
      {
         Send(i,@AddBackgroundObject,#who=self);
      }

      Send(self,@LoadMailNews);

      % Count right now as the last time we moved.
      piLastMoveUpdateTime = GetTickCount();

      propagate;
   }
```

#### GetInactiveTime
`GetInactiveTime(session)`

Return the number of milliseconds since a message was received on the
given session.

```
   GetIdleTime()
   {
      return GetInactiveTime(poSession);
   }
```

#### GetTimeZoneOffset
`GetTimeZoneOffset( )`

Returns the time zone offset from UTC for the location of the server.
Subtracting the return value of GetTimeZoneOffset from UTC gives local time.

```
   Constructor()
   {
      ptTick = CreateTimer(self,@TimeTick,60000);
      piTimeZoneOffset = GetTimeZoneOffset();

      return;
   }
```

## Miscellaneous
#### Random
`Random(expr1, expr2)`

Return a random number uniformly distributed over the given closed interval
of integers.

```
   SetRandomHomeroom()
   "This sets one of the many hometowns to be the initial hometown."
   {
      local rand;

      rand = Random(1,10);

      if rand < 4
      {
         piHomeroom = RID_MAR_INN;
      }
      else if rand > 7
      {
         piHomeroom = RID_JAS_INN;
      }
      else
      {
         piHomeroom = RID_COR_INN;
      }

      return;
   }
```

#### Abs
`Abs(expr)`

Return the absolute value of the given expression.

```
   KarmaSame(who=$)
   "Return TRUE if the mob and the who have karmas of the same polarity."
   {
      local iPlayer_karma;

      % This means the monster has no Karma.
      if viKarma = $
      {
         return TRUE;
      }

      iPlayer_karma = Send(who,@GetKarma);
     
      % Second clause is to give newbies a break
      if (viKarma * Send(who,@GetKarma)) < 0
         AND Abs(iPlayer_karma) >= 10
      {
         return FALSE;
      }

      return TRUE;
   }
```

#### Sqrt
`Sqrt(expr)`

Returns the square root of the given expression.

```
   if (oTarget = $)
   {
      iAdd = (((Send(mob,@GetDifficulty)*piMax_distance)/10)
               * piDistance_factor);
      iAdd = iAdd - (piDistance_factor
         * Bound(Sqrt(Send(mob,@SquaredDistanceTo,
                 #what=what)),0,piMax_distance));
      iAdd = Bound(iAdd,0,$);
   }
```

#### Bound
`Bound(expr1, expr2, expr3)`

Bound expr1 to the interval [expr2, expr3], and return the result.

```
   local iDuration;

   if Duration <> $
   {
      iDuration = Random(Duration-20,Duration+20);
      iDuration = iDuration * 1000;
      iDuration = Bound(iDuration,30000,200000);
      ptExpire = CreateTimer(self,@Expire,iDuration);
   }
```

#### IsObject

```
   if what <> $
      AND NOT IsObject(what)
   {
      % Log a debug message for this
      Debug("User ",self," tried to look at non-object ",what);

      return;
   }
```

#### GetSessionIP
`GetSessionIP(session)`

Returns the IP of a session.

```
   GetIP()
   "Returns the user's ip in a list which contains the 4 octets"
   "use this message for comparisons and the string message sparingly."
   {
      return GetSessionIP(poSession);
   }
```

#### SaveGame
`SaveGame()`

Triggers Blakserv to save the game without Garbage Collection, and returns a
kod string (TAG_STRING) with the string value of the save game time.

```
   if (IKnowThisIsDangerous = 820965)
   {
      % Save the game.
      psSaveGame = SaveGame();
   }
```

#### LoadGame
`LoadGame(string)`

Triggers Blakserv to load a savegame using the save game time stored in the
string parameter. LoadGame will post the load message to the Blakserv
interface thread, which will be executed when the message queue is empty.

```
   % If we don't have a save game, don't try to reload.
   if psSaveGame = $
   {
      Debug("System has no save game, not ending chaos night!");

      return system_failure_rsc;
   }
   % LoadGame will schedule a game reload in the blakserv main thread,
   % which will kick all users offline and reload the saved game. This
   % is performed in the main thread so that blakod interpreting isn't
   % affected.
   LoadGame(psSaveGame);
   psSaveGame = $;
```

#### RecycleUser

```
   if pbRecycleUsersEnabled
   {
      oUser = RecycleUser(oUser);
   }
```

#### SetClassVar
`SetClassVar(class, classvar, data)`

Allows changing a classvar for a given class from within KOD. Limited use, as
changes are temporary until the system is reloaded. Requires the string name
of the classvar via debug string.

```
   ReduceAllItemWeightAndBulk(iNumber=1)
   "Reduces (or increases) the weight and bulk of all items "
   "to the number given. Default 1. Item has to be in the "
   "item templates list. Does not affect shillings."
   {
      local i;

      % Check for frenzy active first, to avoid running
      % this accidentally.
      if poChaosNight = $
      {
         return;
      }

      foreach i in plItemTemplates
      {
         if NOT IsClass(i,&Money)
         {
            SetClassVar(i,"viBulk",iNumber);
            SetClassVar(i,"viWeight",iNumber);
         }
      }

      return;
   }
```

#### MiniGameNumberToString

```

```

#### MiniGameStringToNumber

```

```

#### DumpStack

```

```
