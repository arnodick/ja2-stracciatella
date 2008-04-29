#include "GameVersion.h"
#include "Local.h"
#include "SGP.h"
#include "Gameloop.h"
#include "Screens.h"
#include "ShopKeeper_Interface.h"
#include "Tactical_Placement_GUI.h"
#include "Cursors.h"
#include "Init.h"
#include "Music_Control.h"
#include "Sys_Globals.h"
#include "Laptop.h"
#include "MapScreen.h"
#include "Game_Clock.h"
#include "Overhead.h"
#include "LibraryDataBase.h"
#include "Map_Screen_Interface.h"
#include "Tactical_Save.h"
#include "Interface.h"
#include "GameSettings.h"
#include "Interface_Control.h"
#include "Text.h"
#include "HelpScreen.h"
#include "SaveLoadGame.h"
#include "Finances.h"
#include "Options_Screen.h"
#include "Debug.h"
#include "Video.h"
#include "MemMan.h"
#include "Button_System.h"
#include "Font_Control.h"

#ifdef JA2BETAVERSION
#	include "PreBattle_Interface.h"
#endif

#ifdef JA2DEMOADS
#	include "Fade_Screen.h"
#endif


UINT32 guiCurrentScreen;
UINT32 guiPendingScreen = NO_PENDING_SCREEN;
UINT32 guiPreviousScreen = NO_PENDING_SCREEN;

#define	DONT_CHECK_FOR_FREE_SPACE		255
static UINT8 gubCheckForFreeSpaceOnHardDriveCount = DONT_CHECK_FOR_FREE_SPACE;

// callback to confirm game is over
void EndGameMessageBoxCallBack( UINT8 bExitValue );

// The InitializeGame function is responsible for setting up all data and Gaming Engine
// tasks which will run the game

#ifdef JA2BETAVERSION

BOOLEAN gubReportMapscreenLock = 0;


static void ReportMapscreenErrorLock(void)
{
	switch( gubReportMapscreenLock )
	{
		case 1:
			DoScreenIndependantMessageBox( L"You have just loaded the game which is in a state that you shouldn't be able to.  You can still play, but there should be a sector with enemies co-existing with mercs.  Please don't report that.", MSG_BOX_FLAG_OK, NULL );
			fDisableDueToBattleRoster = FALSE;
			fDisableMapInterfaceDueToBattle = FALSE;
			gubReportMapscreenLock = 0;
			break;
		case 2:
			DoScreenIndependantMessageBox( L"You have just saved the game which is in a state that you shouldn't be able to.  Please report circumstances (ex:  merc in other sector pipes up about enemies), etc.  Autocorrected, but if you reload the save, don't report the error appearing in load.", MSG_BOX_FLAG_OK, NULL );
			fDisableDueToBattleRoster = FALSE;
			fDisableMapInterfaceDueToBattle = FALSE;
			gubReportMapscreenLock = 0;
			break;
	}
}
#endif

BOOLEAN InitializeGame(void)
{
	UINT32				uiIndex;

	// Initlaize mouse subsystems
	MSYS_Init( );
	InitButtonSystem();
	InitCursors( );

	// Init Fonts
	if ( !InitializeFonts( ) )
	{
		// Send debug message and quit
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "COULD NOT INUT FONT SYSTEM...");
		return( ERROR_SCREEN );
	}

	//Deletes all the Temp files in the Maps\Temp directory
	InitTacticalSave( TRUE );

	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Version Label: %s", g_version_label));
	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Version #:     %s", g_version_number));

	// Initialize Game Screens.
  for (uiIndex = 0; uiIndex < MAX_SCREENS; uiIndex++)
  {
		UINT32 (*init)(void) = GameScreens[uiIndex].InitializeScreen;
		if (init != NULL && !init())
    { // Failed to initialize one of the screens.
      return FALSE;
    }
  }

	//Init the help screen system
	InitHelpScreenSystem();

	//Loads the saved (if any) general JA2 game settings
	LoadGameSettings();

	//Initialize the Game options ( Gun nut, scifi and dif. levels
	InitGameOptions();

	// preload mapscreen graphics
	HandlePreloadOfMapGraphics( );

	guiCurrentScreen = INIT_SCREEN;

  return TRUE;
}

// The ShutdownGame function will free up/undo all things that were started in InitializeGame()
// It will also be responsible to making sure that all Gaming Engine tasks exit properly

void    ShutdownGame(void)
{
	// handle shutdown of game with respect to preloaded mapscreen graphics
	HandleRemovalOfPreLoadedMapGraphics( );

	 ShutdownJA2( );

	//Save the general save game settings to disk
	SaveGameSettings();


	 //shutdown the file database manager
	 ShutDownFileDatabase( );


	//Deletes all the Temp files in the Maps\Temp directory
	InitTacticalSave( FALSE );
}


