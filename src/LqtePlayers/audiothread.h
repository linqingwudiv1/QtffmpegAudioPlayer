#ifndef AUDIOTHREAD_H
#define AUDIOTHREAD_H

#include <QThread>
#include <QAudio>
class QFile;
class QAudioOutput;
class AudioProgram : public QObject{
    Q_OBJECT
public:
    AudioProgram(QObject *parent = 0);
    QAudio::State audioState();
signals:
    void notifySignal(qreal value);
    void stateChangedSignal(QAudio::State state);
public slots:
    void cacheFilePosChanged(quint64 pos);
    void setVolume(qreal vol);
    void suspend();
    void resume();
    void startPlaySlot();
private:
    QFile *cacheFile ;
    QAudioOutput *audioOutput;
};
class AudioThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioThread(QObject *parent = 0);
    void run();
    QAudio::State audioState();
signals:
    void cacheFilePosChanged(quint64 pos);
    void stateChangedSignal(QAudio::State state);
    void notifySignal(qreal value);
    void startPlaySignal();
    void setVolume(qreal vol);
    void suspend();
    void resume();
public slots:
private:
    AudioProgram *audioProgram;
};

#endif // AUDIOTHREAD_H
