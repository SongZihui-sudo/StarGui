#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QString>
#include <QDir>
#include <QThread>
#include <QListWidget>
#include <QVector>
#include <QSharedPointer>
#include <QTcpSocket>
#include <star.h>
#include <QBuffer>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    class fileitem
    {
    public:
        typedef QSharedPointer< fileitem > ptr;

        fileitem( QString path, size_t chunk_size );
        virtual ~fileitem() = default;

        /*
            二进制读文件
         */
        virtual bool bread_chunk( size_t& index, QByteArray& buffer ) = 0;

        /*
            二进制写文件
         */
        virtual bool bwrite_chunk( QByteArray buffer ) = 0;

        /*
            文件的大小
        */
        virtual qint64 size();

        virtual void save() = 0;

    protected:
        size_t m_chunk_size;
        QString m_path;
    };

public:
    struct master_server_info
    {
        QString addr;
        int64_t port;
    };

    struct user_info
    {
        QString user_name;
        QString user_pwd;
    };

public:
    explicit MainWindow( QWidget* parent = 0 );
    ~MainWindow();

    virtual void paintEvent( QPaintEvent* event ) override;
    
    virtual void mousePressEvent( QMouseEvent* event ) override;
    
    virtual void mouseMoveEvent( QMouseEvent* event ) override;

    virtual void mouseReleaseEvent( QMouseEvent* event ) override;

    /*
       追加文件内容
    */
    virtual bool append( QString file_name, QString buffer, size_t index, QTcpSocket* tcpSocket );
    
     /*
       询问指定文件的元数据
     */
    virtual star::file_meta_data
    get_file_meta_data( QString file_name, QString file_path = "UNKNOW" );

    /*   
        登录
    */
    virtual bool login( QString user_name = "UNKNOW", QString user_password = "UNKNOW" );

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

private:
    QDir m_dir;
    star::config::ptr m_settings;                  /* 客户端配置 */
    QString m_addr;                                /* 客户端地址 */
    qint16 m_port;                                 /* 客户端端口 */
    QString m_current_path;                        /* 当前所在的路径 */
    master_server_info m_master_server_info;       /* master server 信息 */
    qint64 max_chunk_size;                         /* 块最大的大小 */
    user_info m_user;                              /* 用户认证信息 */
    qint16 m_thread_num;                           /* 线程数 */
    qint64 m_buffer_size;                          /* 接收缓存区的大小 */
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

/*
    文本文件
*/
class txtFileitem : public MainWindow::fileitem
{
public:
    txtFileitem( QString path, size_t chunk_size )
    : MainWindow::fileitem( path, chunk_size ){};
    virtual ~txtFileitem() = default;
    /*
        二进制读文件
    */
    virtual bool bread_chunk( size_t& index, QByteArray& buffer );

    /*
        二进制写文件
     */
    virtual bool bwrite_chunk( QByteArray buffer );

    void save(){};

public:
    /*
        读取全部内容
    */
    virtual bool readAll( QString& buffer );
};

/*
    图片文件
*/
class imgFileitem : public MainWindow::fileitem
{
public:
    imgFileitem( QString path, size_t chunk_size )
    : MainWindow::fileitem( path, chunk_size )
    {
        QFileInfo fi( this->m_path );
        /* 把图片装换成base64 */
        QImage image( this->m_path );
        QBuffer buffer( &this->imageData );
        image.save( &buffer, qstr2str(fi.suffix()).c_str() );
        this->imageData = this->imageData.toBase64();
    };
    virtual ~imgFileitem() = default;
    /*
        二进制读文件
    */
    virtual bool bread_chunk( size_t& index, QByteArray& buffer );

    /*
        二进制写文件
     */
    virtual bool bwrite_chunk( QByteArray buffer );

    /*
        保存图片
    */
    virtual void save();

public:
    /*
        读取全部内容
    */
    virtual bool readAll( QString& buffer );

private:
    QByteArray imageData;
};

#endif // MAINWINDOW_H