static void InsertCommasIntoNumber(wchar_t pString[])
{
  INT16 sCounter = 0;
  INT16 sZeroCount = 0;
	INT16 sTempCounter = 0;

	// go to end of dollar figure
	while (pString[sCounter] != L'\0')
	{
		sCounter++;
	}

	// is there under $1,000?
	if (sCounter < 4)
	{
		// can't do anything, return
		return;
	}

	// at end, start backing up until beginning
  while (sCounter > 0)
	{
		// enough for a comma?
		if (sZeroCount == 3)
		{
			// reset count
			sZeroCount = 0;
      // set tempcounter to current counter
			sTempCounter = sCounter;

			// run until end
			while (pString[sTempCounter] != L'\0')
			{
				sTempCounter++;
			}
			// now shift everything over ot the right one place until sTempCounter = sCounter
			while (sTempCounter >= sCounter)
			{
				pString[sTempCounter + 1] = pString[sTempCounter];
				sTempCounter--;
			}
			// now insert comma
			pString[sCounter] = L',';
		}

		// increment count of digits
		sZeroCount++;

		// decrement counter
		sCounter--;
	}
}


static void HandleNewScreenChange(UINT32 uiNewScreen, UINT32 uiOldScreen);


// This is the main Gameloop. This should eventually by one big switch statement which represents
// the state of the game (i.e. Main Menu, PC Generation, Combat loop, etc....)
// This function exits constantly and reenters constantly
void GameLoop(void)
{
  InputAtom					InputEvent;
	UINT32	uiOldScreen=guiCurrentScreen;

	SGPPoint MousePos;
	GetMousePos(&MousePos);
	// Hook into mouse stuff for MOVEMENT MESSAGES
	MouseSystemHook(MOUSE_POS, MousePos.iX, MousePos.iY);
	MusicPoll();

  while (DequeueSpecificEvent(&InputEvent, INPUT_MOUSE))
  {
		MouseSystemHook(InputEvent.usEvent, MousePos.iX, MousePos.iY);
	}


	if ( gfGlobalError )
	{
		guiCurrentScreen = ERROR_SCREEN;
	}


	//if we are to check for free space on the hard drive
	if( gubCheckForFreeSpaceOnHardDriveCount < DONT_CHECK_FOR_FREE_SPACE )
	{
		//only if we are in a screen that can get this check
		if( guiCurrentScreen == MAP_SCREEN || guiCurrentScreen == GAME_SCREEN || guiCurrentScreen == SAVE_LOAD_SCREEN )
		{
			if( gubCheckForFreeSpaceOnHardDriveCount < 1 )
			{
				gubCheckForFreeSpaceOnHardDriveCount++;
			}
			else
			{
				// Make sure the user has enough hard drive space
				if( !DoesUserHaveEnoughHardDriveSpace() )
				{
					CHAR16	zText[512];
					CHAR16	zSpaceOnDrive[512];
					UINT32	uiSpaceOnDrive;
					CHAR16	zSizeNeeded[512];

					swprintf( zSizeNeeded, lengthof(zSizeNeeded), L"%d", REQUIRED_FREE_SPACE / BYTESINMEGABYTE );
					InsertCommasIntoNumber(zSizeNeeded);

					uiSpaceOnDrive = GetFreeSpaceOnHardDriveWhereGameIsRunningFrom( );

					swprintf( zSpaceOnDrive, lengthof(zSpaceOnDrive), L"%.2f", uiSpaceOnDrive / (FLOAT)BYTESINMEGABYTE );

					swprintf( zText, lengthof(zText), pMessageStrings[ MSG_LOWDISKSPACE_WARNING ], zSpaceOnDrive, zSizeNeeded );

					if( guiPreviousOptionScreen == MAP_SCREEN )
						DoMapMessageBox( MSG_BOX_BASIC_STYLE, zText, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL );
					else
						DoMessageBox( MSG_BOX_BASIC_STYLE, zText, GAME_SCREEN, MSG_BOX_FLAG_OK, NULL, NULL );
				}
				gubCheckForFreeSpaceOnHardDriveCount = DONT_CHECK_FOR_FREE_SPACE;
			}
		}
	}

	// ATE: Force to be in message box screen!
	if ( gfInMsgBox )
	{
		guiPendingScreen = MSG_BOX_SCREEN;
	}

	if ( guiPendingScreen != NO_PENDING_SCREEN )
	{
		// Based on active screen, deinit!
		if( guiPendingScreen != guiCurrentScreen )
		{
			switch( guiCurrentScreen )
			{
				case MAP_SCREEN:
					if( guiPendingScreen != MSG_BOX_SCREEN )
					{
						EndMapScreen( FALSE );
					}
					break;
				case LAPTOP_SCREEN:
					ExitLaptop();
					break;
			}
		}

		// if the screen has chnaged
		if( uiOldScreen != guiPendingScreen )
		{
			// Set the fact that the screen has changed
			uiOldScreen = guiPendingScreen;

			HandleNewScreenChange( guiPendingScreen, guiCurrentScreen );
		}
		guiCurrentScreen = guiPendingScreen;
		guiPendingScreen = NO_PENDING_SCREEN;

	}



  uiOldScreen = (*(GameScreens[guiCurrentScreen].HandleScreen))();

	// if the screen has chnaged
	if( uiOldScreen != guiCurrentScreen )
	{
		HandleNewScreenChange( uiOldScreen, guiCurrentScreen );
		guiCurrentScreen = uiOldScreen;
	}

	RefreshScreen();

	guiGameCycleCounter++;

	UpdateClock();

	#ifdef JA2BETAVERSION
	if( gubReportMapscreenLock )
	{
		ReportMapscreenErrorLock();
	}
	#endif

}


