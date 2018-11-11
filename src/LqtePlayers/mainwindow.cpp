//!* ***********************************************
//!* LqtePlayer: Ching Lin of 2014.10.15-2014.11.10 *
//!* ***********************************************
#include "mainwindow.h"
#include "audiodecodemgr.h"
#include "audiothread.h"

#include <QVBoxLayout>
#include <QMouseEvent>
#include <QMenu>

#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QWidgetAction>
#include <QSettings>
#include <QAudioOutput>
#include <QScrollBar>
#include <QMovie>
#include <QSystemTrayIcon>

#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <QTimer>

// 0 title,1 album,2 artist,3 albumArtist,4 fileName,5 format,
// 6 picPath,7 0+[1or3],8 time,9 duration,10 bitrate
AudioDecodeMgr *mgr;
QString snapStr;
QString musicPath;
QStringList filters;
QString filter;
QMap<CustomModel *, QFile *> tableFileMap;
int counters = 0;
QTimer singleShot;
MyThread::MyThread(CustomModel **mdl, QObject *parent):
    model(mdl),QThread(parent)
{
    start();
}

void MyThread::run()
{
    mgr = new AudioDecodeMgr(model);
    connect(mgr, SIGNAL(startPlay(int)),
            this ,SIGNAL(startPlay()));
    connect(this, SIGNAL(decodeAudio(QString)),
            mgr, SIGNAL(decodeAudioSignal(QString)));
    connect(mgr, SIGNAL(finishSignal()),
            this, SIGNAL(decodeFinished()));
    exec();
}

//AddDirThread
void AddDirThread::setDir(QDir dir, bool isSubDir){
    if(isRunning())
        return;
    this->dir = dir;
    this->isSubDir = isSubDir;
    start(QThread::LowPriority);
}

void AddDirThread::addMusicDir(QDir dir, bool isSubDir){
    QStringList musicFilesName = dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);
    qDebug()<<musicFilesName<<dir.path();
    foreach (QString musicName, musicFilesName) {
        QString musicPath = dir.path() + "/"  + musicName;
        mgr->decodeAudio(musicPath, true);
    }
    emit updataTableData();
    if(isSubDir){
        QStringList subDirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        foreach (QString subDirPath, subDirs) {
            QDir subDir(dir.path() + "/" + subDirPath);
            addMusicDir(subDir, isSubDir);
        }
    }
}

void AddDirThread::run(){
    addMusicDir(dir, isSubDir);
}

//MainWindow
MainWindow::MainWindow(QWidget *parent) :
    QFrame(parent),xOffset(-1),yOffset(-1),stretchDire(NoStretch),state(NoState)
{
    setObjectName("MainWindow");
    currentRow = -1;
    menuActAt = -1;
    playMode = Sequence;
    isStretchable = true;
    isVisiable = true;
    initilze();
    createWidget();
    createLayout();
    connection();
    loadSetting();
}

void MainWindow::initilze()
{
    movie = new QMovie(":/img/img/play.gif");
    singleShot.setSingleShot(true);
    audioThread = new AudioThread();
    threadA = new MyThread(&tableModel);
    setFrameShape(QFrame::Panel);
    setFrameShadow(QFrame::Plain);
    setLineWidth(1);
    setMouseTracking(true);
    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowTitleHint |
                   Qt::WindowMinimizeButtonHint);
    filters <<"*.mp3"<<"*.acc"<<"*.ogg"<<"*.m4a"<<"*.flac"<<"*.ape"<<"*.wav"<<"*.wma"<<"*.amr"<<"*.mid";
    filter = "Audio(*.mp3 *.acc *.ogg *.m4a *.flac *.ape *.wav *.wma *.amr *.mid)";
    btnGroup = new QButtonGroup(this);
    sysIcon = new QSystemTrayIcon(QIcon(":/img/img/contacts.png"));
    QMenu *snapMenu = new QMenu();
    snapMenu->setMinimumWidth(185);
    snapMenu->addAction(QIcon(":/img/img/play.png"),"play", this, SLOT(playMusic()));
    snapMenu->addAction(QIcon(":/img/img/ff.png"), "front", this, SLOT(FFMusic()));
    snapMenu->addAction(QIcon(":/img/img/bf.png"),"back", this, SLOT(BFMusic()));
    snapMenu->addAction(QIcon(":/img/img/shutdown.png"),"exit",
                        this, SLOT(close()));
    sysIcon->setContextMenu(snapMenu);
    sysIcon->show();
}

