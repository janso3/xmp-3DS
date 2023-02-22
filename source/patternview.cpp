#include "patternview.h"
#include "player.h"

#include <3ds.h>
#include <xmp.h>

#include <algorithm>
#include <stdio.h>

PatternView::PatternView()
{
	static const char *notes[] = {
		"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};

	for (size_t i = 0; i < MAX_NOTES; ++i)
		m_notes[i] = notes[i % 12] + std::to_string(i / 12);
}

PatternView::~PatternView()
{
}

void PatternView::Render()
{
	if (!Player::the().HasLoadedModule())
		return;

	auto xmp_ctx = Player::the().GetXmpContext();
	auto info = Player::the().GetModuleInfo();

	xmp_frame_info frame;
	xmp_get_frame_info(xmp_ctx, &frame);

	if (frame.row == m_last_row)
		return;
	m_last_row = frame.row;

	auto chn = info.mod->chn;

	printf(CONSOLE_BLUE "\x1b[0;0HCH ");
	for (int i = 0; i < 8; ++i)
	{
		int channel = m_channel_scroll + i;
		if (channel >= chn)
			break;
		printf("[%02d ]%s", channel, i == 7 ? "" : " ");
	}

	int row_offset = std::min(std::max(0, frame.row - 13), frame.num_rows - 28);
	for (int visual_row = 0; visual_row < 28; ++visual_row)
	{
		int row = visual_row + row_offset;

		printf("\x1b[%d;0H" CONSOLE_BLUE "%02x" CONSOLE_RESET "%c",
			   visual_row + 2,
			   row,
			   row == frame.row ? '>' : ' ');

		for (int visual_channel = 0; visual_channel < 8; ++visual_channel)
		{
			int channel = visual_channel + m_channel_scroll;
			if (channel >= chn)
				break;

			xmp_pattern *xxp = info.mod->xxp[frame.pattern];
			xmp_track *xxt = info.mod->xxt[xxp->index[channel]];
			xmp_event *ev = &xxt->event[row];

			unsigned char note = ev->note;
			unsigned char ins = ev->ins;

			// Note
			if (!note || note >= MAX_NOTES)
				printf("---");
			else
				printf("%s", m_notes[note - 1].c_str());

			// Instrument
			printf(CONSOLE_CYAN);

			if (ins > 0)
				printf("%02x", ins);
			else
				printf("--");

			printf(CONSOLE_RESET " ");
		}
	}
}

void PatternView::Invalidate()
{
	m_channel_scroll = 0;
	m_last_row = 0;
	consoleClear();
}

void PatternView::ScrollLeft()
{
	if (m_channel_scroll)
		m_channel_scroll--;
}

void PatternView::ScrollRight()
{
	auto info = Player::the().GetModuleInfo();
	if (!info.mod)
		return;

	int chn = info.mod->chn;
	m_channel_scroll = std::min(m_channel_scroll + 1, std::max(0, chn - 8));
}
