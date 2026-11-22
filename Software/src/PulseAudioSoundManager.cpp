/*
 * PulseAudioSoundManager.cpp
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


#include "PulseAudioSoundManager.hpp"
#include "debug.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline void weights_init(float *dest, int samples, enum w_type w)
{
	switch(w) {
		case WINDOW_TRIANGLE:
			for (int i = 0; i < samples; i++)
				dest[i] = 1 - 2*fabsf((i - ((samples - 1)/2.0f))/(samples - 1));
			break;
		case WINDOW_HANNING:
			for (int i = 0; i < samples; i++)
				dest[i] = 0.5f*(1 - cos((2*M_PI*i)/(samples - 1)));
			break;
		case WINDOW_HAMMING:
			for (int i = 0; i < samples; i++)
				dest[i] = 0.54 - 0.46*cos((2*M_PI*i)/(samples - 1));
			break;
		case WINDOW_BLACKMAN:
			for (int i = 0; i < samples; i++) {
				const float c1 = cos((2*M_PI*i)/(samples - 1));
				const float c2 = cos((4*M_PI*i)/(samples - 1));
				dest[i] = 0.42659 - 0.49656*c1 + 0.076849*c2;
			}
			break;
		case WINDOW_BLACKMAN_HARRIS:
			for (int i = 0; i < samples; i++) {
				const float c1 = cos((2*M_PI*i)/(samples - 1));
				const float c2 = cos((4*M_PI*i)/(samples - 1));
				const float c3 = cos((6*M_PI*i)/(samples - 1));
				dest[i] = 0.35875 - 0.48829*c1 + 0.14128*c2 - 0.001168*c3;
			}
			break;
		case WINDOW_FLAT:
			for (int i = 0; i < samples; i++)
				dest[i] = 1.0f;
			break;
		case WINDOW_WELCH:
			for (int i = 0; i < samples; i++)
				dest[i] = 1 - pow((i - ((samples - 1)/2.0f))/((samples - 1)/2.0f), 2.0f);
			break;
		default:
			for (int i = 0; i < samples; i++)
				dest[i] = 0.0f;
			break;
	}
	float sum = 0.0f;
	for (int i = 0; i < samples; i++)
		sum += dest[i];
	for (int i = 0; i < samples; i++)
		dest[i] /= sum;
}

static void pa_context_state_cb(pa_context *c, void *userdata)
{
	pa_context_state_t state;
	int *pa_ready = (int *)userdata;

	state = pa_context_get_state(c);
	switch (state) {
		case PA_CONTEXT_UNCONNECTED:
			*pa_ready = 3;
			break;
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		default:
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			*pa_ready = 2;
			break;
		case PA_CONTEXT_READY:
			*pa_ready = 1;
			break;
	}
}

PulseAudioSoundManager::PulseAudioSoundManager(QObject *parent) : SoundManagerBase(parent)
{
	m_timer.setTimerType(Qt::PreciseTimer);
	connect(&m_timer, &QTimer::timeout, this, &PulseAudioSoundManager::updateColors);

	m_pa_alive_timer.setTimerType(Qt::PreciseTimer);
	connect(&m_pa_alive_timer, &QTimer::timeout, this, &PulseAudioSoundManager::checkPulse);
}

PulseAudioSoundManager::~PulseAudioSoundManager()
{
	m_cont = 0;
	Uninit();
}

void PulseAudioSoundManager::Uninit()
{
	std::lock_guard<std::mutex> l(m_mutex);

	if (m_stream) {
		pa_stream_disconnect(m_stream);
		pa_stream_unref(m_stream);
		m_stream = nullptr;
	}
	if (m_context) {
		pa_context_disconnect(m_context);
		pa_context_unref(m_context);
		m_context = nullptr;
	}
	if (m_main_loop) {
		pa_threaded_mainloop_stop(m_main_loop);
		pa_threaded_mainloop_free(m_main_loop);
		m_main_loop = nullptr;
	}

	if (m_plan) {
		fftwf_destroy_plan(m_plan);
		m_plan = nullptr;
	}

	if (m_output) {
		fftwf_free(m_output);
		m_output = nullptr;
	}

	m_isInited = false;
}

bool PulseAudioSoundManager::init() {
	int ret = 0;
	m_buffer_samples = fftSize() * 2;
	m_cont = 1;
	m_ss.format = PA_SAMPLE_FLOAT32LE;
	m_ss.rate = 44100;
	m_ss.channels = 1;

	init_buffers();

	m_main_loop = pa_threaded_mainloop_new();
	m_context = pa_context_new(pa_threaded_mainloop_get_api(m_main_loop), "Prismatik");
	pa_context_set_state_callback(m_context, context_state_cb, this);

	// Lock the mainloop so that it does not run and crash before the context is ready
	pa_threaded_mainloop_lock(m_main_loop);
	pa_threaded_mainloop_start(m_main_loop);

	ret = pa_context_connect(m_context, m_server, PA_CONTEXT_NOFLAGS, NULL);

	qInfo() << "pa_context_connect:" << pa_strerror(ret);
	if (ret != PA_OK)
		goto unlock_and_fail;

	// wait for pa_context_state_cb
	for(;;)
	{
		if(m_pa_ready == 1) break;
		if(m_pa_ready == 2 || !m_cont) goto unlock_and_fail;
		pa_threaded_mainloop_wait(m_main_loop);
	}

	pa_threaded_mainloop_unlock(m_main_loop);

	populatePulseaudioDeviceList();

	m_isInited = true;
	return true;

unlock_and_fail:
	m_pa_ready = 2;
	pa_threaded_mainloop_unlock(m_main_loop);

	Uninit();
	return false;
}

static void pa_devicelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata)
{
	Q_UNUSED(c);
	auto pa = static_cast<PulseAudioSoundManager*>(userdata);
	pa->addDevice(l, eol);
}

void PulseAudioSoundManager::addDevice(const pa_source_info *l, int eol)
{
	if (eol > 0) {
		pa_threaded_mainloop_signal(m_main_loop, 0);
		return;
	}
	if (l->monitor_of_sink != PA_INVALID_INDEX)
		m_devices.push_back({l->description, l->name});
}

void PulseAudioSoundManager::populatePulseaudioDeviceList()
{
	m_devices.clear();
	m_devices.push_back({QStringLiteral("Default device"), QLatin1String("")});

	pa_operation *pa_op;
	pa_op = pa_context_get_source_info_list(m_context,
		pa_devicelist_cb, this);

	for (;;) {
		switch (pa_operation_get_state(pa_op)) {
		case PA_OPERATION_RUNNING:
			pa_threaded_mainloop_wait(m_main_loop);
			continue;
		case PA_OPERATION_CANCELLED:
		case PA_OPERATION_DONE:
		default:
			pa_operation_unref(pa_op);
			return;
		}
	}
}

void PulseAudioSoundManager::populateDeviceList(QList<SoundManagerDeviceInfo>& devices, int& recommended)
{
	populatePulseaudioDeviceList();
	recommended = 0;
	for (int i = 0; i < m_devices.size(); i++) {
		devices.append(SoundManagerDeviceInfo(m_devices[i].first, i));
	}
}

void PulseAudioSoundManager::stream_state_cb(pa_stream *s, void *userdata)
{
	Q_UNUSED(s);
	PulseAudioSoundManager *pa = reinterpret_cast<PulseAudioSoundManager *>(userdata);
	pa_threaded_mainloop_signal(pa->m_main_loop, 0);
}

void PulseAudioSoundManager::context_state_cb(pa_context *c, void *userdata)
{
	PulseAudioSoundManager *pa = reinterpret_cast<PulseAudioSoundManager *>(userdata);

	pa_context_state_cb(c, &pa->m_pa_ready);
	pa_threaded_mainloop_signal(pa->m_main_loop, 0);
}

void PulseAudioSoundManager::start(bool isEnabled)
{
	int ret;

	if (m_isEnabled == isEnabled)
		return;

	m_isEnabled = isEnabled;

	if (m_isEnabled)
	{
		if (!m_isInited)
		{
			if (!init()) {
				m_isEnabled = false;
				return;
			}
		}

		if (!m_stream)
		{
			pa_threaded_mainloop_lock(m_main_loop);
			m_stream = pa_stream_new(m_context, "Prismatik", &m_ss, nullptr);

			if (!m_stream)
				goto unlock_and_fail;

			pa_stream_set_state_callback(m_stream, stream_state_cb, this);

			// Sets individual read callback fragsize but recording itself
			// still "lags" ~1sec (read_cb is called in bursts) without
			// PA_STREAM_ADJUST_LATENCY
			pa_buffer_attr buffer_attr;
			buffer_attr.maxlength = (uint32_t) -1;
			buffer_attr.tlength = (uint32_t) -1;
			buffer_attr.prebuf = (uint32_t) -1;
			buffer_attr.minreq = (uint32_t) -1;
			buffer_attr.fragsize = pa_usec_to_bytes(m_buffering * 1000, &m_ss);

			pa_stream_set_read_callback(m_stream, stream_read_cb, this);

			QString dev;
			if (m_device > 0 && m_device < m_devices.size()) {
				dev = m_devices[m_device].second;
				qInfo() << "Pulseaudio device:" << m_devices[m_device].first;
			} else {
				qInfo() << "Pulseaudio device: Default device";
			}

			ret = pa_stream_connect_record(m_stream, !dev.isEmpty() ? dev.toUtf8().constData() : nullptr, &buffer_attr, PA_STREAM_ADJUST_LATENCY);

			if (ret != PA_OK) {
				qCritical() << "Pulseaudio failed to connect to device" << (!dev.isEmpty() ? dev : QStringLiteral("Default device"));
				goto unlock_and_fail;
			}

			// Wait for the stream to be ready
			for(;;) {
				pa_stream_state_t stream_state = pa_stream_get_state(m_stream);
				assert(PA_STREAM_IS_GOOD(stream_state));
				if (stream_state == PA_STREAM_READY) break;
				if (stream_state == PA_STREAM_FAILED) {
					qCritical() << "Pulseaudio stream failed to connect to device" << dev;
					goto unlock_and_fail;
				}
				pa_threaded_mainloop_wait(m_main_loop);
			}

			pa_threaded_mainloop_unlock(m_main_loop);
		}

		// Usually stream is already uncorked but check anyway
		if (m_stream)
		{
			pa_threaded_mainloop_lock(m_main_loop);
			if (pa_stream_is_corked(m_stream) > 0)
			{
				pa_operation *op = pa_stream_cork(m_stream, 0, stream_success_cb, this);
				if (op)
					pa_operation_unref(op);
			}
			pa_threaded_mainloop_unlock(m_main_loop);
		}

		// setup update timer (40hz)
		using namespace std::chrono_literals;
		m_timer.start(25ms);
		m_pa_alive_timer.start(1s);
	}
	else
	{
		m_timer.stop();
		m_pa_alive_timer.stop();
		if (m_stream) {
			ret = pa_stream_disconnect(m_stream);
			pa_stream_unref(m_stream);
			m_stream = nullptr;
		}
	}

	if (m_visualizer == nullptr)
		return;
	if (m_isEnabled)
		m_visualizer->start();
	else
		m_visualizer->stop();

	return;

unlock_and_fail:
	if (m_stream) {
		ret = pa_stream_disconnect(m_stream);
		pa_stream_unref(m_stream);
		m_stream = nullptr;
	}
	pa_threaded_mainloop_unlock(m_main_loop);

}

void PulseAudioSoundManager::updateFft()
{
}

void PulseAudioSoundManager::checkPulse()
{
	// pulseaudio disconnected, retry
	if (m_pa_ready > 1) {
		if (m_context) {
			int ret = pa_context_connect(m_context, m_server, PA_CONTEXT_NOFLAGS, NULL);
			if (ret != PA_OK) { // Most likely got PA_ERR_BADSTATE so restart the whole thing
				qInfo() << "Pulseaudio reconnect failed:" << ret << pa_strerror(ret);
				Uninit();
			}
		}

		if (!m_isInited) {
			m_pa_ready = 0;
			start(!m_isEnabled);
			start(!m_isEnabled);
		}
	}
}

void PulseAudioSoundManager::init_buffers()
{
	// just in case, free old stuff
	if (m_plan) {
		fftwf_destroy_plan(m_plan);
		m_plan = nullptr;
	}

	if (m_output) {
		fftwf_free(m_output);
		m_output = nullptr;
	}

	/* Pulse buffer */
	m_pa_samples_read = 0;
	m_pa_buf.resize(m_buffer_samples * m_ss.channels);

	/* FFTW buffer */
	m_output = (fftwf_complex*) fftwf_alloc_complex(fftSize() + 1);
	m_input.resize(m_buffer_samples);
	m_plan = fftwf_plan_dft_r2c_1d(m_input.size(), m_input.data(), m_output, 0);

	m_weights.resize(fftSize() + 1);
	weights_init(m_weights.data(), m_weights.size(), WINDOW_HANNING); // TODO: BASS does hanning?
}

