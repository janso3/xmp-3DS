#include "bottomview.h"
#include "player.h"

#include <stdio.h>
#include <dirent.h>
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
}

BottomView::~BottomView()
{
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
	bool did_load = false;
	if (m_selection == 0)
	{
		dirent *dir;
		DIR *d = opendir(".");
		if (!d)
			return false;
		int i = 0;
		while ((dir = readdir(d)) != NULL)
		{
			if (i == m_scroll)
			{
				if (dir->d_type == DT_DIR)
				{
					chdir(dir->d_name);
					m_scroll = 0;
				}
				else
				{
					Player::the().LoadModule(dir->d_name);
					did_load = true;
				}
				consoleClear();
				Update();
				break;
			}
			i++;
		}
		closedir(d);
	}
	return did_load;
}

void BottomView::GoBackDirectory()
{
	if (m_selection != 0)
		return;

	if (!chdir(".."))
	{
		consoleClear();
		m_scroll = 0;
		Update();
	}
}

void BottomView::RenderLoadMenu()
{
	dirent *dir;
	DIR *d = opendir(".");
	if (!d)
		return;

	m_scroll = std::min(m_scroll, std::max(0, m_num_files - 1));

	int i = 0, offset = std::max(0, m_scroll - 25);
	while ((dir = readdir(d)) != NULL)
	{
		int visual_row = i - offset;
		if (visual_row >= 0 && visual_row < 26)
		{
			bool is_dir = dir->d_type == DT_DIR;
			printf("\x1b[%d;0H%s%c%s%s                                      ",
				   visual_row + 1,
				   is_dir ? CONSOLE_CYAN : "",
				   i == m_scroll ? '>' : ' ',
				   dir->d_name,
				   is_dir ? "/" CONSOLE_RESET : "");
		}
		i++;
	}
	m_num_files = i;
	closedir(d);
}

void BottomView::RenderInfo()
{
	printf("\x1b[0;0H");
	if (Player::the().HasLoadedModule())
	{
		auto info = Player::the().GetModuleInfo();
		auto *mod = info.mod;

		printf("Name: %s\nType: %s\nLength: %d\n\n",
			   mod->name,
			   mod->type,
			   mod->len);
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

		printf("\x1b[%d;0H%s                                       ", i + 16, formats[format]);
	}
	m_scroll = std::min(m_scroll, i);
	printf(CONSOLE_RESET);
}