void MainWindow::createWidget(){
    titleBar = new QWidget(this);
    playMiniWgt = new QWidget(this) ;
    playWgt = new QWidget(this);
    centerWgt = new QWidget();
    groupListPop = new QFrame(this);
    fileDia = new QFileDialog(this);
    soundSlider = new QSlider(Qt::Horizontal,this);
    ui_TitleBar.setupUi(titleBar);
    ui_MiniPlay.setupUi(playMiniWgt);
    ui_MainWindow.setupUi(playWgt);
    ui_ViewCenter.setupUi(centerWgt);
    ui_ListGroup.setupUi(groupListPop);
    list = ui_ListGroup.listWidget;
    tableView = ui_ViewCenter.tableView;
    slider = ui_MainWindow.slider;
    slider->installEventFilter(this);
    btnGroup->addButton( ui_MainWindow.SequenceBtn, 0);
    btnGroup->addButton( ui_MainWindow.shuffleBtn, 1);
    btnGroup->addButton( ui_MainWindow.repeatBtn, 2);

    //MainWindow option
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(soundSlider);
    QMenu *snapMenu = new QMenu();
    snapMenu->addAction(action);
    ui_MainWindow.soundBtn->setMenu(snapMenu);
    ui_MiniPlay.soundBtn->setMenu(snapMenu);

    //viewCenter option
    tableModel = playTableModel = nullptr;
    TableView::iconMap.insert(":/img/img/collect.png",new QIcon(":/img/img/collect.png"));
    qDebug()<<tableModel;
    QMenu *menu = new QMenu();
    addDirAct = menu->addAction(QIcon(":/img/img/file-open.png"), "添加目录");
    addFilesAct = menu->addAction(QIcon(":/img/img/text.png"), "添加文件");
    ui_ViewCenter.optionBtn->setMenu(menu);

    //groupListPop option
    groupListPop->setWindowFlags(Qt::SplashScreen);
    groupListPop->setAttribute(Qt::WA_TranslucentBackground);
    groupListPop->setWindowOpacity(0.6);
}

void MainWindow::createLayout()
{
    mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->setSpacing(2);
    mainLayout->addWidget(titleBar, 0, Qt::AlignTop);
    mainLayout->addWidget(playMiniWgt, 0, Qt::AlignTop);
    mainLayout->addWidget(playWgt, 0, Qt::AlignTop);
    mainLayout->addWidget(centerWgt);

    playMiniWgt->hide();
    setLayout(mainLayout);
}

