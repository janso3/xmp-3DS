#pragma once

#include <xmp.h>
#include <string>
#include <array>

class PatternView {
public:
	PatternView();
	~PatternView();

	void Render();
	void Invalidate();

	void ScrollLeft();
	void ScrollRight();

private:
	xmp_module_info *m_info;

	int m_channel_scroll { 0 };
	int m_last_row { 0 };

	static constexpr size_t MAX_NOTES = 120;
	std::array<std::string, MAX_NOTES> m_notes;
};
