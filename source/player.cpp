#include "player.h"

#include <3ds.h>
#include <xmp.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

int Player::LogXmpError(int rc) {
	switch (rc) {
		case -XMP_ERROR_INTERNAL:
			fprintf(stderr, "XMP: Internal error\n");
			break;
		case -XMP_ERROR_FORMAT:
			fprintf(stderr, "XMP: Unsupported module format\n");
			break;
		case -XMP_ERROR_LOAD:
			fprintf(stderr, "XMP: Error loading file\n");
			break;
		case -XMP_ERROR_DEPACK:
			fprintf(stderr, "XMP: Error depacking file\n");
			break;
		case -XMP_ERROR_SYSTEM:
			fprintf(stderr, "XMP: %s\n", strerror(errno));
			break;
		case -XMP_ERROR_STATE:
			fprintf(stderr, "XMP: Incorrect player state\n");
			break;
		default:
			break;
	}

	return rc;
}

void Player::AudioCallback(void *user_data) {
	auto *player = static_cast<Player*>(user_data);

	ndspWaveBuf *wave_buf = player->m_wave_buf;	
	static int fill_block = 0;

	if (wave_buf[fill_block].status == NDSP_WBUF_DONE) {
		size_t size = wave_buf[fill_block].nsamples * sizeof(int16_t) * 2;
		if (player->m_playing)
			xmp_play_buffer(player->m_xmp_ctx, wave_buf[fill_block].data_pcm16, size, 0);
		DSP_FlushDataCache(wave_buf[fill_block].data_pcm16, size);
		ndspChnWaveBufAdd(0, &wave_buf[fill_block]);
		fill_block ^= 1;
	}
}

Player::Player() {
	ndspInit();

	ndspChnReset(0);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
	ndspChnSetRate(0, AUDIO_SPS);
	ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE);

	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = 1.0;
	mix[1] = 1.0;
	ndspChnSetMix(0, mix);

	m_audio_buffer = (int16_t*)linearAlloc(AUDIO_SAMPLESPERBUF * sizeof(int16_t) * 2 * 2);
	
	m_wave_buf[0].data_vaddr = &m_audio_buffer[0];
	m_wave_buf[0].nsamples = AUDIO_SAMPLESPERBUF;
	m_wave_buf[1].data_vaddr = &m_audio_buffer[AUDIO_SAMPLESPERBUF*2];
	m_wave_buf[1].nsamples = AUDIO_SAMPLESPERBUF;	

	ndspSetCallback(&AudioCallback, this);
	ndspChnWaveBufAdd(0, &m_wave_buf[0]);
	ndspChnWaveBufAdd(0, &m_wave_buf[1]);

	m_xmp_ctx = xmp_create_context();
}

Player::~Player() {
	ndspExit();
	if (m_audio_buffer)
		linearFree(m_audio_buffer);
	xmp_free_context(m_xmp_ctx);
}

Player& Player::the() {
	static Player player;
	return player;
}

bool Player::LoadModule(const char *path) {
	if (m_has_loaded_module) {
		memset(m_audio_buffer, 0, AUDIO_SAMPLESPERBUF * sizeof(int16_t) * 2 * 2);
		xmp_release_module(m_xmp_ctx);
		m_has_loaded_module = false;
	}

	if (LogXmpError(xmp_load_module(m_xmp_ctx, path)) != 0)
		return false;

	xmp_start_player(m_xmp_ctx, AUDIO_SPS, 0);
	xmp_get_module_info(m_xmp_ctx, &m_module_info);
	m_playing = true;

	return m_has_loaded_module = true;
}

void Player::TogglePause() {
	if (m_has_loaded_module) {
		memset(m_audio_buffer, 0, AUDIO_SAMPLESPERBUF * sizeof(int16_t) * 2 * 2);
		m_playing = !m_playing;
	}
}