void MainWindow::connection(){
    //titleBar connect
    connect(ui_TitleBar.miniBtn,  SIGNAL(released()), this, SLOT(showMinimized()));
    connect(ui_TitleBar.modelBtn, SIGNAL(released()), this, SLOT(modelSwitch()));
    connect(ui_TitleBar.closeBtn, SIGNAL(released()), this, SLOT(closeHandle()));

    //miniplay wgt;
    connect(ui_MiniPlay.miniBtn,  SIGNAL(released()), this, SLOT(showMinimized()));
    connect(ui_MiniPlay.modelBtn, SIGNAL(released()), this, SLOT(modelSwitch()));
    connect(ui_MiniPlay.closeBtn, SIGNAL(released()), this, SLOT(closeHandle()));
    connect(ui_MiniPlay.BFBtn, SIGNAL(pressed()), this, SLOT(BFMusic()));
    connect(ui_MiniPlay.playBtn, SIGNAL(pressed()), this, SLOT(playMusic()));
    connect(ui_MiniPlay.FFBtn, SIGNAL(pressed()), this, SLOT(FFMusic()));
    connect(ui_MiniPlay.lisBtn, SIGNAL(pressed()), this, SLOT(hideCenterView()));

    //play widget connect
    connect(ui_MainWindow.BFBtn, SIGNAL(pressed()), this, SLOT(BFMusic()));
    connect(ui_MainWindow.playBtn, SIGNAL(pressed()), this, SLOT(playMusic()));
    connect(ui_MainWindow.FFBtn, SIGNAL(pressed()), this, SLOT(FFMusic()));
    connect(ui_MainWindow.lisBtn, SIGNAL(pressed()), this, SLOT(hideCenterView()));
    connect(soundSlider, SIGNAL(valueChanged(int)), this,SLOT(volumeChange(int)));

    //viewCenter connect
    connect(ui_ViewCenter.listBtn, SIGNAL(toggled(bool)), this, SLOT(groupList(bool)));
    connect(addDirAct, SIGNAL(triggered()), this, SLOT(addDir()));
    connect(addFilesAct, SIGNAL(triggered()), this, SLOT(addFiles()));
    connect(tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(playMusic(QModelIndex)));
    connect(tableView, SIGNAL(updateTable()), this , SLOT(updateTableData()));

    //groupListPop connect
    connect(list, SIGNAL(currentRowChanged(int)), this, SLOT(updateTableData()));
    connect(ui_ListGroup.newBtn, SIGNAL(released()), this, SLOT(newGroupList()));
    connect(ui_ListGroup.editBtn, SIGNAL(released()), this, SLOT(editGroupList()));
    connect(ui_ListGroup.delBtn, SIGNAL(released()), this, SLOT(delGroupList()));

    //sysicon
    connect(sysIcon, &QSystemTrayIcon::activated,
            [=](QSystemTrayIcon::ActivationReason reason){
        if(reason == QSystemTrayIcon::DoubleClick){
            setVisible(true);
        }
    });
    //other
    connect(this, SIGNAL(decodeAudioSignal(QString)),
            threadA, SIGNAL(decodeAudio(QString)));
    connect(threadA, SIGNAL(startPlay()), this, SLOT(startPlay()));
    connect(btnGroup, SIGNAL(buttonToggled(QAbstractButton*, bool)),
            this, SLOT(playModelChange(QAbstractButton*, bool)));
    connect(&addDirThread, SIGNAL(updataTableData()), this, SLOT(updateTableData()));
    connect(this, SIGNAL(startPlaySignal()),
            audioThread,SIGNAL(startPlaySignal()));
    connect(this, SIGNAL(setVolume(qreal)), audioThread,
            SIGNAL(setVolume(qreal)));
    connect(this, SIGNAL(suspend()), audioThread, SIGNAL(suspend()));
    connect(this, SIGNAL(resume()), audioThread, SIGNAL(resume()));
    connect(audioThread, SIGNAL(notifySignal(qreal)),
            this, SLOT(notifySlot(qreal)));
    connect(audioThread, SIGNAL(stateChangedSignal(QAudio::State)),
            this, SLOT(stateChange(QAudio::State)));
    connect(this, SIGNAL(cacheFilePosChanged(quint64)),
            audioThread, SIGNAL(cacheFilePosChanged(quint64)));
    connect(&singleShot, SIGNAL(timeout()), this, SLOT(continuous()));
}

void MainWindow::loadSetting(){
    QSettings settings("Lqter", "LqtePlayer");
    QStringList tableList;
    int currentRow;
    int volume;
    tableList = settings.value("TableList").toStringList();
    currentRow = settings.value("CurrentRow").toInt();
    volume = settings.value("Volume").toInt();
    restoreGeometry(settings.value("Geometry").toByteArray());
    playMode = PlayMode(settings.value("PlayMode").toInt());
    switch(playMode){
    case Sequence:
        ui_MainWindow.SequenceBtn->setChecked(true);
        break;
    case Shuffle:
        ui_MainWindow.shuffleBtn->setChecked(true);
        break;
    case Repeat:
        ui_MainWindow.repeatBtn->setChecked(true);
        break;
    }
    QFile *snapFile;
    CustomModel *snapModel;
    QStandardItem *snapItem ;
    qint32 rowCount;
    if(!tableList.isEmpty()){
        list->insertItems(0, tableList);
        foreach(QString str, tableList){
            tableView->addMenu->addAction(str);
            tableView->moveMenu->addAction(str);
            snapModel = new CustomModel();
            tableView->tableModelMap[snapModel] = str;
            snapFile = new QFile(str+".tbe");
            if(snapFile->open(QIODevice::ReadWrite)){
                tableFileMap[snapModel] = snapFile;
                QDataStream in(snapFile);
                in.setVersion(QDataStream::Qt_5_2);
                in>>rowCount;
                for(int r = 0;r < rowCount;r++){
                    QStringList rowList;
                    QList<QStandardItem *>itemList;
                    in>>rowList;
                    for(int c = 0;c < 11;c++){
                        if(c == 7){
                            QIcon *icon = TableView::iconMap.value(rowList.at(6) ,nullptr);
                            if(!icon){
                                icon = new QIcon(rowList.at(6));
                                TableView::iconMap[rowList.at(6)] = icon;
                            }
                            snapItem = new QStandardItem(*TableView::iconMap[rowList.at(6)], rowList.at(c));
                        }
                        else{
                            snapItem = new QStandardItem(rowList.at(c));
                        }
                        itemList<<snapItem;
                    }
                    snapModel->appendRow(itemList);
                }
            }
            connectModel(snapModel);
        }
        list->setCurrentRow(currentRow);
        playTableModel = qobject_cast<CustomModel *>(tableView->model());
    }
    soundSlider->setValue(volume);
    bool miniModel = settings.value("ModelSwitch", false).toBool();
    if(miniModel)
        modelSwitch();
    bool centerViewIsHidden = settings.value("CenterViewIsHidden", false).toBool();
    if(centerViewIsHidden)
        hideCenterView();
    snapSize = settings.value("SnapSize",QSize(322,600)).toSize();
    this->currentRow =  settings.value("CurPlayRow", -1).toInt();
    if(this->currentRow != -1 ){
        snapStr = tableModel->index(this->currentRow, 8).data().toString();
        setPlayDivInfo();
    }
    fileDia->setDirectory(settings.value("FileDialogPath",".").toString());
}

