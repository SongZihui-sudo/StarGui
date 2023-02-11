#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QString>
#include <QDir>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow( QWidget* parent = 0 );
    ~MainWindow();

    virtual void paintEvent( QPaintEvent* event ) override;
    virtual void mousePressEvent( QMouseEvent* event ) override;
    virtual void mouseMoveEvent( QMouseEvent* event ) override;
    virtual void mouseReleaseEvent( QMouseEvent* event ) override;

    /* 信号槽函数 */
private slots:
    /*
        上传
    */
    virtual void upload();

    /*
        下载
    */
    virtual void download();

    /*
        移动文件
    */
    virtual void movefile();

    /*
        文件重命名
    */
    virtual void rename();

    /*
        删除文件
    */
    virtual void del();

    /*
        新建文件夹
    */
    virtual void mkdir();

    /*
        进入文件夹
    */
    virtual void in_dir();

    /*
        登录
    */
    virtual void login();

    /*
        设置用户认证信息
    */
    virtual void regist();

    /*
        刷新文件列表
    */
    virtual void list();

    /*
        关闭
    */
    virtual void close_window();

    /*
        最小化
    */
    virtual void minium_window();

    /*
        最大化
    */
    virtual void maxium_window();

private:
    Ui::MainWindow* ui;
    bool m_bDragging;         /* 是否正在拖动 */
    QPoint m_poStartPosition; /* 拖动开始前的鼠标位置 */
    QPoint m_poFramePosition; /* 窗体的原始位置 */
    QDir m_dir;
};

/* 把 qstring 转 string */
static std::string qstr2str( const QString qstr )
{
    QByteArray cdata = qstr.toLocal8Bit();
    return std::string( cdata );
}

/*
    拼一个命令
*/
QString join_cmd( std::initializer_list< QString > args );

#endif // MAINWINDOW_H
