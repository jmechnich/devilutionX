#include "selgame.h"

#include <fmt/format.h>

#include "DiabloUI/diabloui.h"
#include "DiabloUI/dialogs.h"
#include "DiabloUI/selhero.h"
#include "DiabloUI/selok.h"
#include "config.h"
#include "control.h"
#include "menu.h"
#include "options.h"
#include "storm/storm.h"
#include "utils/language.h"

namespace devilution {

char selgame_Label[32];
char selgame_Ip[129] = "";
char selgame_Password[16] = "";
char selgame_Description[256];
bool selgame_enteringGame;
int selgame_selectedGame;
bool selgame_endMenu;
int *gdwPlayerId;
_difficulty nDifficulty;
int nTickRate;
int heroLevel;

static GameData *m_game_data;
extern int provider;

#define DESCRIPTION_WIDTH 205

namespace {

const char *title = "";

std::vector<std::unique_ptr<UiListItem>> vecSelGameDlgItems;
std::vector<std::unique_ptr<UiItemBase>> vecSelGameDialog;
std::vector<std::string> Gamelist;
int HighlightedItem;

constexpr const char *DefaultPassword = "asd";

} // namespace

void selgame_FreeVectors()
{
	vecSelGameDlgItems.clear();

	vecSelGameDialog.clear();
}

void selgame_Free()
{
	ArtBackground.Unload();

	selgame_FreeVectors();
}

void selgame_GameSelection_Init()
{
	selgame_enteringGame = false;
	selgame_selectedGame = 0;

	if (provider == SELCONN_LOOPBACK) {
		selgame_enteringGame = true;
		selgame_GameSelection_Select(0);
		return;
	}

	strcpy(selgame_Ip, sgOptions.Network.szPreviousHost);

	selgame_FreeVectors();

	UiAddBackground(&vecSelGameDialog);
	UiAddLogo(&vecSelGameDialog);

	SDL_Rect rect1 = { (Sint16)(PANEL_LEFT + 24), (Sint16)(UI_OFFSET_Y + 161), 590, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(_(ConnectionNames[provider]), rect1, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	SDL_Rect rect2 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 211), 205, 192 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(_("Description:"), rect2, UiFlags::FontSize24 | UiFlags::ColorUiSilver));

	SDL_Rect rect3 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 256), DESCRIPTION_WIDTH, 192 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(selgame_Description, rect3, UiFlags::FontSize12 | UiFlags::ColorUiSilverDark, 1, 16));

	SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 300), (Sint16)(UI_OFFSET_Y + 211), 295, 33 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(_("Select Action"), rect4, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Create Game"), 0));
	vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Join Game"), 1));

	for (unsigned i = 0; i < Gamelist.size(); i++) {
		vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(Gamelist[i].c_str(), i + 2));
	}

	vecSelGameDialog.push_back(std::make_unique<UiList>(vecSelGameDlgItems, PANEL_LEFT + 305, (UI_OFFSET_Y + 255), 285, 26, UiFlags::AlignCenter | UiFlags::FontSize24 | UiFlags::ColorUiGold));

	SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("OK"), &UiFocusNavigationSelect, rect5, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

	SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("CANCEL"), &UiFocusNavigationEsc, rect6, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

	UiInitList(vecSelGameDlgItems.size(), selgame_GameSelection_Focus, selgame_GameSelection_Select, selgame_GameSelection_Esc, vecSelGameDialog, true, nullptr, HighlightedItem);
}

void selgame_GameSelection_Focus(int value)
{
	HighlightedItem = value;
	switch (vecSelGameDlgItems[value]->m_value) {
	case 0:
		strncpy(selgame_Description, _("Create a new game with a difficulty setting of your choice."), sizeof(selgame_Description) - 1);
		break;
	case 1:
		strncpy(selgame_Description, _("Enter an IP or a hostname and join a game already in progress at that address."), sizeof(selgame_Description) - 1);
		break;
	default:
		strncpy(selgame_Description, _("Join the public game already in progress at this address."), sizeof(selgame_Description) - 1);
		break;
	}
	const std::string wrapped = WordWrapString(selgame_Description, DESCRIPTION_WIDTH);
	strncpy(selgame_Description, wrapped.data(), sizeof(selgame_Description) - 1);
}

