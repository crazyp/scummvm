/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001-2003 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#ifndef PLAYER_V2_H
#define PLAYER_V2_H

#include "common/scummsys.h"
#include "sound/mixer.h"

struct channel_data {
	uint16 time_left;          // 00
	uint16 next_cmd;           // 02
	uint16 base_freq;          // 04
	uint16 freq_delta;         // 06
	uint16 freq;               // 08
	uint16 volume;             // 10
	uint16 volume_delta;       // 12
	uint16 tempo;              // 14
	uint16 inter_note_pause;   // 16
	uint16 transpose;          // 18
	uint16 note_length;        // 20
	uint16 hull_curve;         // 22
	uint16 hull_offset;        // 24
	uint16 hull_counter;       // 26
	uint16 freqmod_table;      // 28
	uint16 freqmod_offset;     // 30
	uint16 freqmod_incr;       // 32
	uint16 freqmod_multiplier; // 34
	uint16 freqmod_modulo;     // 36
	uint16 unknown[5];         // 38 - 46
	uint16 music_script_nr;    // 48
} GCC_PACK;

union ChannelInfo {
	channel_data d;
	uint16 array[sizeof(channel_data)/2];
};



class Player_V2 {
public:
	Player_V2();
	~Player_V2();

	void startSound(int nr, byte *data);
	void stopSound(int nr);
	void stopAllSounds();
	int  getSoundStatus(int nr);

private:
	SoundMixer *_mixer;
	int _next_tick;

	int sample_rate;
	int ticks_per_sample;
	int ticks_counted;
	int samples_left;
	int freq;
	int last_freq;
	int level;
	int pcjr;
	const uint16 *freqs_table;
	unsigned int decay;

	ChannelInfo channels[4];

	int   current_nr;
	byte *current_data;
	int   next_nr;
	byte *next_data;
	byte *retaddr;

	void execute_cmd(ChannelInfo *channel);
	void next_freqs(ChannelInfo *channel);
	void generate_samples(int16 *buf, int len);
	void on_timer();
	void clear_channel(int i);
	void chainNextSound();

	static void premix_proc(void *param, int16 *buf, uint len);
	void do_mix (int16 *buf, int len);
};

#endif