void MainWindow::saveSetting(){
    QSettings settings("Lqter", "LqtePlayer");
    QStringList tableList;
    QFile *snapFile;
    QString tableName;
    CustomModel *snapModel;
    for(int i = 0;i < list->count();i++){
        tableName = list->item(i)->text();
        tableList<< tableName;
        snapModel = tableView->tableModelMap.key(tableName ,nullptr);
        snapFile = tableFileMap[snapModel];
        snapFile->reset();
        if(snapModel && snapModel->isDataChanged()){
            QDataStream out(snapFile);
            out<<(qint32)snapModel->rowCount();
            for(int r = 0;r < snapModel->rowCount();r++){
                QStringList strList;
                for(int c = 0;c < snapModel->columnCount();c++){
                    if(r == currentRow && c == 8){
                        strList<<snapStr;
                    }
                    else{
                        strList<< snapModel->index(r, c).data().toString();
                    }
                }
                out<<strList;
            }
            snapFile->close();
        }
    }
    settings.setValue("TableList", tableList);
    settings.setValue("CurrentRow", list->currentRow());
    settings.setValue("Volume", soundSlider->value());
    settings.setValue("PlayMode", playMode);
    settings.setValue("Geometry", saveGeometry());
    settings.setValue("ModelSwitch", playMiniWgt->isVisible());
    settings.setValue("CenterViewIsHidden", centerWgt->isHidden());
    settings.setValue("CurPlayRow", this->currentRow);
    settings.setValue("FileDialogPath", fileDia->directory().path());
    settings.setValue("SnapSize",snapSize);
}

//private function
QLineEdit *MainWindow::setItemToEditState(QListWidgetItem *item){
    Qt::ItemFlags flags = item->flags();
    item->setFlags(Qt::ItemIsEditable | flags);
    list->editItem(item);
    item->setFlags(flags);
    QLineEdit *editor = qobject_cast<QLineEdit * >(QApplication::focusWidget());
    editor->selectAll();
    return editor;
}

void MainWindow::getCorrectTableName(QString &tn){
    if(list->findItems(tn, Qt::MatchExactly).count() > 1){
        int i = 1;
        do{
            tn = list->currentItem()->text() + "(" + QString::number(i++) + ")";
            if(list->findItems(tn,Qt::MatchExactly).count() < 1){
                list->currentItem()->setText(tn);
                break;
            }
        }while(true);
    }
}

void MainWindow::connectModel(CustomModel *snapModel){
    connect(snapModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            snapModel, SLOT(dataChangedSlot()));
    connect(snapModel, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            snapModel, SLOT(dataChangedSlot()));
    connect(snapModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            snapModel, SLOT(dataChangedSlot()));

    connect(snapModel, &CustomModel::rowsInserted,
            [=](QModelIndex index,int start,int end){
        qDebug()<<"rowsInserted"<<start<<end;
        if(playTableModel == snapModel && currentRow > start &&currentRow > end){
            currentRow += end - start + 1;
            qDebug()<<"currentRow > start &&currentRow > end"<< currentRow;
        }
    });
    connect(snapModel, &CustomModel::rowsMoved,
            [=](QModelIndex srcP, int start, int end, QModelIndex desP, int desRow){
        qDebug()<<"move"<<start<<end;
        if(playTableModel == snapModel && start <= currentRow && end >= currentRow){
            currentRow = desRow + currentRow - start;
        }
    });
    connect(snapModel, &CustomModel::rowsRemoved,
            [=](QModelIndex index,int start,int end){
        qDebug()<<"remove"<<start<<end;
        if(playTableModel == snapModel && currentRow > start && currentRow > end){
            currentRow -= end - start + 1;
        }
    });
}