/**
 * @brief Load the current hero level from save file
 * @param pInfo Hero info
 * @return always true
 */
bool UpdateHeroLevel(_uiheroinfo *pInfo)
{
	if (pInfo->saveNumber == gSaveNumber)
		heroLevel = pInfo->level;

	return true;
}

void selgame_GameSelection_Select(int value)
{
	selgame_enteringGame = true;
	selgame_selectedGame = value;

	if (value > 1 && selgame_selectedGame != 0) {
		strcpy(selgame_Ip, Gamelist[value - 2].c_str());
		strcpy(selgame_Password, DefaultPassword);
		selgame_Password_Select(value);
		return;
	}

	gfnHeroInfo(UpdateHeroLevel);

	selgame_FreeVectors();

	UiAddBackground(&vecSelGameDialog);
	UiAddLogo(&vecSelGameDialog);

	SDL_Rect rect1 = { (Sint16)(PANEL_LEFT + 24), (Sint16)(UI_OFFSET_Y + 161), 590, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(&title, rect1, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	SDL_Rect rect2 = { (Sint16)(PANEL_LEFT + 34), (Sint16)(UI_OFFSET_Y + 211), 205, 33 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(selgame_Label, rect2, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	SDL_Rect rect3 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 256), DESCRIPTION_WIDTH, 192 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(selgame_Description, rect3, UiFlags::FontSize12 | UiFlags::ColorUiSilverDark, 1, 16));

	switch (value) {
	case 0: {
		title = _("Create Game");

		SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 211), 295, 35 };
		vecSelGameDialog.push_back(std::make_unique<UiArtText>(_("Select Difficulty"), rect4, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

		vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Normal"), DIFF_NORMAL));
		vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Nightmare"), DIFF_NIGHTMARE));
		vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Hell"), DIFF_HELL));

		vecSelGameDialog.push_back(std::make_unique<UiList>(vecSelGameDlgItems, PANEL_LEFT + 300, (UI_OFFSET_Y + 282), 295, 26, UiFlags::AlignCenter | UiFlags::FontSize24 | UiFlags::ColorUiGold));

		SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
		vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("OK"), &UiFocusNavigationSelect, rect5, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

		SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
		vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("CANCEL"), &UiFocusNavigationEsc, rect6, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

		UiInitList(vecSelGameDlgItems.size(), selgame_Diff_Focus, selgame_Diff_Select, selgame_Diff_Esc, vecSelGameDialog, true);
		break;
	}
	case 1: {
		title = _("Join TCP Games");

		SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 305), (Sint16)(UI_OFFSET_Y + 211), 285, 33 };
		vecSelGameDialog.push_back(std::make_unique<UiArtText>(_("Enter address"), rect4, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

		SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 305), (Sint16)(UI_OFFSET_Y + 314), 285, 33 };
		vecSelGameDialog.push_back(std::make_unique<UiEdit>(_("Enter address"), selgame_Ip, 128, rect5, UiFlags::FontSize24 | UiFlags::ColorUiGold));

		SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
		vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("OK"), &UiFocusNavigationSelect, rect6, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

		SDL_Rect rect7 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
		vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("CANCEL"), &UiFocusNavigationEsc, rect7, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

		HighlightedItem = 0;
		UiInitList(0, nullptr, selgame_Password_Init, selgame_GameSelection_Init, vecSelGameDialog);
		break;
	}
	}
}

void selgame_GameSelection_Esc()
{
	UiInitList_clear();
	selgame_enteringGame = false;
	selgame_endMenu = true;
}

void selgame_Diff_Focus(int value)
{
	switch (vecSelGameDlgItems[value]->m_value) {
	case DIFF_NORMAL:
		strncpy(selgame_Label, _("Normal"), sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, _("Normal Difficulty\nThis is where a starting character should begin the quest to defeat Diablo."), sizeof(selgame_Description) - 1);
		break;
	case DIFF_NIGHTMARE:
		strncpy(selgame_Label, _("Nightmare"), sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, _("Nightmare Difficulty\nThe denizens of the Labyrinth have been bolstered and will prove to be a greater challenge. This is recommended for experienced characters only."), sizeof(selgame_Description) - 1);
		break;
	case DIFF_HELL:
		strncpy(selgame_Label, _("Hell"), sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, _("Hell Difficulty\nThe most powerful of the underworld's creatures lurk at the gateway into Hell. Only the most experienced characters should venture in this realm."), sizeof(selgame_Description) - 1);
		break;
	}
	const std::string wrapped = WordWrapString(selgame_Description, DESCRIPTION_WIDTH);
	strncpy(selgame_Description, wrapped.data(), sizeof(selgame_Description) - 1);
}

