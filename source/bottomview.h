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
	void RenderLoadMenu();
	void RenderInfo();
	void RenderInstruments();
	void RenderAbout();

	bool UpdateDirectoryListing();
	void FreeDirectoryListing();

	xmp_module_info *m_info;

	static const std::array<const char *, 4> m_tabs;

	dirent **m_directory_listing;
	int m_directory_entries{0};

	int m_scroll{0};
	int m_selection{0};
};
