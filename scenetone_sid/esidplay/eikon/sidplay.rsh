/**
 * @file sidplay.rsh - Common resources for ER5 and ER6
 * rsh means resource header file.
 *
 * (c) 2002 Alfred E. Heggestad
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2 as
 *    published by the Free Software Foundation.
 *
 */


/**
 * Hotkeys
 */
RESOURCE HOTKEYS r_sidplay_hotkeys
	{
	plain =
	{
		//
		// keymappings inspired by Xmms/WinAmp
		//
		// L Load tune (open tray)
		// Z Previous tune
		// X Play
		// C Pause
		// V Stop
		// B Next tune
		//
		HOTKEY { command=EEikCmdFileOpen;  key='l'; },
		HOTKEY { command=ECmdPrev;         key='z'; },
		HOTKEY { command=ECmdPlay;         key='x'; },
		HOTKEY { command=ECmdPause;        key='c'; },
		HOTKEY { command=ECmdStop;         key='v'; },
		HOTKEY { command=ECmdNext;         key='b'; },
		HOTKEY { command=EEikCmdExit;      key='q'; },
		HOTKEY { command=ECmdWavDump;      key='w'; }
	};
	control =
	{
		HOTKEY { command=EEikCmdFileOpen;        key='o'; },
		HOTKEY { command=EEikCmdExit;            key='e'; }
	};
	shift_control =
	{
		HOTKEY { command=EEikCmdHelpAbout;       key='a'; }
	};
	}


/**
 * Menu
 */
RESOURCE MENU_BAR r_sidplay_menubar
    {
    titles=
		{
        MENU_TITLE { menu_pane=r_sidplay_file_menu;   txt="File"; },
//        MENU_TITLE { menu_pane=r_sidplay_config_menu; txt="Config"; },
//        MENU_TITLE { menu_pane=r_sidplay_extra_menu;  txt="Extra"; },
        MENU_TITLE { menu_pane=r_sidplay_about_menu;  txt="About"; }
		};
    }

RESOURCE MENU_PANE r_sidplay_file_menu
	{
	items=
		{
		MENU_ITEM { command=EEikCmdFileOpen; txt="Open"; },
		MENU_ITEM { command=ECmdWavDump;     txt="WavDump"; },
		MENU_ITEM { command=EEikCmdExit;     txt="Exit"; }
		};
    }

RESOURCE MENU_PANE r_sidplay_config_menu
	{
	items=
	{
//		MENU_ITEM { command=ECmdConfigAudio;     txt="Audio"; },  -- Not applicable for ER5
//		MENU_ITEM { command=ECmdConfigEmulator;  txt="Emulator"; },
//		MENU_ITEM { command=ECmdConfigSidFilter; txt="SID filter"; },
//		MENU_ITEM { command=ECmdConfigHVSCinfo;  txt="HVSC info"; }
	};
    }

RESOURCE MENU_PANE r_sidplay_extra_menu
	{
	items=
	{
//		MENU_ITEM { command=ECmdExtraSTILview; txt="STIL view"; },
//		MENU_ITEM { command=ECmdExtraHistory;  txt="History"; },
//		MENU_ITEM { command=ECmdExtraMixer;    txt="Mixer"; },
//		MENU_ITEM { command=ECmdExtraScope;    txt="Oscilloscope"; }
	};
    }

RESOURCE MENU_PANE r_sidplay_about_menu
	{
	items=
		{
		MENU_ITEM { command=ECmdAboutSidPlay;    txt="SidPlay"; },
		MENU_ITEM { command=ECmdAboutSidTune;    txt="SidTune"; },
		MENU_ITEM { command=ECmdAboutAudio;      txt="Audio"; }
        };
    }



/**
 * Locales
 */
RESOURCE TBUF r_sidplay_appname {buf = "Sidplay";}
RESOURCE TBUF r_sidplay_play    {buf = "play";}
RESOURCE TBUF r_sidplay_stop    {buf = "stop";}
