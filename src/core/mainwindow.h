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
            �����ƶ��ļ�
         */
        virtual bool bread_chunk( size_t& index, QByteArray& buffer ) = 0;

        /*
            ������д�ļ�
         */
        virtual bool bwrite_chunk( QByteArray buffer ) = 0;

        /*
            �ļ��Ĵ�С
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
       ׷���ļ�����
    */
    virtual bool append( QString file_name, QString buffer, size_t index, QTcpSocket* tcpSocket );
    
     /*
       ѯ��ָ���ļ���Ԫ����
     */
    virtual star::file_meta_data
    get_file_meta_data( QString file_name, QString file_path = "UNKNOW" );

    /*   
        ��¼
    */
    virtual bool login( QString user_name = "UNKNOW", QString user_password = "UNKNOW" );

    /* �źŲۺ��� */
private slots:
    /*
        �ϴ�
    */
    virtual void upload();

    /*
        ����
    */
    virtual void download();

    /*
        �ƶ��ļ�
    */
    virtual void movefile();

    /*
        �ļ�������
    */
    virtual void rename();

    /*
        ɾ���ļ�
    */
    virtual void del();

    /*
        �½��ļ���
    */
    virtual void mkdir();

    /*
        �����ļ���
    */
    virtual void in_dir();

    /*
        �����û���֤��Ϣ
    */
    virtual void regist();

    /*
        ˢ���ļ��б�
    */
    virtual void list();

    /*
        �ر�
    */
    virtual void close_window();

    /*
        ��С��
    */
    virtual void minium_window();

    /*
        ���
    */
    virtual void maxium_window();

private:
    Ui::MainWindow* ui;
    bool m_bDragging;         /* �Ƿ������϶� */
    QPoint m_poStartPosition; /* �϶���ʼǰ�����λ�� */
    QPoint m_poFramePosition; /* �����ԭʼλ�� */

private:
    QDir m_dir;
    star::config::ptr m_settings;                  /* �ͻ������� */
    QString m_addr;                                /* �ͻ��˵�ַ */
    qint16 m_port;                                 /* �ͻ��˶˿� */
    QString m_current_path;                        /* ��ǰ���ڵ�·�� */
    master_server_info m_master_server_info;       /* master server ��Ϣ */
    qint64 max_chunk_size;                         /* �����Ĵ�С */
    user_info m_user;                              /* �û���֤��Ϣ */
    qint16 m_thread_num;                           /* �߳��� */
    qint64 m_buffer_size;                          /* ���ջ������Ĵ�С */
};

/* �� qstring ת string */
static std::string qstr2str( const QString qstr )
{
    QByteArray cdata = qstr.toLocal8Bit();
    return std::string( cdata );
}

/*
    ƴһ������
*/
QString join_cmd( std::initializer_list< QString > args );

/*
    �ı��ļ�
*/
class txtFileitem : public MainWindow::fileitem
{
public:
    txtFileitem( QString path, size_t chunk_size )
    : MainWindow::fileitem( path, chunk_size ){};
    virtual ~txtFileitem() = default;
    /*
        �����ƶ��ļ�
    */
    virtual bool bread_chunk( size_t& index, QByteArray& buffer );

    /*
        ������д�ļ�
     */
    virtual bool bwrite_chunk( QByteArray buffer );

    void save(){};

public:
    /*
        ��ȡȫ������
    */
    virtual bool readAll( QString& buffer );
};

/*
    ͼƬ�ļ�
*/
class imgFileitem : public MainWindow::fileitem
{
public:
    imgFileitem( QString path, size_t chunk_size )
    : MainWindow::fileitem( path, chunk_size )
    {
        QFileInfo fi( this->m_path );
        /* ��ͼƬװ����base64 */
        QImage image( this->m_path );
        QBuffer buffer( &this->imageData );
        image.save( &buffer, qstr2str(fi.suffix()).c_str() );
        this->imageData = this->imageData.toBase64();
    };
    virtual ~imgFileitem() = default;
    /*
        �����ƶ��ļ�
    */
    virtual bool bread_chunk( size_t& index, QByteArray& buffer );

    /*
        ������д�ļ�
     */
    virtual bool bwrite_chunk( QByteArray buffer );

    /*
        ����ͼƬ
    */
    virtual void save();

public:
    /*
        ��ȡȫ������
    */
    virtual bool readAll( QString& buffer );

private:
    QByteArray imageData;
};

#endif // MAINWINDOW_H
