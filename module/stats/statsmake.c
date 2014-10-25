// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * charmake.c:  Manage main character creation tabbed dialog.
 */

#include "client.h"
#include "stats.h"

// Info for each tab of tabbed dialog
typedef struct {
   int     name;          // Resource id of name
   int     template_id;   // Resource id of dialog template
   DLGPROC dialog_proc;   // Dialog procedure for tab
   Bool    guest;         // True if we should show tab for guest accounts
} TabPage;

// Window handles of modeless dialogs, one per tab
TabPage tab_pages[] = {
{ IDS_CHARSTATS,      IDD_CHARSTATS,      CharStatsDialogProc,  True, },
};

#define NUM_TAB_PAGES (sizeof(tab_pages) / sizeof(TabPage))

// Constants for indices into tab_pages
#define TAB_NAME       0
#define TAB_APPEARANCE 1
#define TAB_STATS      2
#define TAB_SPELLS     3
#define TAB_SKILLS     4

HWND hMakeCharDialog = NULL;

int spell_points;                       // Number of points left

static HWND hTab;      // Handle of tab control
static HWND hTabPage;  // Handle of currently active child modeless dialog, depending on active tab

static Bool enter_game;// True when user fills in dialog and hits OK

extern ID    char_to_use;     /* ID # of character player wants to use in game */
extern char  name_to_use[];   /* name of character player wants to use in game */

static int  CALLBACK MakeCharSheetInit(HWND hDlg, UINT uMsg, LPARAM lParam);
/********************************************************************/
/*
 * MakeChar:  Bring up character creation dialog.
 */
void MakeChar(int *stats_in)
{
   int i;
   PROPSHEETHEADER psh;
   PROPSHEETPAGE psp[NUM_TAB_PAGES];

   initStatsFromServer(stats_in);

   // Prepare property sheet header
   psh.dwSize = sizeof(PROPSHEETHEADER);
   psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_NOAPPLYNOW;
   psh.hwndParent = cinfo->hMain;
   psh.hInstance = hInst;
   psh.pszCaption = GetString(hInst, IDS_DIALOGTITLE);
   psh.nPages = 0;
   psh.nStartPage = 0;
   psh.ppsp = (LPCPROPSHEETPAGE) &psp;
   psh.pfnCallback = MakeCharSheetInit;

   // Prepare property sheets
   for (i=0; i < NUM_TAB_PAGES; i++)
   {
      // Skip non-guest pages if we're a guest
      if (cinfo->config->guest && !tab_pages[i].guest)
	 continue;
      
      psh.nPages++;
      psp[i].dwSize = sizeof(PROPSHEETPAGE);
      psp[i].dwFlags = PSP_USETITLE;
      psp[i].hInstance = hInst;
      psp[i].pszTemplate = MAKEINTRESOURCE(tab_pages[i].template_id);
      psp[i].pfnDlgProc = (DLGPROC) tab_pages[i].dialog_proc;
      psp[i].pszTitle = MAKEINTRESOURCE(tab_pages[i].name);
      psp[i].lParam = i;
      psp[i].pfnCallback = NULL;
   }
   
   enter_game = False;

   PropertySheet(&psh);   

   if (exiting)
      PostMessage(cinfo->hMain, BK_MODULEUNLOAD, 0, MODULE_ID);
}
/*******************************************************************/
/*
 * VerifySettings:  User has pressed Done button; check values of all settings.
 *   If settings are OK, send them to server.
 */
Bool VerifySettings(void)
{
   
   int stats[NUM_CHAR_STATS] = {0,0,0,0,0,0};
   
   // Fill in stat values
   CharStatsGetChoices(stats);
   debug(("CharStatsGetChoices() returned {%i,%i,%i,%i,%i,%i}\n", stats[0], stats[1], stats[2], stats[3], stats[4], stats[5] ));

   SendNewCharInfo(stats[0], stats[1], stats[2], stats[3], stats[4], stats[5] );

   EnableWindow(GetDlgItem(hMakeCharDialog, IDOK), FALSE);
   return True;
}

/********************************************************************/
int CALLBACK MakeCharSheetInit(HWND hDlg, UINT uMsg, LPARAM lParam)
{
   int style;
  
   if (uMsg != PSCB_INITIALIZED)
      return 0;

   // Remove context-sensitive help button
   style = GetWindowLong(hDlg, GWL_EXSTYLE);
   SetWindowLong(hDlg, GWL_EXSTYLE, style & (~WS_EX_CONTEXTHELP));

   hMakeCharDialog = hDlg;

   hTab = PropSheet_GetTabControl(hDlg);

   return 0;
}
/********************************************************************/
void CharTabPageCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   UserDidSomething();

   switch (id)
   {
   case IDC_PREVIOUS_PAGE:
      PropSheet_SetCurSel(hMakeCharDialog, 0, TabCtrl_GetCurSel(hTab) - 1);
      return;

   case IDC_NEXT_PAGE:
      PropSheet_SetCurSel(hMakeCharDialog, 0, TabCtrl_GetCurSel(hTab) + 1);
      return;

   case IDC_OK:
	   return;
   }
}
/********************************************************************/
void CharInfoValid(void)
{
   enter_game = True;
   if (hMakeCharDialog != NULL)
      EndDialog(hMakeCharDialog, IDOK);

   /* Player has made new character; start game with this char */
   //GoToGame(char_to_use);
}
/********************************************************************/
void CharInfoInvalid(void)
{
   if (hMakeCharDialog != NULL)
   {
      ClientError(hInst, hMakeCharDialog, IDS_CHARNAMEUSED);
      EnableWindow(GetDlgItem(hMakeCharDialog, IDOK), TRUE);
   }
}

