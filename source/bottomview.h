#pragma once

#include <xmp.h>
#include <array>
#include <string>
#include <dirent.h>

class BottomView
{
public:
	BottomView();
	~BottomView();

	void Update();
	bool Select(); // Returns whether a new module has been selected
	void GoBackDirectory();

	void ScrollUp();
	void ScrollDown();
	void PrevPage();
	void NextPage();

private:
	enum Tab : int
	{
		LOAD = 0,
		INFO,
		INSTRUMENTS,
		ABOUT,
		NUM_TABS
	};

	int &ScrollValue() { return m_scroll.at(m_selection); }

	void RenderLoadMenu();
	void RenderInfo();
	void RenderInstruments();
	void RenderAbout();

	bool UpdateDirectoryListing();
	void FreeDirectoryListing();

	xmp_module_info *m_info;

	static const std::array<const char *, NUM_TABS> m_tabs;

	dirent **m_directory_listing{nullptr};
	int m_directory_entries{0};

	std::array<int, NUM_TABS> m_scroll;
	int m_selection{0};
};