bool IsDifficultyAllowed(int value)
{
	if (value == 0 || (value == 1 && heroLevel >= 20) || (value == 2 && heroLevel >= 30)) {
		return true;
	}

	selgame_Free();

	if (value == 1)
		UiSelOkDialog(title, _("Your character must reach level 20 before you can enter a multiplayer game of Nightmare difficulty."), false);
	if (value == 2)
		UiSelOkDialog(title, _("Your character must reach level 30 before you can enter a multiplayer game of Hell difficulty."), false);

	LoadBackgroundArt("ui_art\\selgame.pcx");

	return false;
}

void selgame_Diff_Select(int value)
{
	if (selhero_isMultiPlayer && !IsDifficultyAllowed(vecSelGameDlgItems[value]->m_value)) {
		selgame_GameSelection_Select(0);
		return;
	}

	nDifficulty = (_difficulty)vecSelGameDlgItems[value]->m_value;

	if (!selhero_isMultiPlayer) {
		// This is part of a dangerous hack to enable difficulty selection in single-player.
		// FIXME: Dialogs should not refer to each other's variables.

		// We're in the selhero loop instead of the selgame one.
		// Free the selgame data and flag the end of the selhero loop.
		selhero_endMenu = true;

		// We only call FreeVectors because ArtBackground.Unload()
		// will be called by selheroFree().
		selgame_FreeVectors();

		// We must clear the InitList because selhero's loop will perform
		// one more iteration after this function exits.
		UiInitList_clear();

		return;
	}

	selgame_GameSpeedSelection();
}

void selgame_Diff_Esc()
{
	if (!selhero_isMultiPlayer) {
		selgame_Free();

		selhero_Init();
		selhero_List_Init();
		return;
	}

	if (provider == SELCONN_LOOPBACK) {
		selgame_GameSelection_Esc();
		return;
	}

	HighlightedItem = 0;
	selgame_GameSelection_Init();
}

