#include "bottomview.h"
#include "player.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>

#include <algorithm>
#include <string>

#define CONTROLS CONSOLE_RED "Controls:\n"                             \
							 "Start: Exit the app\n"                   \
							 "Select: Toggle pause\n"                  \
							 "D-pad up/down: Navigate bottom screen\n" \
							 "D-pad left/right: Scroll pattern view\n" \
							 "A: Load file or move into folder\n"      \
							 "B: Move to the previous folder\n" CONSOLE_RESET

#define BANNER "  :::    ::: ::::    ::::  :::::::::\n"  \
			   "  :+:    :+: +:+:+: :+:+:+ :+:    :+:\n" \
			   "   +:+  +:+  +:+ +:+:+ +:+ +:+    +:+\n" \
			   "    +#++:+   +#+  +:+  +#+ +#++:++#+\n"  \
			   "   +#+  +#+  +#+       +#+ +#+\n"        \
			   "  #+#    #+# #+#       #+# #+#\n"        \
			   "  ###    ### ###       ### ###\n"        \
			   "---------------------------------------\n"

const std::array<const char *, 4> BottomView::m_tabs = {
	"[Load]",
	"[Info]",
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
	if (m_scroll)
		m_scroll--;
	Update();
}

void BottomView::ScrollDown()
{
	m_scroll++;
	Update();
}

void BottomView::PrevPage()
{
	if (m_selection)
		m_selection--;
	else
		m_selection = m_tabs.size() - 1;
	m_scroll = 0;
	consoleClear();
	Update();
}

void BottomView::NextPage()
{
	if (m_selection < m_tabs.size() - 1)
		m_selection++;
	else
		m_selection = 0;
	m_scroll = 0;
	consoleClear();
	Update();
}

void BottomView::Update()
{
	switch (m_selection)
	{
	case 0:
		RenderLoadMenu();
		break;
	case 1:
		RenderInfo();
		break;
	case 2:
		RenderInstruments();
		break;
	case 3:
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
	if (m_selection != 0)
		return false;

	if (m_scroll >= m_directory_entries)
		return false;

	auto *entry = m_directory_listing[m_scroll];
	if (entry->d_type == DT_DIR)
	{
		if (!chdir(entry->d_name))
		{
			m_scroll = 0;
			UpdateDirectoryListing();
			Update();
		}
		return false;
	}

	Player::the().LoadModule(entry->d_name);
	return true;
}

void BottomView::GoBackDirectory()
{
	if (m_selection != 0)
		return;

	if (!chdir(".."))
	{
		m_scroll = 0;
		UpdateDirectoryListing();
		Update();
	}
}

void BottomView::RenderLoadMenu()
{
	m_scroll = std::min(m_scroll, m_directory_entries - 1);

	int row_offset = std::max(0, std::min(m_scroll - 13, m_directory_entries - 27));
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
				   file_index == m_scroll ? '>' : ' ',
				   entry->d_name,
				   is_dir ? "/" CONSOLE_RESET : "");
		}
		printf("\x1b[K");
	}
}

void BottomView::RenderInfo()
{
	printf("\x1b[0;0H");

	auto &player = Player::the();
	if (player.HasLoadedModule())
	{
		auto info = player.GetModuleInfo();
		auto *mod = info.mod;

		int total_minutes = info.seq_data[0].duration / (1000 * 60);
		int total_seconds = (info.seq_data[0].duration / 1000) % 60;

		printf("Name: %s\nType: %s\nLength: %d patterns\nDuration: %dmin%ds\n",
			   mod->name,
			   mod->type,
			   mod->len,
			   total_minutes,
			   total_seconds);
	}

	printf(CONTROLS);
}

void BottomView::RenderInstruments()
{
	if (!Player::the().HasLoadedModule())
		return;

	auto info = Player::the().GetModuleInfo();

	xmp_instrument *xxi = info.mod->xxi;
	int num_ins = info.mod->ins;

	m_scroll = std::min(m_scroll, std::max(0, num_ins - 28));

	for (int i = 0; i < 27; ++i)
	{
		int ins = i + m_scroll;
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
	auto formats = xmp_get_format_list();
	int i;
	for (i = 0; i < 12; ++i)
	{
		int format = i + m_scroll;
		if (formats[format] == NULL)
			break;

		printf("\x1b[%d;0H%s\x1b[K", i + 16, formats[format]);
	}
	m_scroll = std::min(m_scroll, i);
	printf(CONSOLE_RESET);
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
		m_directory_listing = NULL;
		return false;
	}

	return true;
}

void BottomView::FreeDirectoryListing()
{
	if (m_directory_entries > 0)
	{
		for (int i = 0; i < m_directory_entries; ++i)
			free(m_directory_listing[i]);
		free(m_directory_listing);
	}
}
