#pragma once

#include <xmp.h>
#include <array>
#include <string>

class BottomView {
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

	xmp_module_info *m_info;

	static const std::array<const char*, 4> m_tabs;

	int m_num_files { 0 };
	int m_scroll { 0 };
	int m_selection { 0 };
};