void selgame_GameSpeedSelection()
{
	gfnHeroInfo(UpdateHeroLevel);

	selgame_FreeVectors();

	UiAddBackground(&vecSelGameDialog);
	UiAddLogo(&vecSelGameDialog);

	SDL_Rect rect1 = { (Sint16)(PANEL_LEFT + 24), (Sint16)(UI_OFFSET_Y + 161), 590, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(_("Create Game"), rect1, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	SDL_Rect rect2 = { (Sint16)(PANEL_LEFT + 34), (Sint16)(UI_OFFSET_Y + 211), 205, 33 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(selgame_Label, rect2, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	SDL_Rect rect3 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 256), DESCRIPTION_WIDTH, 192 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(selgame_Description, rect3, UiFlags::FontSize12 | UiFlags::ColorUiSilverDark, 1, 16));

	SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 211), 295, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(_("Select Game Speed"), rect4, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Normal"), 20));
	vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Fast"), 30));
	vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Faster"), 40));
	vecSelGameDlgItems.push_back(std::make_unique<UiListItem>(_("Fastest"), 50));

	vecSelGameDialog.push_back(std::make_unique<UiList>(vecSelGameDlgItems, PANEL_LEFT + 300, (UI_OFFSET_Y + 279), 295, 26, UiFlags::AlignCenter | UiFlags::FontSize24 | UiFlags::ColorUiGold));

	SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("OK"), &UiFocusNavigationSelect, rect5, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

	SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("CANCEL"), &UiFocusNavigationEsc, rect6, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

	UiInitList(vecSelGameDlgItems.size(), selgame_Speed_Focus, selgame_Speed_Select, selgame_Speed_Esc, vecSelGameDialog, true);
}

void selgame_Speed_Focus(int value)
{
	switch (vecSelGameDlgItems[value]->m_value) {
	case 20:
		strncpy(selgame_Label, _("Normal"), sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, _("Normal Speed\nThis is where a starting character should begin the quest to defeat Diablo."), sizeof(selgame_Description) - 1);
		break;
	case 30:
		strncpy(selgame_Label, _("Fast"), sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, _("Fast Speed\nThe denizens of the Labyrinth have been hastened and will prove to be a greater challenge. This is recommended for experienced characters only."), sizeof(selgame_Description) - 1);
		break;
	case 40:
		strncpy(selgame_Label, _("Faster"), sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, _("Faster Speed\nMost monsters of the dungeon will seek you out quicker than ever before. Only an experienced champion should try their luck at this speed."), sizeof(selgame_Description) - 1);
		break;
	case 50:
		strncpy(selgame_Label, _("Fastest"), sizeof(selgame_Label) - 1);
		strncpy(selgame_Description, _("Fastest Speed\nThe minions of the underworld will rush to attack without hesitation. Only a true speed demon should enter at this pace."), sizeof(selgame_Description) - 1);
		break;
	}
	const std::string wrapped = WordWrapString(selgame_Description, DESCRIPTION_WIDTH);
	strncpy(selgame_Description, wrapped.data(), sizeof(selgame_Description) - 1);
}

void selgame_Speed_Esc()
{
	selgame_GameSelection_Select(0);
}

void selgame_Speed_Select(int value)
{
	nTickRate = vecSelGameDlgItems[value]->m_value;

	if (provider == SELCONN_LOOPBACK) {
		selgame_Password_Select(0);
		return;
	}

	selgame_Password_Init(0);
}

void selgame_Password_Init(int /*value*/)
{
	memset(&selgame_Password, 0, sizeof(selgame_Password));

	selgame_FreeVectors();

	UiAddBackground(&vecSelGameDialog);
	UiAddLogo(&vecSelGameDialog);

	SDL_Rect rect1 = { (Sint16)(PANEL_LEFT + 24), (Sint16)(UI_OFFSET_Y + 161), 590, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(_(ConnectionNames[provider]), rect1, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	SDL_Rect rect2 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 211), 205, 192 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(_("Description:"), rect2, UiFlags::FontSize24 | UiFlags::ColorUiSilver));

	SDL_Rect rect3 = { (Sint16)(PANEL_LEFT + 35), (Sint16)(UI_OFFSET_Y + 256), DESCRIPTION_WIDTH, 192 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(selgame_Description, rect3, UiFlags::FontSize12 | UiFlags::ColorUiSilverDark, 1, 16));

	SDL_Rect rect4 = { (Sint16)(PANEL_LEFT + 305), (Sint16)(UI_OFFSET_Y + 211), 285, 33 };
	vecSelGameDialog.push_back(std::make_unique<UiArtText>(_("Enter Password"), rect4, UiFlags::AlignCenter | UiFlags::FontSize30 | UiFlags::ColorUiSilver, 3));

	SDL_Rect rect5 = { (Sint16)(PANEL_LEFT + 305), (Sint16)(UI_OFFSET_Y + 314), 285, 33 };
	vecSelGameDialog.push_back(std::make_unique<UiEdit>(_("Enter Password"), selgame_Password, 15, rect5, UiFlags::FontSize24 | UiFlags::ColorUiGold));

	SDL_Rect rect6 = { (Sint16)(PANEL_LEFT + 299), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("OK"), &UiFocusNavigationSelect, rect6, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

	SDL_Rect rect7 = { (Sint16)(PANEL_LEFT + 449), (Sint16)(UI_OFFSET_Y + 427), 140, 35 };
	vecSelGameDialog.push_back(std::make_unique<UiArtTextButton>(_("CANCEL"), &UiFocusNavigationEsc, rect7, UiFlags::AlignCenter | UiFlags::VerticalCenter | UiFlags::FontSize30 | UiFlags::ColorUiGold));

	UiInitList(0, nullptr, selgame_Password_Select, selgame_Password_Esc, vecSelGameDialog);
}

static bool IsGameCompatible(const GameData &data)
{
	if (data.versionMajor == PROJECT_VERSION_MAJOR
	    && data.versionMinor == PROJECT_VERSION_MINOR
	    && data.versionPatch == PROJECT_VERSION_PATCH
	    && data.programid == GAME_ID) {
		return IsDifficultyAllowed(data.nDifficulty);
	}

	selgame_Free();

	if (data.programid != GAME_ID) {
		UiSelOkDialog(title, _("The host is running a different game than you."), false);
	} else {
		char msg[128];
		strcpy(msg, fmt::format(_(/* TRANSLATORS: Error message when somebody tries to join a game running another version. */ "Your version {:s} does not match the host {:d}.{:d}.{:d}."), PROJECT_VERSION, data.versionMajor, data.versionMinor, data.versionPatch).c_str());

		UiSelOkDialog(title, msg, false);
	}

	LoadBackgroundArt("ui_art\\selgame.pcx");

	return false;
}

void selgame_Password_Select(int /*value*/)
{
	if (selgame_selectedGame != 0) {
		strcpy(sgOptions.Network.szPreviousHost, selgame_Ip);
		if (SNetJoinGame(selgame_Ip, selgame_Password, gdwPlayerId)) {
			if (!IsGameCompatible(*m_game_data)) {
				selgame_GameSelection_Select(1);
				return;
			}

			UiInitList_clear();
			selgame_endMenu = true;
		} else {
			selgame_Free();
			UiSelOkDialog(_("Multi Player Game"), SDL_GetError(), false);
			LoadBackgroundArt("ui_art\\selgame.pcx");
			selgame_Password_Init(selgame_selectedGame);
		}
		return;
	}

	m_game_data->nDifficulty = nDifficulty;
	m_game_data->nTickRate = nTickRate;
	m_game_data->bRunInTown = sgOptions.Gameplay.bRunInTown ? 1 : 0;
	m_game_data->bTheoQuest = sgOptions.Gameplay.bTheoQuest ? 1 : 0;
	m_game_data->bCowQuest = sgOptions.Gameplay.bCowQuest ? 1 : 0;

	if (SNetCreateGame(nullptr, selgame_Password, (char *)m_game_data, sizeof(*m_game_data), gdwPlayerId)) {
		UiInitList_clear();
		selgame_endMenu = true;
	} else {
		selgame_Free();
		UiSelOkDialog(_("Multi Player Game"), SDL_GetError(), false);
		LoadBackgroundArt("ui_art\\selgame.pcx");
		selgame_Password_Init(0);
	}
}

void selgame_Password_Esc()
{
	if (selgame_selectedGame == 1)
		selgame_GameSelection_Select(1);
	else
		selgame_GameSpeedSelection();
}

void RefreshGameList()
{
	static uint32_t lastRequest = 0;
	static uint32_t lastUpdate = 0;

	if (selgame_enteringGame)
		return;

	uint32_t currentTime = SDL_GetTicks();

	if (lastRequest == 0 || currentTime - lastRequest > 30000) {
		DvlNet_SendInfoRequest();
		lastRequest = currentTime;
		lastUpdate = currentTime - 3000; // Give 2 sec for responses, but don't wait 5
	}

	if (lastUpdate == 0 || currentTime - lastUpdate > 5000) {
		std::vector<std::string> gamelist = DvlNet_GetGamelist();
		Gamelist.clear();
		for (unsigned i = 0; i < gamelist.size(); i++) {
			Gamelist.push_back(gamelist[i]);
		}
		selgame_GameSelection_Init();
		lastUpdate = currentTime;
	}
}

bool UiSelectGame(GameData *gameData, int *playerId)
{
	gdwPlayerId = playerId;
	m_game_data = gameData;
	LoadBackgroundArt("ui_art\\selgame.pcx");
	HighlightedItem = 0;
	selgame_GameSelection_Init();

	selgame_endMenu = false;

	DvlNet_SetPassword(DefaultPassword);
	DvlNet_ClearGamelist();

	while (!selgame_endMenu) {
		UiClearScreen();
		UiPollAndRender();
		RefreshGameList();
	}
	selgame_Free();

	return selgame_enteringGame;
}
} // namespace devilution