void SetPendingNewScreen( UINT32 uiNewScreen )
{
	guiPendingScreen = uiNewScreen;
}


// Gets called when the screen changes, place any needed in code in here
static void HandleNewScreenChange(UINT32 uiNewScreen, UINT32 uiOldScreen)
{
	//if we are not going into the message box screen, and we didnt just come from it
	if( ( uiNewScreen != MSG_BOX_SCREEN && uiOldScreen != MSG_BOX_SCREEN ) )
	{
		//reset the help screen
		NewScreenSoResetHelpScreen( );
	}
}

void HandleShortCutExitState( void )
{
	// look at the state of fGameIsRunning, if set false, then prompt user for confirmation

	// use YES/NO Pop up box, settup for particular screen
	const SGPRect pCenteringRect = { 0, 0, SCREEN_WIDTH, INV_INTERFACE_START_Y };

	if( guiCurrentScreen == ERROR_SCREEN )
	{ //an assert failure, don't bring up the box!
		gfProgramIsRunning = FALSE;
		return;
	}

	if( guiCurrentScreen == AUTORESOLVE_SCREEN )
	{
		DoMessageBox(MSG_BOX_BASIC_STYLE, pMessageStrings[MSG_EXITGAME], guiCurrentScreen, MSG_BOX_FLAG_YESNO, EndGameMessageBoxCallBack, &pCenteringRect);
		return;
	}

	/// which screen are we in?
	if ( (guiTacticalInterfaceFlags & INTERFACE_MAPSCREEN ) )
	{
		// set up for mapscreen
		DoMapMessageBox( MSG_BOX_BASIC_STYLE, pMessageStrings[ MSG_EXITGAME ], MAP_SCREEN, MSG_BOX_FLAG_YESNO, EndGameMessageBoxCallBack );

	}
	else if( guiCurrentScreen == LAPTOP_SCREEN )
	{
		// set up for laptop
		DoLapTopSystemMessageBox( MSG_BOX_LAPTOP_DEFAULT, pMessageStrings[ MSG_EXITGAME ], LAPTOP_SCREEN, MSG_BOX_FLAG_YESNO, EndGameMessageBoxCallBack );
	}
	else if( guiCurrentScreen == SHOPKEEPER_SCREEN )
	{
		DoSkiMessageBox( MSG_BOX_BASIC_STYLE, pMessageStrings[ MSG_EXITGAME ], SHOPKEEPER_SCREEN, MSG_BOX_FLAG_YESNO, EndGameMessageBoxCallBack );
	}
	else
	{

		// check if error or editor
#ifdef JA2BETAVERSION
		if ( guiCurrentScreen == AIVIEWER_SCREEN || guiCurrentScreen == QUEST_DEBUG_SCREEN )
		{
			// then don't prompt
			gfProgramIsRunning = FALSE;
			return;
		}
#endif

		if( ( guiCurrentScreen == ERROR_SCREEN ) || ( guiCurrentScreen == EDIT_SCREEN ) || ( guiCurrentScreen == DEBUG_SCREEN ) )
		{
			// then don't prompt
			gfProgramIsRunning = FALSE;
			return;
		}

		// set up for all otherscreens
		DoMessageBox(MSG_BOX_BASIC_STYLE, pMessageStrings[MSG_EXITGAME], guiCurrentScreen, MSG_BOX_FLAG_YESNO, EndGameMessageBoxCallBack, &pCenteringRect);
	}
}


void EndGameMessageBoxCallBack( UINT8 bExitValue )
{
	// yes, so start over, else stay here and do nothing for now
  if( bExitValue == MSG_BOX_RETURN_YES )
	{
#ifdef JA2DEMOADS
		guiPendingScreen = DEMO_EXIT_SCREEN;
		SetMusicMode( MUSIC_MAIN_MENU );
		FadeOutNextFrame( );
#else
		gfProgramIsRunning = FALSE;
#endif
	}

	//If we are in the tactical placement gui, we need this flag set so the interface is updated.
	if( gfTacticalPlacementGUIActive )
	{
		gfTacticalPlacementGUIDirty = TRUE;
		gfValidLocationsChanged = TRUE;
	}
}


void NextLoopCheckForEnoughFreeHardDriveSpace()
{
	gubCheckForFreeSpaceOnHardDriveCount = 0;
}
