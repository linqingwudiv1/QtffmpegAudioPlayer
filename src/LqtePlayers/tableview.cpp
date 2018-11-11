#include "tableview.h"

#include <QDebug>
#include <QMouseEvent>
#include <QScrollBar>
#include <QMenu>
#include <QHeaderView>
#include <QStyleOptionViewItem>
#include <QProcess>

#include <QPainter>
QMap<QString ,QIcon *>TableView::iconMap = QMap<QString ,QIcon *>();

CustomModel::CustomModel(QObject *parent) :
    QStandardItemModel(parent)
{
    isDataChange = false;
}

bool CustomModel::setData(const QModelIndex &index, const QVariant &value, int role){
    return QStandardItemModel::setData(index, value, role);
}

void CustomModel::dataChangedSlot(){
    isDataChange = true;
    qDebug()<<"hehe";
    disconnect(this, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(dataChangedSlot()));
    disconnect(this, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(dataChangedSlot()));
    disconnect(this, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(dataChangedSlot()));
}

bool CustomModel::isDataChanged(){
    return isDataChange  ;
}

//NoFocusDelegate;
NoFocusDelegate::NoFocusDelegate(QObject *parent):
    QItemDelegate(parent){

}

void NoFocusDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const{
    QStyleOptionViewItem itemOption(option);
    if (itemOption.state & QStyle::State_HasFocus)
    {
       itemOption.state = itemOption.state ^ QStyle::State_HasFocus;
    }
    QItemDelegate::paint(painter, itemOption, index);
}

TableView::TableView(QWidget *parent) :
    QTableView(parent)
{
    setItemDelegate(new NoFocusDelegate());
    setAlternatingRowColors(true);
    horizontalScrollBar()->setEnabled(false);
    horizontalScrollBar()->setVisible(true);
    horHeadView = horizontalHeader();
    horHeadView->setSectionResizeMode(QHeaderView::Fixed);
    setIconSize(QSize(20,20));
    qDebug()<<verticalHeader()->defaultSectionSize();
    menu = new QMenu(this);
    playAct = menu->addAction(QIcon(":/img/img/play.png"), "Play");
    menu->addSeparator();
    addMenu = menu->addMenu(QIcon(":img/img/add.png"), "Add to");
    moveMenu = menu->addMenu(QIcon(":/img/img/text.png"), "Move to");
    delAct = menu->addAction(QIcon(":img/img/del.png"), "Del");
    clearAct = menu->addAction("Clear invlid music~");
    explorerAct = menu->addAction("open explorer~");
    infoAct = menu->addAction(QIcon(":/img/img/info.png"), "Details");

    //connect
    connect(addMenu, SIGNAL(triggered(QAction*)), this, SLOT(addToMenu(QAction*)));
    connect(moveMenu, SIGNAL(triggered(QAction*)), this, SLOT(moveToMenu(QAction*)));
    connect(playAct, SIGNAL(triggered()), this, SLOT(playEmit()));
    connect(delAct, SIGNAL(triggered()), this, SLOT(delEmit()));
    connect(clearAct, SIGNAL(triggered()), this, SLOT(clearEmit()));
    connect(explorerAct, SIGNAL(triggered()), this, SLOT(exoplorer()));
    connect(infoAct, SIGNAL(triggered()), this, SLOT(infoEmit()));
}

void TableView::setModel(QAbstractItemModel *model){
    QTableView::setModel(model);
    horHeadView->resizeSection(7, width() - 60);
    horHeadView->resizeSection(8, 48);
    for(int i = 0;i <= 10;i++){
        if(i != 7 && i != 8 )
            setColumnHidden(i,true);
    }
    emit modelChanged();
}

//virtual protected

// slot
void TableView::addToMenu(QAction *act){
    QString toTable = act->text();
    CustomModel *snapModel = tableModelMap.key(toTable);
    QStandardItem *item;
    int curRow;
    foreach(QModelIndex index, selectedIndexes()){
        if(index.column() == 7){
            curRow = index.row();
            QList<QStandardItem *>itemList ;
            for(int i = 0;i < 11;i++){
                if(i != 7){
                    item = new QStandardItem(model()->index(curRow, i).data().toString());
                }
                else{
                    item = new QStandardItem(index.data(Qt::DecorationRole).value<QIcon>(),
                                             index.data().toString());
                }
                itemList<<item ;
            }
            snapModel->appendRow(itemList);
        }
    }
}

void TableView::moveToMenu(QAction *act){
    qDebug()<<"movetoMenu"<<addMenu<<moveMenu <<sender();;
    addToMenu(act);
    delEmit();
}

void TableView::playEmit(){
    emit doubleClicked(selectedIndexes().back());
}

void TableView::delEmit(){
    QList<int > removeRows;
    foreach(QModelIndex index, selectedIndexes()){
        if(index.column() == 7){
            removeRows<< index.row();
        }
    }
    qSort(removeRows.begin(),removeRows.end());
    for(int i = removeRows.count()-1 ;i >= 0;i--){
        model()->removeRow(removeRows.at(i));
    }
}

void TableView::clearEmit(){
}

void TableView::exoplorer(){
    int row = currentIndex().row();
    QString path = model()->index(row, 4).data().toString();
    path.replace("//","\\");
    path.replace("/","\\");
    QProcess::startDetached("explorer /select,"+path);
}

void TableView::infoEmit(){
}

//event;
void TableView::mouseDoubleClickEvent(QMouseEvent *event){
    if(event->buttons() & Qt::RightButton){
        return ;
    }
    return QTableView::mouseDoubleClickEvent(event);
}

void TableView::mousePressEvent(QMouseEvent *event){
    QTableView::mousePressEvent(event);
    if(event->buttons() & Qt::RightButton){
        menu->move(event->globalPos());
        menu->show();
    }
}

void TableView::mouseMoveEvent(QMouseEvent *event){
    if(cursor().shape() != Qt::ArrowCursor){
        setCursor(Qt::ArrowCursor);
    }
    return QTableView::mouseMoveEvent(event);
}

void TableView::resizeEvent(QResizeEvent *event){
    horHeadView->resizeSection(7, width() - 60);
    horHeadView->resizeSection(8, 48);
    QTableView::resizeEvent(event);
}
