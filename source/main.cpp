#include "player.h"
#include "patternview.h"
#include "bottomview.h"

#include <3ds.h>
#include <citro2d.h>

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	gfxInitDefault();

	PrintConsole top_screen, bottom_screen;

	consoleInit(GFX_TOP, &top_screen);
	consoleInit(GFX_BOTTOM, &bottom_screen);

	auto &player = Player::the();

	PatternView pattern_view;

	BottomView bottom_view;
	bottom_view.Update();

	hidSetRepeatParameters(20, 5);

	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysDownRepeat();

		if (kDown & KEY_START)
			break;

		if (kDown & KEY_SELECT)
			player.TogglePause();

		if (kHeld & KEY_DLEFT)
			pattern_view.ScrollLeft();
		if (kHeld & KEY_DRIGHT)
			pattern_view.ScrollRight();

		consoleSelect(&top_screen);
		pattern_view.Render();

		consoleSelect(&bottom_screen);

		if (kHeld & KEY_DUP)
			bottom_view.ScrollUp();
		if (kHeld & KEY_DDOWN)
			bottom_view.ScrollDown();

		if (kDown & KEY_L)
			bottom_view.PrevPage();
		if (kDown & KEY_R)
			bottom_view.NextPage();
		if (kDown & KEY_A)
		{
			if (bottom_view.Select())
			{
				consoleSelect(&top_screen);
				pattern_view.Invalidate();
			}
		}
		if (kDown & KEY_B)
			bottom_view.GoBackDirectory();

		gfxFlushBuffers();
		gfxSwapBuffers();

		gspWaitForVBlank();
	}

	gfxExit();

	return 0;
}
