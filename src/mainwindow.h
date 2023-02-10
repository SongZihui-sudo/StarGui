#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

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
    virtual void move();

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

private:
    Ui::MainWindow* ui;
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
