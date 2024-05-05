/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "QtAudioDriver.h"

#ifdef USE_QT_MULTIMEDIAKIT
#include <QtMultimediaKit/QAudioOutput>
#elif (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QtMultimedia/QAudioOutput>
#else
#include <QtMultimedia/QAudioSink>
#endif

#include <mt32emu/mt32emu.h>

#include "../Master.h"
#include "../MasterClock.h"
#include "../SynthRoute.h"

using namespace MT32Emu;

class WaveGenerator : public QIODevice {
private:
	QtAudioStream &stream;

public:
	WaveGenerator(QtAudioStream &useStream) : stream(useStream) {
		open(QIODevice::ReadOnly | QIODevice::Unbuffered);
	}

	qint64 readData(char *data, qint64 len) {
		if (len == (stream.audioLatencyFrames << 2)) {
			// Fill empty buffer to keep correct timing at startup / x-run recovery
			memset(data, 0, len);
			return len;
		}
		MasterClockNanos nanosNow = MasterClock::getClockNanos();
		quint32 framesInAudioBuffer;
		if (stream.settings.advancedTiming) {
			framesInAudioBuffer = (stream.audioOutput->bufferSize() - stream.audioOutput->bytesFree()) >> 2;
		} else {
			framesInAudioBuffer = 0;
		}
		uint framesToRender = uint(len >> 2);
		stream.renderAndUpdateState((Bit16s *)data, framesToRender, nanosNow, framesInAudioBuffer);
		return len;
	}

	qint64 writeData(const char *data, qint64 len) {
		Q_UNUSED(data);
		Q_UNUSED(len);
		return 0;
	}
};

class ProcessingThread : public QThread {
private:
	QtAudioStream &stream;

public:
	ProcessingThread(QtAudioStream &useStream) : stream(useStream) {}

	void run() {
		stream.start();
		exec();
		stream.close();
	}
};

QtAudioStream::QtAudioStream(const AudioDriverSettings &useSettings, SynthRoute &useSynthRoute, const quint32 useSampleRate) :
	AudioStream(useSettings, useSynthRoute, useSampleRate)
{
	// Creating QAudioOutput in a thread leads to smooth rendering
	// Rendering will be performed in the main thread otherwise
	processingThread = new ProcessingThread(*this);
	processingThread->start(QThread::TimeCriticalPriority);
}

QtAudioStream::~QtAudioStream() {
	qDebug() << "QAudioDriver: Stopping processing thread";
	processingThread->exit();
	processingThread->wait();
	qDebug() << "QAudioDriver: Processing thread stopped";
	delete processingThread;
}

void QtAudioStream::start() {
	QAudioFormat format;
	format.setSampleRate(sampleRate);
	format.setChannelCount(2);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	// libmt32emu produces samples in native byte order which is the default byte order used in QAudioFormat
	format.setSampleType(QAudioFormat::SignedInt);
	audioOutput = new QAudioOutput(format);
#else
	format.setSampleFormat(QAudioFormat::Int16);
	audioOutput = new QAudioSink(format);
#endif
	waveGenerator = new WaveGenerator(*this);
	if (settings.audioLatency != 0) {
		audioOutput->setBufferSize((sampleRate * settings.audioLatency << 2) / MasterClock::MILLIS_PER_SECOND);
	}
	audioOutput->start(waveGenerator);
	MasterClockNanos audioLatency = MasterClock::NANOS_PER_SECOND * audioOutput->bufferSize() / (sampleRate << 2);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	MasterClockNanos chunkSize = MasterClock::NANOS_PER_SECOND * audioOutput->periodSize() / (sampleRate << 2);
#else
	MasterClockNanos chunkSize = audioLatency / 5;
#endif
	audioLatencyFrames = quint32(audioOutput->bufferSize() >> 2);
	qDebug() << "QAudioDriver: Latency set to:" << (double)audioLatency / MasterClock::NANOS_PER_SECOND << "sec." << "Chunk size:" << (double)chunkSize / MasterClock::NANOS_PER_SECOND;

	// Setup initial MIDI latency
	if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames;
	qDebug() << "QAudioDriver: MIDI latency set to:" << (double)midiLatencyFrames / sampleRate << "sec";
}

void QtAudioStream::close() {
	audioOutput->stop();
	delete audioOutput;
	delete waveGenerator;
}

QtAudioDefaultDevice::QtAudioDefaultDevice(QtAudioDriver &driver) : AudioDevice(driver, "Default") {}

AudioStream *QtAudioDefaultDevice::startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const {
	return new QtAudioStream(driver.getAudioSettings(), synthRoute, sampleRate);
}

QtAudioDriver::QtAudioDriver(Master *useMaster) : AudioDriver("qtaudio", "QtAudio") {
	Q_UNUSED(useMaster);

	loadAudioSettings();
}

const QList<const AudioDevice *> QtAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new QtAudioDefaultDevice(*this));
	return deviceList;
}

void QtAudioDriver::validateAudioSettings(AudioDriverSettings &settings) const {
	settings.chunkLen = settings.audioLatency / 5;
	if ((settings.midiLatency != 0) && (settings.midiLatency < settings.chunkLen)) settings.midiLatency = settings.chunkLen;
}
