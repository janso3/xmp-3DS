#include "bottomview.h"
#include "player.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>

#include <algorithm>
#include <string>

#define BANNER "  :::    ::: ::::    ::::  :::::::::\n"  \
			   "  :+:    :+: +:+:+: :+:+:+ :+:    :+:\n" \
			   "   +:+  +:+  +:+ +:+:+ +:+ +:+    +:+\n" \
			   "    +#++:+   +#+  +:+  +#+ +#++:++#+\n"  \
			   "   +#+  +#+  +#+       +#+ +#+\n"        \
			   "  #+#    #+# #+#       #+# #+#\n"        \
			   "  ###    ### ###       ### ###\n"        \
			   "---------------------------------------\n"

const std::array<const char *, BottomView::NUM_TABS> BottomView::m_tabs = {
	"[Load]",
	"[Ctrl]",
	"[Inst]",
	"[About]"};

BottomView::BottomView()
{
	UpdateDirectoryListing();
}

BottomView::~BottomView()
{
	FreeDirectoryListing();
}

void BottomView::ScrollUp()
{
	auto &scroll = ScrollValue();
	scroll--;
	Update();
}

void BottomView::ScrollDown()
{
	ScrollValue()++;
	Update();
}

void BottomView::PrevPage()
{
	if (m_selection)
		m_selection--;
	else
		m_selection = m_tabs.size() - 1;
	ClearTabView();
	Update();
}

void BottomView::NextPage()
{
	if (m_selection < m_tabs.size() - 1)
		m_selection++;
	else
		m_selection = 0;
	ClearTabView();
	Update();
}

void BottomView::Update()
{
	switch (m_selection)
	{
	case Tab::LOAD:
		RenderLoadMenu();
		break;
	case Tab::CTRL:
		RenderControlMenu();
		break;
	case Tab::INSTRUMENTS:
		RenderInstruments();
		break;
	case Tab::ABOUT:
		RenderAbout();
		break;
	default:
		break;
	}

	// Render tabs at the bottom
	printf("\x1b[28;0H|------ Press L/R to switch tabs ------|\n");
	for (size_t i = 0; i < m_tabs.size(); ++i)
	{
		if (m_selection == (int)i)
			printf(CONSOLE_MAGENTA);
		printf("%s " CONSOLE_RESET, m_tabs[i]);
	}
}

bool BottomView::Select()
{
	auto &scroll = ScrollValue();
	auto &player = Player::the();

	if (m_selection == Tab::LOAD)
	{
		if (scroll >= m_directory_entries)
			return false;

		auto *entry = m_directory_listing[scroll];
		if (entry->d_type == DT_DIR)
		{
			if (!chdir(entry->d_name))
			{
				scroll = 0;
				UpdateDirectoryListing();
				Update();
			}
			return false;
		}

		player.LoadModule(entry->d_name);
		return true;
	}
	else if (m_selection == Tab::CTRL)
	{
		switch (scroll)
		{
		case SUBSONG:
			player.NextSubsong();
			break;
		case LOOP:
			player.ToggleLooping();
			break;
		case PLAYBACK:
			player.ToggleStereo();
			break;
		default:
			break;
		}
		Update();
	}

	return false;
}

void BottomView::GoBackDirectory()
{
	if (m_selection != Tab::LOAD)
		return;

	if (!chdir(".."))
	{
		ScrollValue() = 0;
		UpdateDirectoryListing();
		Update();
	}
}

void BottomView::RenderLoadMenu()
{
	auto &scroll = ScrollValue();
	if (scroll < 0)
		scroll = m_directory_entries - 1;
	else if (scroll >= m_directory_entries - 1)
		scroll = 0;

	int row_offset = std::max(0, std::min(scroll - 13, m_directory_entries - 27));
	for (int visual_row = 0; visual_row < 27; ++visual_row)
	{
		int file_index = visual_row + row_offset;

		printf("\x1b[%d;0H", visual_row + 1);
		if (file_index < m_directory_entries)
		{
			auto *entry = m_directory_listing[file_index];
			bool is_dir = entry->d_type == DT_DIR;
			printf("%s%c%.39s%s",
				   is_dir ? CONSOLE_CYAN : "",
				   file_index == scroll ? '>' : ' ',
				   entry->d_name,
				   is_dir ? "/" CONSOLE_RESET : "");
		}
		printf("\x1b[K");
	}
}

