#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include <QTableView>
#include <QItemDelegate>
#include <QStandardItemModel>
class QMenu;
class QHeaderView;


class CustomModel : public QStandardItemModel{
    Q_OBJECT
public:
    explicit CustomModel(QObject *parent = 0);
    bool isDataChanged();
    bool setData(const QModelIndex &index, const QVariant &value, int role);
public slots:
    void dataChangedSlot();
private:
    bool isDataChange;
};

class NoFocusDelegate :public QItemDelegate{
    Q_OBJECT
public:
    explicit NoFocusDelegate(QObject *parent = 0);
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
};

class TableView : public QTableView
{
    Q_OBJECT
public:
    explicit TableView(QWidget *parent = 0);
    void setModel(QAbstractItemModel *model);
protected:
protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
signals:
    void updateTable();
    void modelChanged();
public slots:
    void addToMenu(QAction *act);
    void moveToMenu(QAction *act);
    void playEmit();
    void delEmit();
    void clearEmit();
    void exoplorer();
    void infoEmit();
public:
    QMenu *addMenu;
    QMenu *moveMenu;
    QMap<CustomModel *, QString>tableModelMap;
    static QMap<QString ,QIcon *>iconMap;
private:
    QMenu *menu;
    QHeaderView *horHeadView;
    QAction *playAct;
    QAction *delAct;
    QAction *clearAct;
    QAction *infoAct;
    QAction *explorerAct;

};

#endif // TABLEVIEW_H
