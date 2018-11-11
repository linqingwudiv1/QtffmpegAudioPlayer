#ifndef AUDIODECODEMGR_H
#define AUDIODECODEMGR_H

#include <QObject>
#include <QFile>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVCodec;
struct AVPacket;
struct AVFrame;
struct SwrContext;
class AudioDecodeMgr;
class CustomModel;

class AudioDecodeMgr : public QObject
{
    Q_OBJECT
public:
    explicit AudioDecodeMgr(CustomModel **model =nullptr, QObject *parent = 0);
    int freeHandle(bool isSuccess=false);
    ~AudioDecodeMgr();
signals:
    void startPlay(int sampleRate);
    void decodeAudioSignal(QString musicPath);
    void finishSignal();
public slots:
    int decodeAudio(QString path,bool isGetInfoOnly = false);
private:
    AVFormatContext *fmt_Ctx;
    AVCodecContext *dec_Ctx;
    AVCodec *dec;
    AVPacket *pkt;
    AVFrame  *frame;
    SwrContext *swr_Ctx;
    AVStream *st;
    int stream_Index_Audio;
    int stream_Index_Video;
    quint8 **out_buf;
    int out_LineSize;
    int out_Channels;
    int out_Sample_Size;
    int out_Sample_Max_Size;
    int ret;
    bool convert_Spe_Fmt;
    CustomModel **tableModel;

    quint64 channel_Out;
    int     sample_Rate_Out;
};

#endif // AUDIODECODEMGR_H
