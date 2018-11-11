#include "audiodecodemgr.h"
#include "mainwindow.h"
extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libavutil/dict.h"
}

#include <qdebug.h>
#include <QFileInfo>
#include <QTime>
#define DEFAULT_NB_SAMPLE 2048

AudioDecodeMgr::AudioDecodeMgr(CustomModel **model, QObject *parent):
    QObject(parent)
{
    tableModel = model;
    fmt_Ctx = NULL;
    dec_Ctx = NULL;
    dec = NULL;
    pkt = NULL;
    frame = NULL;
    swr_Ctx = NULL;
    out_buf = NULL;
    convert_Spe_Fmt = false;
    av_register_all();
    channel_Out = AV_CH_LAYOUT_STEREO;
    sample_Rate_Out = 44100;

    connect(this, SIGNAL(decodeAudioSignal(QString)),
            this ,SLOT(decodeAudio(QString)));
}

int AudioDecodeMgr::decodeAudio(QString path, bool isGetInfoOnly){
    fmt_Ctx = avformat_alloc_context();
    if(avformat_open_input(&fmt_Ctx, path.toUtf8().data(), NULL, NULL) < 0){
        qDebug()<<"open_input";
        return freeHandle();
    }
    if(avformat_find_stream_info(fmt_Ctx,NULL)<0){
        qDebug()<<"find_stream_info failed;";
        return freeHandle();
    }
    if(isGetInfoOnly){
        qDebug()<<"isGetInfoOnly";
        QString str[11];// 0title,1album,2artist,3albumArtist,4fileName,5format,6picPath,70+[1or3],8time;
                        // 9 duration 10bitrate
        int duration = fmt_Ctx->duration / 1000;
        AVDictionaryEntry *tag = NULL;
        char *keyOrder[4] = {"title","album","artist","album_artist"};
        for(int i = 0;i < 4;i++){
            while(tag = av_dict_get(fmt_Ctx->metadata,keyOrder[i], tag, AV_DICT_IGNORE_SUFFIX)){
                str[i] = tag->value;
                //qDebug()<<str[i]<<i;
                if(!str[i].isEmpty())
                    break;
            }
        }
        str[4] = path;
        QFileInfo fileInfo(path);
        str[5] = fileInfo.completeSuffix();
        if(str[0].isEmpty())
            str[0] = fileInfo.baseName();
        QTime t(0,0);
        t = t.addMSecs(duration);
        str[8] = t.toString("mm:ss");
        str[9] = QString::number(duration);
        str[10] =QString::number( fmt_Ctx->bit_rate);

        stream_Index_Video = av_find_best_stream(fmt_Ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if(stream_Index_Video < 0){
            qDebug("error of streamindex Video");
        }
        else{
        st = fmt_Ctx->streams[stream_Index_Video];
        if(st->attached_pic.size > 0){
                QFile file;
                str[6] = "cacheFile/attPic/" + fileInfo.baseName() + "-" + str[2] + ".jpg";
                file.setFileName(str[6]);
                if(file.exists() || !file.open(QIODevice::WriteOnly)){
                   qDebug()<<"open cachefile failed or file is exsits";
                }
                else{
                file.write((char *)st->attached_pic.data, st->attached_pic.size);
                file.close();
                }
            }
        }
        int ir = (str[2].isEmpty() ? (str[3].isEmpty() ? 1 : 3) : 2);
        if(!str[ir].isEmpty())
            str[7] = str[0] + "-" + str[ir];
        else
            str[7] = str[0] + "." + str[5];
        qDebug()<<"str[7]:"<<str[7];
        QList<QStandardItem *> itemList;
        if(str[6].isEmpty())
            str[6] = ":/img/img/collect.png";
        for(int ig = 0;ig < 11;ig++){
            QStandardItem *item ;
            if(ig == 7){
                QIcon *icon = TableView::iconMap.value(str[6], nullptr);
                if(!icon){
                    icon = new QIcon(str[6]);
                    TableView::iconMap.insert(str[6],icon);
                }
                item = new QStandardItem(*icon, str[ig]);
            }
            else
                item =new QStandardItem(str[ig]);
            itemList<<item;
        }
        (*tableModel)->appendRow(itemList);
        return freeHandle(true);
    }
    stream_Index_Audio = av_find_best_stream(fmt_Ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(stream_Index_Audio < 0){
        qDebug("error of streamindex audio");
        return freeHandle();
    }
    st = fmt_Ctx->streams[stream_Index_Audio];
    dec_Ctx = st->codec;
    dec = avcodec_find_decoder(dec_Ctx->codec_id);
    if(!dec){
        qDebug()<<"not codec ~~~";
        return freeHandle();
    }
    if(avcodec_open2(dec_Ctx,dec, NULL) < 0){
        qDebug()<<"open avcodec faild";
        return freeHandle();
    }
    pkt=new AVPacket();
    av_init_packet(pkt);
    frame = av_frame_alloc();
    if(dec_Ctx->sample_fmt != AV_SAMPLE_FMT_S16){
        swr_Ctx = swr_alloc_set_opts(NULL, channel_Out,
                                     AV_SAMPLE_FMT_S16, sample_Rate_Out,
                                     dec_Ctx->channel_layout, dec_Ctx->sample_fmt,
                                     dec_Ctx->sample_rate, 0, NULL);
        if(swr_init(swr_Ctx) < 0){
            qDebug()<<"swr_init faild";
            return freeHandle();
        }
        convert_Spe_Fmt = true;
        out_Channels = av_get_channel_layout_nb_channels(dec_Ctx->channel_layout);
        out_Sample_Max_Size = out_Sample_Size = av_rescale_rnd(DEFAULT_NB_SAMPLE,
                                                             sample_Rate_Out,
                                                             dec_Ctx->sample_rate,
                                                             AV_ROUND_UP);
        ret = av_samples_alloc_array_and_samples(&out_buf, &out_LineSize, out_Channels,
                                               DEFAULT_NB_SAMPLE, AV_SAMPLE_FMT_S16, 1);
        if(ret < 0){
            qDebug()<<"default space allocate faild";
        }
    }
    else{
        convert_Spe_Fmt = false;
    }

    int go_Frame = 0;
    int msleep = 0;
    bool sendStartPlay = false;

    QFile cacheFile;
    cacheFile.setFileName("cacheFile.raw");
    if(!cacheFile.open(QIODevice::WriteOnly)){
        qDebug()<<"cache file open failed~";
        return freeHandle();
    }
    while(av_read_frame(fmt_Ctx,pkt) >= 0){
        if(pkt->stream_index == stream_Index_Audio){
            do{
                int length = avcodec_decode_audio4(dec_Ctx,frame, &go_Frame,pkt);
                if(length < 0){
                    qDebug()<<"length <0 .skip this frame";
                    break;
                }
                if(go_Frame){
                    if(convert_Spe_Fmt){
                       out_Sample_Size = av_rescale_rnd(
                                   swr_get_delay(swr_Ctx, dec_Ctx->sample_rate) + frame->nb_samples,
                                                 sample_Rate_Out, frame->sample_rate, AV_ROUND_UP);
                       if(out_Sample_Size > out_Sample_Max_Size){
                           av_freep(&out_buf[0]);
                           ret = av_samples_alloc(out_buf, &out_LineSize, out_Channels,
                                            out_Sample_Size, AV_SAMPLE_FMT_S16, 1);
                           if(ret < 0){
                               qDebug()<<"falid allocate";
                               return freeHandle();
                           }
                           out_Sample_Max_Size = out_Sample_Size;
                       }
                       ret = swr_convert(swr_Ctx,out_buf,out_Sample_Size,
                                       (const uint8_t **)frame->data, frame->nb_samples);
                       if (ret < 0) {
                           qDebug("Error while converting \n");
                           return freeHandle();
                       }
                       int buf_size = av_samples_get_buffer_size(&out_LineSize, out_Channels,
                                                                  out_Sample_Size, AV_SAMPLE_FMT_S16,1);

                       if (buf_size < 0) {
                           qDebug("Could not get sample buffer size \n");
                           return freeHandle();
                       }
                       cacheFile.write((char *)out_buf[0], buf_size);
                    }
                    else{
                       cacheFile.write((char *)frame->data[0], frame->linesize[0]);
                    }
                    pkt->data += length;
                    pkt->size -= length;
                }
                av_free_packet(pkt);
                if(!sendStartPlay){
                    qDebug()<<"emit signal~";
                    emit startPlay(dec_Ctx->sample_rate);
                    sendStartPlay = true;
                }
            }while(pkt->size > 0);
        }
    }
    cacheFile.close();
    delete pkt;
    qDebug()<<"finish";
    return freeHandle(true);
}

int AudioDecodeMgr::freeHandle(bool isSuccess){
    if(dec_Ctx &&avcodec_is_open(dec_Ctx)){
        avcodec_close(dec_Ctx);
        dec_Ctx = NULL;
    }
    if(frame){
        av_frame_free(&frame);
    }
    if(swr_Ctx){
        swr_close(swr_Ctx);
        swr_free(&swr_Ctx);
    }
    if(out_buf && convert_Spe_Fmt){
        av_free(out_buf[0]);
        out_buf = NULL;
    }
    if(fmt_Ctx){
        avformat_close_input(&fmt_Ctx);
        avformat_free_context(fmt_Ctx);
    }
    emit finishSignal();
    if(isSuccess)
        return 0;
    else
        return -1;
}

AudioDecodeMgr::~AudioDecodeMgr(){

}
