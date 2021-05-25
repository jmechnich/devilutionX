/**
 * @file mainmenu.cpp
 *
 * Implementation of functions for interacting with the main menu.
 */

#include "DiabloUI/diabloui.h"
#include "init.h"
#include "movie.h"
#include "options.h"
#include "pfile.h"
#include "storm/storm.h"
#include "utils/language.h"

namespace devilution {

char gszHero[16];

/* data */

/** The active music track id for the main menu. */
uint8_t menu_music_track_id = TMUSIC_INTRO;

void mainmenu_refresh_music()
{
	music_start(menu_music_track_id);

	if (gbIsSpawn && !gbIsHellfire) {
		return;
	}

	do {
		menu_music_track_id++;
		if (menu_music_track_id == NUM_MUSIC || (!gbIsHellfire && menu_music_track_id > TMUSIC_L4))
			menu_music_track_id = TMUSIC_L2;
		if (gbIsSpawn && menu_music_track_id > TMUSIC_L1)
			menu_music_track_id = TMUSIC_L5;
	} while (menu_music_track_id == TMUSIC_TOWN || menu_music_track_id == TMUSIC_L1);
}

static bool mainmenu_init_menu(_selhero_selections type)
{
	bool success;

	if (type == SELHERO_PREVIOUS)
		return true;

	music_stop();

	success = StartGame(type != SELHERO_CONTINUE, type != SELHERO_CONNECT);
	if (success)
		mainmenu_refresh_music();

	return success;
}

static bool mainmenu_single_player()
{
	gbIsMultiplayer = false;
	return mainmenu_init_menu(SELHERO_NEW_DUNGEON);
}

static bool mainmenu_multi_player()
{
	gbIsMultiplayer = true;
	return mainmenu_init_menu(SELHERO_CONNECT);
}

static void mainmenu_play_intro()
{
	music_stop();
	if (gbIsHellfire)
		play_movie("gendata\\Hellfire.smk", true);
	else
		play_movie("gendata\\diablo1.smk", true);
	mainmenu_refresh_music();
}

bool Dummy_GetHeroInfo(_uiheroinfo *pInfo)
{
	return true;
}

bool mainmenu_select_hero_dialog(GameData *gameData)
{
	_selhero_selections dlgresult = SELHERO_NEW_DUNGEON;
	if (demoMode) {
		pfile_ui_set_hero_infos(Dummy_GetHeroInfo);
		gbLoadGame = true;
	} else if (!gbIsMultiplayer) {
		UiSelHeroSingDialog(
		    pfile_ui_set_hero_infos,
		    pfile_ui_save_create,
		    pfile_delete_save,
		    pfile_ui_set_class_stats,
		    &dlgresult,
		    &gszHero,
		    &gameData->nDifficulty);

		gbLoadGame = (dlgresult == SELHERO_CONTINUE);
	} else {
		UiSelHeroMultDialog(
		    pfile_ui_set_hero_infos,
		    pfile_ui_save_create,
		    pfile_delete_save,
		    pfile_ui_set_class_stats,
		    &dlgresult,
		    &gszHero);
	}
	if (dlgresult == SELHERO_PREVIOUS) {
		SErrSetLastError(1223);
		return false;
	}

	pfile_read_player_from_save(gszHero, myplr);

	if (recordDemo != -1)
		CreateDemoFile(recordDemo);

	return true;
}

void mainmenu_loop()
{
	bool done;
	_mainmenu_selections menu;

	mainmenu_refresh_music();
	done = false;

	do {
		menu = MAINMENU_NONE;
		if (demoMode)
			menu = MAINMENU_SINGLE_PLAYER;
		else if (!UiMainMenuDialog(gszProductName, &menu, effects_play_sound, 30))
			app_fatal("%s", _("Unable to display mainmenu"));

		switch (menu) {
		case MAINMENU_NONE:
			break;
		case MAINMENU_SINGLE_PLAYER:
			if (!mainmenu_single_player())
				done = true;
			break;
		case MAINMENU_MULTIPLAYER:
			if (!mainmenu_multi_player())
				done = true;
			break;
		case MAINMENU_ATTRACT_MODE:
		case MAINMENU_REPLAY_INTRO:
			if (gbIsSpawn && !gbIsHellfire)
				done = false;
			else if (gbActive)
				mainmenu_play_intro();
			break;
		case MAINMENU_SHOW_CREDITS:
			UiCreditsDialog();
			break;
		case MAINMENU_SHOW_SUPPORT:
			UiSupportDialog();
			break;
		case MAINMENU_EXIT_DIABLO:
			done = true;
			break;
		}
	} while (!done);

	music_stop();
}

} // namespace devilution
