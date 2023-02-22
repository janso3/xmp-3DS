#pragma once

#include <3ds.h>
#include <xmp.h>
#include <stdint.h>

class Player
{
public:
	static Player &the();

	bool LoadModule(const char *path);
	void TogglePause();

	xmp_context GetXmpContext() { return m_xmp_ctx; }
	xmp_module_info const &GetModuleInfo() { return m_module_info; }

	bool HasLoadedModule() { return m_has_loaded_module; }

private:
	Player();
	~Player();

	static constexpr size_t AUDIO_SAMPLESPERBUF = 512;
	static constexpr size_t AUDIO_SPS = 44100;

	bool m_has_loaded_module{false};
	bool m_playing{false};

	xmp_context m_xmp_ctx{nullptr};
	xmp_module_info m_module_info;
	int16_t *m_audio_buffer{nullptr};
	ndspWaveBuf m_wave_buf[2];

	static int LogXmpError(int rc);
	static void AudioCallback(void *d);
};
