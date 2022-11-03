#ifndef AUDIO_FILE_WRITER_H
#define AUDIO_FILE_WRITER_H

#include <QtCore>

class AudioFileWriter {
public:
	static bool convertSamplesFromNativeEndian(const qint16 *sourceBuffer, qint16 *targetBuffer, uint sampleCount, QSysInfo::Endian targetByteOrder);

	AudioFileWriter(uint sampleRate, const QString &fileName);
	virtual ~AudioFileWriter();

	bool open(bool skipInitialSilence = true);
	bool write(const qint16 *buffer, uint framesToWrite);
	void close();

private:
	const uint sampleRate;
	const QString fileName;
	const bool waveMode;
	QFile file;
	bool skipSilence;
};

class MidiParser;
class QSynth;

class AudioFileRenderer : public QThread {
	Q_OBJECT

public:
	AudioFileRenderer();
	~AudioFileRenderer();

	bool convertMIDIFiles(QString useOutFileName, QStringList useMIDIFileNameList, QString synthProfileName, quint32 bufferSize = 65536);
	void startRealtimeProcessing(QSynth *useSynth, quint32 useSampleRate, QString useOutFileName, quint32 bufferSize);
	void stop();

protected:
	void run();

private:
	QSynth *synth;
	uint sampleRate;
	QString outFileName;
	unsigned int bufferSize;
	qint16 *buffer;
	MidiParser *parsers;
	uint parsersCount;
	bool realtimeMode;
	volatile bool stopProcessing;

signals:
	void parsingFailed(const QString &, const QString &);
	void midiEventProcessed(int midiEventsProcessed, int midiEventsTotal);
	void conversionFinished();
};

#endif
