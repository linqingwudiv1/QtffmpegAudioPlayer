#include "audiothread.h"


#include <QAudioOutput>
#include <QFile>
#include <QDebug>

AudioProgram::AudioProgram(QObject *parent):
    QObject(parent)
{
    QAudioFormat audioFormat;
    audioFormat.setSampleSize(16);
    audioFormat.setChannelCount(2);
    audioFormat.setSampleRate(44100);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setSampleType(QAudioFormat::UnSignedInt);
    if(!QAudioDeviceInfo::defaultOutputDevice().isFormatSupported(audioFormat))
    {
        qDebug()<<"not supported format of audio";
        return ;
    }
    audioOutput = new QAudioOutput(audioFormat);
    audioOutput->setNotifyInterval(600);
    audioOutput->setBufferSize(5120);

    cacheFile = new QFile("cacheFile.raw");
    if(!cacheFile->open(QIODevice::ReadOnly))
    {
        qDebug()<<"cacheFile open faild";
    }

    connect(audioOutput, SIGNAL(stateChanged(QAudio::State)),
            this,SIGNAL(stateChangedSignal(QAudio::State)));

    connect(audioOutput, &QAudioOutput::notify,
            [=]()
    {
        qint64 pos = cacheFile->pos();
        qreal value = pos / 2.0000 / 2.0000 / 44100.0000;
        emit notifySignal(value);
    });
}

QAudio::State AudioProgram::audioState()
{
    return audioOutput->state();
}

void AudioProgram::setVolume(qreal vol)
{
    audioOutput->setVolume(vol);
}

void AudioProgram::cacheFilePosChanged(quint64 pos)
{
    cacheFile->seek(pos);
}

void AudioProgram::suspend()
{
    audioOutput->suspend();
}

void AudioProgram::resume()
{
    audioOutput->resume();
}

void AudioProgram::startPlaySlot()
{
    if(!cacheFile->isOpen()){
        cacheFile->open(QIODevice::ReadOnly);
    }
    cacheFile->reset();
    audioOutput->start(cacheFile);
}

AudioThread::AudioThread(QObject *parent) :
    QThread(parent)
{
    start();
}

QAudio::State AudioThread::audioState()
{
    return audioProgram->audioState();
}

void AudioThread::run()
{
    audioProgram = new AudioProgram();
    connect(audioProgram, SIGNAL(notifySignal(qreal)),
            this, SIGNAL(notifySignal(qreal)));
    connect(audioProgram, SIGNAL(stateChangedSignal(QAudio::State)),
            this, SIGNAL(stateChangedSignal(QAudio::State)));
    connect(this, SIGNAL(cacheFilePosChanged(quint64)),
            audioProgram, SLOT(cacheFilePosChanged(quint64)));
    connect(this, SIGNAL(suspend()), audioProgram, SLOT(suspend()));
    connect(this, SIGNAL(resume()), audioProgram, SLOT(resume()));
    connect(this, SIGNAL(startPlaySignal()), audioProgram, SLOT(startPlaySlot()));
    connect(this, SIGNAL(setVolume(qreal)), audioProgram, SLOT(setVolume(qreal)));
    exec();
}
