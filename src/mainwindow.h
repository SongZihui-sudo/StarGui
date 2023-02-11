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
        ��¼
    */
    virtual void login();

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
    QDir m_dir;
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

#endif // MAINWINDOW_H