void BottomView::RenderControlMenu()
{
	auto &player = Player::the();

	auto &scroll = ScrollValue();

	if (scroll < 0)
		scroll = NUM_SETTINGS - 1;
	else if (scroll >= NUM_SETTINGS)
		scroll = 0;

	// Settings
	printf("\x1b[0;0H" CONSOLE_RED "Select a setting with the D-pad\nChange setting with A\n\n" CONSOLE_RESET);

	auto PrintSelection = [&](int i)
	{
		if (i == scroll)
			printf("\x1b[47;30m");
	};

	// FIXME: Allow subsong selection
	PrintSelection(SUBSONG);
	printf("Subsong : ");
	if (player.HasLoadedModule())
	{
		auto info = player.GetModuleInfo();

		int subsong = player.GetSubsongIndex();
		int total_minutes = info.seq_data[subsong].duration / (1000 * 60);
		int total_seconds = (info.seq_data[subsong].duration / 1000) % 60;

		printf("Index %d (%dmin%ds)",
			   subsong,
			   total_minutes,
			   total_seconds);
	}
	printf(CONSOLE_RESET "\x1b[K\n");

	PrintSelection(PLAYBACK);
	printf("Playback: %s\x1b[K\n" CONSOLE_RESET, player.IsStereo() ? "Stereo" : "Mono");

	PrintSelection(LOOP);
	printf("Loop    : %s\x1b[K\n" CONSOLE_RESET, player.IsLooping() ? "Yes" : "No");

	if (player.HasLoadedModule())
	{
		auto info = player.GetModuleInfo();
		auto *mod = info.mod;

		printf(
			CONSOLE_RED "\n|------------ Module info -------------|\n" CONSOLE_RESET
						"Name: %s\nType: %s\nLength: %d patterns\n\n",
			mod->name,
			mod->type,
			mod->len);
	}
}

void BottomView::RenderInstruments()
{
	if (!Player::the().HasLoadedModule())
		return;

	auto info = Player::the().GetModuleInfo();

	xmp_instrument *xxi = info.mod->xxi;
	int num_ins = info.mod->ins;

	auto &scroll = ScrollValue();
	scroll = std::max(0, std::min(scroll, num_ins - 28));

	for (int i = 0; i < 27; ++i)
	{
		int ins = i + scroll;
		if (ins >= num_ins)
			break;

		printf("\x1b[%d;0H%02d: ", i + 1, ins);
		for (int j = 0; j < sizeof(xmp_instrument::name); ++j)
		{
			char c = xxi[ins].name[j];
			printf("%c", c ? c : ' ');
		}
	}
}

void BottomView::RenderAbout()
{
	printf("\x1b[0;0H" BANNER
		   "\nXmp Mod Player\nVersion %s\nLibrary written by Claudio Matsuoka\n3DS front-end by Julian Offenhaeuser\n\nSupported formats:\n" CONSOLE_CYAN,
		   xmp_version);
	auto &scroll = ScrollValue();
	auto formats = xmp_get_format_list();
	int i;
	for (i = 0; i < 12; ++i)
	{
		int format = i + scroll;
		if (formats[format] == NULL)
			break;

		printf("\x1b[%d;0H%s\x1b[K", i + 16, formats[format]);
	}
	scroll = std::min(std::max(0, scroll), i);
	printf(CONSOLE_RESET);
}

void BottomView::ClearTabView()
{
	printf("\x1b[0;0H");
	for (int i = 0; i < 27; ++i)
		printf("\x1b[K\n");
}

// We sort alphabetically, putting directories before files.
static int MyDirentCompare(const dirent **a, const dirent **b)
{
	const auto t1 = (*a)->d_type;
	const auto t2 = (*b)->d_type;

	// FIXME: Can this be factored a bit nicer perhaps?
	if (t1 == DT_DIR)
	{
		if (t2 == DT_DIR)
			return strcoll((*a)->d_name, (*b)->d_name);
		return -1;
	}
	else if (t2 == DT_DIR)
	{
		return 1;
	}

	return strcoll((*a)->d_name, (*b)->d_name);
}

bool BottomView::UpdateDirectoryListing()
{
	FreeDirectoryListing();
	m_directory_entries = scandir(".", &m_directory_listing, NULL, MyDirentCompare);
	if (m_directory_entries < 0)
	{
		m_directory_listing = nullptr;
		return false;
	}

	return true;
}

void BottomView::FreeDirectoryListing()
{
	if (m_directory_listing)
	{
		for (int i = 0; i < m_directory_entries; ++i)
			free(m_directory_listing[i]);
		free(m_directory_listing);
		m_directory_entries = 0;
		m_directory_listing = nullptr;
	}
}