void MainWindow::hideCenterView(){
    bool boolean = centerWgt->isHidden();
    centerWgt->setVisible(boolean);
    setMinimumHeight(73);
    if(!boolean){
        snapSize = size();
        resize(322, mainLayout->sizeHint().height());
        isStretchable = false;
    }
    else{
        qDebug()<<snapSize;
        resize(snapSize);
        isStretchable = true;
    }
}

void MainWindow::continuous(){
    emit decodeAudioSignal(musicPath);
}

void MainWindow::setPlayDivInfo(){
    QString iconPath = playTableModel->index(currentRow, 6).data().toString();
    QString title = playTableModel->index(currentRow, 7).data().toString();
    ui_MainWindow.titleLabel->setText(title);
    ui_TitleBar.titleLabel->setText(title);
    ui_MiniPlay.titleLabel->setText(title);
    if(iconPath.isEmpty()){
        iconPath = ":/img/img/music.png";
    }
    QPixmap pix(iconPath);
    ui_MainWindow.lgoLabel->setPixmap(pix);
    ui_MiniPlay.lgoLabel->setPixmap(pix);
}

void MainWindow::parseLrcFile(QString path){
    // unfinished
    QFile snapFile(path);
    if(!snapFile.open(QIODevice::ReadOnly)){
        qDebug()<< "parseLrcFile open faild";
    }
    QString lrcLine = snapFile.readLine();
    QString lyrics ;
    int total = 0;
    int count = lrcLine.count();
    snapFile.close();
}

//slot
void MainWindow::modelSwitch(){
    bool boolean = playWgt->isHidden();
    playWgt->setVisible(boolean);
    titleBar->setVisible(boolean);
    playMiniWgt->setVisible(!boolean);
    qDebug()<<boolean<< minimumSize();
    if(!boolean)
        resize(322,height());
    if(centerWgt->isHidden()){
        setMinimumHeight(73);
        resize(322,sizeHint().height());
    }
}

void MainWindow::notifySlot(qreal value){
    if(!slider->isSliderDown()){
        slider->setValue(value / (playTableModel->index(currentRow, 9).data().toInt() / 1000.0000) * 200);
    }
    int duration = value;
    QTime t(0,0);
    t = t.addSecs(duration);
    ui_MainWindow.timeLabel->setText(t.toString("mm:ss"));
}

void MainWindow::stateChange(QAudio::State state){
    if(state == QAudio::IdleState){
        BFMusic();
    }
}

void MainWindow::BFMusic(){
    qDebug()<<"BfMusic";
    int count = playTableModel->rowCount();
    if(count == 0 || addDirThread.isRunning()) return ;
    int row;
    switch (playMode) {
      case Sequence :
        row = (currentRow + 1 > count - 1 ? 0 : currentRow + 1);
        break;
      case Shuffle :
        row = qrand() % count;
        break;
    case Repeat:
        row = currentRow;
    }
    playMusic(playTableModel->index(row, 4), true);
}

void MainWindow::playMusic(QModelIndex index, bool fromFForBF){
    //I also was really drunk~~
    if(addDirThread.isRunning()) return ;
    if(!index.isValid()){
        if(audioThread->audioState() == QAudio::ActiveState){
            QIcon ico(":/img/img/play.png");
            ui_MainWindow.playBtn->setIcon(ico);
            ui_MiniPlay.playBtn->setIcon(ico);
            movie->stop();
            emit suspend();
            return ;
        }
        if(audioThread->audioState() == QAudio::SuspendedState){
            QIcon ico(":/img/img/pause.png");
            ui_MainWindow.playBtn->setIcon(ico);
            ui_MiniPlay.playBtn->setIcon(ico);
            movie->start();
            emit resume();
            return ;
        }
    }
    QModelIndex snapIndex = playTableModel->index(currentRow, 8);
    delete tableView->indexWidget(snapIndex);
    if(snapIndex.data().toString().isEmpty()){
        playTableModel->setData(snapIndex , snapStr, Qt::DisplayRole);
    }
    if(!fromFForBF){
            int oldRow = list->row(
                         list->findItems(tableView->tableModelMap.value(playTableModel),
                                        Qt::MatchExactly).first());
            list->item(oldRow)->setIcon(QIcon());
            list->currentItem()->setIcon(QIcon(":/img/img/play.png"));
            QString tableName = list->currentItem()->text();
            playTableModel = tableView->tableModelMap.key(tableName);
    }
    if(index.isValid())
        currentRow = index.row();
    snapIndex = playTableModel->index(currentRow, 8);
    if(tableModel == playTableModel ){
        QLabel *snapLabel ;
        snapLabel = new QLabel();
        if(movie->state() == QMovie::NotRunning)
            movie->start();
        snapLabel->setMovie(movie);
        snapStr = snapIndex.data().toString();
        snapLabel->setWhatsThis(snapStr);
        playTableModel->setData(snapIndex, QVariant(), Qt::DisplayRole);
        tableView->setIndexWidget(snapIndex, snapLabel);
    }
    musicPath = playTableModel->index(currentRow,4).data().toString();
    if(musicPath.isEmpty()){
        return;
    }

    if(QFile::exists(musicPath)){
        setPlayDivInfo();
        if(singleShot.isActive()){
            singleShot.stop();
            singleShot.start(800);
        }
        else
            singleShot.start(800);
    }
    else{
        playTableModel->removeRow(currentRow);
        BFMusic();
    }
}