void PulseAudioSoundManager::process_fft()
{
	for (size_t i = 0; i < m_pa_buf.size(); i += m_ss.channels /* Mono anyway but... */) {
		m_input[i] = m_pa_buf[i];
	}

	fftwf_execute(m_plan);

	for (size_t i = 0; i < fftSize(); i++ ) {
		m_fft[i] = sqrtf( pow(m_output[i][0], 2) + pow(m_output[i][1], 2) ) * m_weights[i];
	}
}

void PulseAudioSoundManager::stream_read_cb (pa_stream *p, size_t nbytes, void *userdata)
{
	constexpr size_t sizeof_sample = sizeof(decltype(PulseAudioSoundManager::m_pa_buf)::value_type); // int16_t or float or...
	PulseAudioSoundManager *pa = reinterpret_cast<PulseAudioSoundManager *> (userdata);
	const void* padata = nullptr;

	if (!pa->m_cont)
		return;

	std::lock_guard<std::mutex> lock(pa->m_mutex);

	int ret = pa_stream_peek(p, &padata, &nbytes);

	if (ret != PA_OK)
		return;

	if ((!padata && nbytes /* hole */)) {
		ret = pa_stream_drop(p);
		return;
	}

	// read data until buffer is full
	size_t n_read_bytes = std::min(nbytes, (pa->m_buffer_samples - pa->m_pa_samples_read) * sizeof_sample);
	if (n_read_bytes > 0)
		memcpy(&pa->m_pa_buf[pa->m_pa_samples_read], padata, n_read_bytes);

	pa->m_pa_samples_read += nbytes / sizeof_sample;

	// if buffer is full, process FFT and read rest of the data if any
	if (pa->m_pa_samples_read >= pa->m_buffer_samples)
	{
		pa->process_fft();
		pa->m_pa_samples_read = 0;
		memcpy(&pa->m_pa_buf[0], (uint8_t *)padata + n_read_bytes, std::min(nbytes - n_read_bytes, pa->m_buffer_samples * sizeof_sample));
	}

	//if copy succeeded, drop samples at pulse's side
	ret = pa_stream_drop(p);
	if (ret != PA_OK)
		qCritical() << Q_FUNC_INFO << "pa_stream_drop:" << pa_strerror(ret);
}
