#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFrame>
#include <QAudioFormat>
#include <QDir>
#include <QAudio>
#include <QThread>
#include "ui_mainwindow.h"
#include "ui_titleBar.h"
#include "ui_viewCenter.h"
#include "ui_listgroup.h"
#include "ui_miniPlay.h"


class QFileDialog;
class QAudioOutput;
class QSystemTrayIcon;
class AudioThread;
class QVBoxLayout;
class QStackedWidget;
class MyThread : public QThread{
    Q_OBJECT
public:
    MyThread(CustomModel **mdl,QObject *parent = 0);
    void run();
signals:
    void startPlay();
    void decodeAudio(QString musicPath);
    void decodeFinished();
private:
    CustomModel **model;
};

class AddDirThread : public QThread{
    Q_OBJECT
public:
    void setDir(QDir dir, bool isSubDir);
    void run();
signals:
    void updataTableData();
private:
    void addMusicDir(QDir dir, bool isSubDir);
    QDir dir;
    bool isSubDir;
};

class MainWindow : public QFrame
{
    Q_OBJECT
    enum StretchDirection{
        NoStretch,LeftStretch,RightStretch,UpStretch,DownStretch
    };
    enum WindowOperationState{NoState,StretchState,MoveingState};
    enum PlayMode{Sequence,Repeat,Shuffle};
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public:
    void initilze();
    void createWidget();
    void createLayout();
    void connection();
    void loadSetting();
    void saveSetting();
signals:
    void suspend();
    void resume();
    void setVolume(qreal vol);
    void startPlaySignal();
    void decodeAudioSignal(QString musicPath);
    void cacheFilePosChanged(quint64 pos);
public slots:
    void notifySlot(qreal value);
    void stateChange(QAudio::State state);
    void modelSwitch();
    void BFMusic();
    void playMusic(QModelIndex index = QModelIndex(), bool fromFForBF = false);
    void FFMusic();
    void addDir();
    void addFiles();
    void groupList(bool isCheck);
    void updateTableData();
    void newGroupList();
    void editGroupList();
    void delGroupList();
    void volumeChange(int volume);
    void playModelChange(QAbstractButton *btn,bool checked);
    void startPlay();
    void closeHandle();
    void hideCenterView();
    void continuous();
private:
    QLineEdit *setItemToEditState(QListWidgetItem *item);
    void getCorrectTableName(QString &tn);
    void connectModel(CustomModel *snapModel);
    void setPlayDivInfo();
    void parseLrcFile(QString path);
protected:
    void closeEvent(QCloseEvent *ev);
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void moveEvent(QMoveEvent *ev);
    bool eventFilter(QObject *obj, QEvent *ev);
private:
    int xOffset;
    int yOffset;
    StretchDirection stretchDire;
    WindowOperationState state;
    PlayMode playMode;
    Ui::TitleBar   ui_TitleBar;
    Ui::MainWindow ui_MainWindow;
    Ui::ViewCenter ui_ViewCenter;
    Ui::ListGroup  ui_ListGroup;
    Ui::MiniPlay   ui_MiniPlay;
    QWidget *titleBar;
    QWidget *playWgt;
    QWidget *playMiniWgt;
    QWidget *centerWgt;
    QFrame  *groupListPop;
    QSlider *slider;
    QSlider *soundSlider;
    QFileDialog *fileDia;
    QAction *addDirAct;
    QAction *addFilesAct;
    CustomModel *tableModel;
    CustomModel *playTableModel;

    QButtonGroup *btnGroup;
    QListWidget *list;
    TableView  *tableView;

    int currentRow;
    int menuActAt;
    bool isStretchable;
    bool isVisiable;
    MyThread *threadA;
    AddDirThread addDirThread;
    QMovie *movie ;
    QSystemTrayIcon *sysIcon;
    QSize snapSize;
    QAudioOutput *audioOutput;
    AudioThread *audioThread;
    QVBoxLayout *mainLayout;
};

#endif // MAINWINDOW_H