void MainWindow::FFMusic(){
    int count = playTableModel->rowCount();
    if(count == 0 || addDirThread.isRunning()) return ;
    int row;
    switch (playMode) {
      case Sequence :
        row = (currentRow - 1 >= 0 ? currentRow - 1 : count - 1);
        break;
      case Shuffle :
        row = qrand() % count;
        break;
    case Repeat:
        row = currentRow;
    }
    if(row >= playTableModel->rowCount() || row < 0)
        return;
    playMusic(playTableModel->index(row, 4),true);
}

void MainWindow::addDir(){
    //assert
    fileDia->setViewMode(QFileDialog::List);
    fileDia->setFileMode(QFileDialog::Directory);
    bool isContainSubDir=false;
    if(fileDia->exec()){
        QString path = fileDia->selectedFiles().first();
        QDir rootDir(path);
        if(QMessageBox::question(this, "是否包含子目录？","是否包含子目录？") ==
           QMessageBox::Yes){
            isContainSubDir = true;
        }
        addDirThread.setDir(rootDir, isContainSubDir);
    }
}

void MainWindow::addFiles(){
    fileDia->setViewMode(QFileDialog::List);
    fileDia->setNameFilter(filter);
    fileDia->setFileMode(QFileDialog::AnyFile);
    if(fileDia->exec()){
        QStringList files = fileDia->selectedFiles();
        foreach (QString MusicPath, files) {
            mgr->decodeAudio(MusicPath, true);
        }
        updateTableData();
    }
}

void MainWindow::groupList(bool isCheck){
    QPoint position = ui_ViewCenter.listBtn->mapToGlobal(QPoint());
    if(isCheck){
        groupListPop->setGeometry(position.x() - 16,
                               position.y() + ui_ViewCenter.listBtn->height() - 12,
                               width() / 2, centerWgt->height() / 2);
        groupListPop->setVisible(true);
    }
    else{
        groupListPop->setVisible(false);
    }
}

void MainWindow::updateTableData(){
    QString tableName = list->currentItem()->text();
    tableModel = tableView->tableModelMap.key(tableName, nullptr);
    tableView->setModel(tableModel);
    ui_ViewCenter.listBtn->setText(tableName + " ");
    if(tableModel == playTableModel ){
        QLabel *snapLabel ;
        snapLabel = new QLabel();
        snapLabel->setMovie(movie);
        snapLabel->setAttribute(Qt::WA_TranslucentBackground, true);
        tableView->setIndexWidget(playTableModel->index(currentRow,8),snapLabel);
        playTableModel->setData(playTableModel->index(currentRow,8),QVariant(),Qt::DisplayRole);
    }
    if(sender() == list){
        if(menuActAt >= 0){
            if(tableView->addMenu->actions().count()>menuActAt){
                tableView->addMenu->actions().at(menuActAt)->setVisible(true);
                tableView->moveMenu->actions().at(menuActAt)->setVisible(true);
            }
        }
        int i = 0;
        foreach(QAction *act,tableView->addMenu->actions()){
            if(act->text() == tableName){
                menuActAt = i;
                break;
            }
            i++;
        }
        tableView->addMenu->actions().at(menuActAt)->setVisible(false);
        tableView->moveMenu->actions().at(menuActAt)->setVisible(false);
    }
}

