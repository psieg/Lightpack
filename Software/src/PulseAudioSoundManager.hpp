/*
 * PulseAudioSoundManager.hpp
 *
 *	Created on: 25.12.2019
 *		Project: Lightpack
 *
 *	Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *	Lightpack a USB content-driving ambient lighting system
 *
 *	Lightpack is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Lightpack is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <QTime>
#include "SoundManagerBase.hpp"
#include <pulse/pulseaudio.h>
#include <vector>
#include <mutex>

#include <cmath>
#include <fftw3.h>

enum w_type
{
	WINDOW_TRIANGLE,
	WINDOW_HANNING,
	WINDOW_HAMMING,
	WINDOW_BLACKMAN,
	WINDOW_BLACKMAN_HARRIS,
	WINDOW_FLAT,
	WINDOW_WELCH,
};

class PulseAudioSoundManager : public SoundManagerBase
{
	Q_OBJECT
public:
	PulseAudioSoundManager(QObject *parent = 0);
	virtual ~PulseAudioSoundManager();

public:
	virtual void start(bool isMoodLampEnabled);
	void addDevice(const pa_source_info *l, int eol);

protected:
	virtual bool init();
	virtual void populateDeviceList(QList<SoundManagerDeviceInfo>& devices, int& recommended);
	virtual void updateFft();
	void checkPulse();

private:
	void Uninit();
	void init_buffers();
	void process_fft();
	void populatePulseaudioDeviceList();

	static void *pa_fft_thread(void *arg);
	static void context_state_cb(pa_context *c, void *userdata);
	static void stream_state_cb(pa_stream *s, void *userdata);
	static void stream_read_cb (pa_stream *p, size_t nbytes, void *userdata);
	static void stream_success_cb (pa_stream *p, int success, void *userdata)
	{
		Q_UNUSED(p);
		Q_UNUSED(success);
		Q_UNUSED(userdata);
	}

	QTimer m_timer;
	QTimer m_pa_alive_timer;
	std::mutex m_mutex;
	int m_cont = 0;

	/* Pulse */
	int m_pa_ready = 0;
	pa_threaded_mainloop *m_main_loop = nullptr;
	pa_context *m_context = nullptr;
	pa_stream *m_stream = nullptr;
	char *m_server = nullptr; // TODO: add server selector?
	int m_error;
	pa_sample_spec m_ss;
	int m_buffering = 50;
	QList<QPair<QString, QString>> m_devices; // buffer devices for index-to-name mapping purposes

	/* Buffer */
	std::vector<float> m_pa_buf;
	size_t m_pa_samples_read, m_buffer_samples;

	/* FFT */
	fftwf_complex *m_output = nullptr; //special buffer with proper SIMD alignments
	std::vector<float> m_input;
	fftwf_plan m_plan = nullptr;
	std::vector<float> m_weights;
};