void MainWindow::newGroupList(){
    QString itemName = "新建列表";
    for(int i = 1;;i++){
        if(list->findItems(itemName + QString::number(i), Qt::MatchExactly).count() < 1){
            itemName = itemName + QString::number(i);
            break;
        }
    }
    int row = list->currentRow() + 1;
    qDebug()<<row<<currentRow<<tableView->tableModelMap<<tableFileMap;
    CustomModel *snapModel = new CustomModel;
    connectModel(snapModel);
    tableView->tableModelMap.insert(snapModel, itemName);
    QAction *actA = tableView->addMenu->addAction(itemName);
    QAction *actB = tableView->moveMenu->addAction(itemName);
    list->insertItem(row, itemName);
    list->setCurrentRow(row);
    QLineEdit *editor = setItemToEditState(list->item(row));
    connect(editor,&QLineEdit::editingFinished, [=](){
        QString tableName = editor->text();
        getCorrectTableName(tableName);
        actA->setText(tableName);
        actB->setText(tableName);
        tableView->tableModelMap[snapModel] = tableName;
        QFile *snapFile = new QFile(tableName + ".tbe");
        if(snapFile->open(QIODevice::ReadWrite)){
            tableFileMap[snapModel] = snapFile;
        }
    });
}

void MainWindow::editGroupList(){
    QString oldName = list->currentItem()->text();
    QLineEdit *editor = setItemToEditState(list->currentItem());
    connect(editor,&QLineEdit::editingFinished,[=](){
        QString tableName = editor->text();
        getCorrectTableName(tableName);
        CustomModel *snapModel = tableView->tableModelMap.key(oldName);
        tableView->tableModelMap[snapModel] = tableName;
        tableFileMap[snapModel]->rename(tableName + ".tbe");
        qDebug()<<tableFileMap[snapModel]->fileName();
        int i = 0;
        foreach(QAction *act,tableView->addMenu->actions()){
           if(act->text() == oldName){
               act->setText(tableName);
               tableView->moveMenu->actions().at(i)->setText(tableName);
               break;
           }
           i++;
        }
    });
}

void MainWindow::delGroupList(){
    if(list->count() <= 1)   return;
    QListWidgetItem *item = list->currentItem();
    QString tableName = item->text();
    CustomModel *snap = tableView->tableModelMap.key(tableName , nullptr);
    tableView->tableModelMap.remove(snap);
    QFile *file = tableFileMap[snap];
    file->remove();
    file->close();
    delete file;
    tableFileMap.remove(snap);
    delete snap;
    list->removeItemWidget(item);
    item->setHidden(true);
    delete item;
    QAction *actA, *actB;
    int i = 0;
    foreach(actA, tableView->addMenu->actions()){
       if(actA->text() == tableName){
           tableView->addMenu->removeAction(actA);
           actB = tableView->moveMenu->actions().at(i);
           tableView->moveMenu->removeAction(actB);
           delete actA;
           delete actB;
           break;
       }
       i++;
    }
}

void MainWindow::volumeChange(int volume){
    qreal vol = volume / 100.0000;
    soundSlider->setWhatsThis(QString::number(vol));
    if(vol > 0.3){
        ui_MainWindow.soundBtn->setIcon(QIcon(":/img/img/sound-full.png"));
        ui_MiniPlay.soundBtn->setIcon(QIcon(":/img/img/sound-full.png"));
    }
    else if(vol <= 0.3){
        ui_MainWindow.soundBtn->setIcon(QIcon(":/img/img/sound-small.png"));
        ui_MiniPlay.soundBtn->setIcon(QIcon(":/img/img/sound-small.png"));
    }
    emit setVolume(vol);
}

void MainWindow::playModelChange(QAbstractButton *btn, bool checked){
    if(!checked) return ;
    if(btn == ui_MainWindow.SequenceBtn){
        playMode = Sequence;
    }
    if(btn == ui_MainWindow.shuffleBtn){
        playMode = Shuffle;
    }
    if(btn == ui_MainWindow.repeatBtn){
        playMode = Repeat;
    }
}

void MainWindow::startPlay(){
    QIcon ico(":/img/img/pause.png");
    ui_MainWindow.playBtn->setIcon(ico);
    ui_MiniPlay.playBtn->setIcon(ico);
    emit startPlaySignal();
}

void MainWindow::closeHandle(){
    setVisible(false);
}
//event
void MainWindow::closeEvent(QCloseEvent *ev){
    saveSetting();
    return QFrame::closeEvent(ev);
}

void MainWindow::mousePressEvent(QMouseEvent *ev){
    if(cursor().shape() == Qt::ArrowCursor){
        QWidget *wgt = childAt(ev->pos());
        if(!wgt){
            return ;
        }
        QString clsName = wgt->metaObject()->className();
        if((clsName != "QWidget") &&
           (clsName != "QLabel")  &&
           (clsName != "QFrame")){
            return;
        }
        if(ev->buttons() & Qt::LeftButton){
            state = MoveingState;
            xOffset = ev->x();
            yOffset = ev->y();
            return;
        }
        if(ev->buttons() & Qt::RightButton ){
            hideCenterView();
            return;
        }
    }
    else{
        QFrame::mousePressEvent(ev);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *ev){
    QWidget *wgt = childAt(ev->pos());
    if(wgt && (wgt->parent() == titleBar || wgt == titleBar)){
        ui_TitleBar.closeBtn->show();
        ui_TitleBar.modelBtn->show();
        ui_TitleBar.miniBtn->show();
    }
    else{
        ui_TitleBar.closeBtn->hide();
        ui_TitleBar.modelBtn->hide();
        ui_TitleBar.miniBtn->hide();
    }
    if(NoState == state && isStretchable){
        if(ev->x() < 0+5){
            stretchDire = LeftStretch;
            setCursor(Qt::SizeHorCursor);
        }
        else if(ev->x() > width()-5){
            stretchDire = RightStretch;
            setCursor(Qt::SizeHorCursor);
        }
        else if(ev->y() < 0+5){
            stretchDire = UpStretch;
            setCursor(Qt::SizeVerCursor);
        }
        else if(ev->y() > height()-5){
            stretchDire = DownStretch;
            setCursor(Qt::SizeVerCursor);
        }
        else{
            stretchDire = NoStretch;
            setCursor(Qt::ArrowCursor);
        }
    }
    if(ev->buttons()&Qt::LeftButton){
        if(cursor().shape() == Qt::ArrowCursor){
            if(xOffset == -1 && yOffset == -1){
                return;
            }
            QPoint curPos = ev->globalPos();
            move(curPos.x() - xOffset,curPos.y() - yOffset);
            return ;
        }
        else{
            state = StretchState;
            QPoint position = pos();
            QPoint rightDownPos = QPoint(x()+width(), y()+height());
            int xStretch = 0,yStretch = 0;
            if(stretchDire == UpStretch){
                yStretch = y() - ev->globalY();
                position.setY((height()+yStretch>minimumHeight() ?
                               ev->globalY(): rightDownPos.y() - minimumHeight()));
            }
            else if(stretchDire == LeftStretch){
                xStretch = x()-ev->globalX();
                position.setX((width() + xStretch>minimumWidth() ?
                               ev->globalX():rightDownPos.x() - minimumWidth()));
            }
            else if(stretchDire == DownStretch){
                yStretch = ev->globalY() - y()-height();
            }
            else if(stretchDire == RightStretch){
                xStretch = ev->globalX() - x()-width();
            }
            move(position);
            int w = (playWgt->isHidden() ? width() : width() + xStretch );
            int h =  height() + yStretch;
            resize(w, h);
            return ;
        }
    }
    else{
        QFrame::mouseMoveEvent(ev);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *ev){
    if(ev->button() == Qt::LeftButton){
        xOffset = yOffset = -1;
        state = NoState;
        setCursor(Qt::ArrowCursor);
    }
    else{
        QFrame::mouseReleaseEvent(ev);
    }
}

void MainWindow::moveEvent(QMoveEvent *ev){
    if(ui_ViewCenter.listBtn->isChecked()){
        ui_ViewCenter.listBtn->setChecked(false);
    }
    return QFrame::moveEvent(ev);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e){
    if(obj == slider){
        QMouseEvent *ev = static_cast<QMouseEvent *>(e);
        if(e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease){
            return true;//ignore keyboard
        }
        if(e->type() == QEvent::MouseButtonPress){
            if(ev->buttons() & Qt::LeftButton){
                qDebug()<<ev->x() * 200 / slider->width();
                int value = ev->x() * 200 / slider->width();
                slider->setSliderDown(true);
                slider->setSliderPosition(value);
            }
            else{
                return true;
            }
        }
        if(e->type() == QEvent::MouseButtonRelease){
            if(slider->isSliderDown()){
                slider->setSliderDown(false);
                int time = playTableModel->index(currentRow, 9).data().toLongLong() * slider->sliderPosition() / 200 / 1000;
                quint64 offset = 44100 * 2 * 2 * time;
                emit cacheFilePosChanged(offset);
            }
        }
    }
    return QFrame::eventFilter(obj, e);
}

MainWindow::~MainWindow()
{
    threadA->quit();
    threadA->wait();
    addDirThread.quit();
    addDirThread.wait();
}
