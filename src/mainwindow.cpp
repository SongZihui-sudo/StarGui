#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QProcess>

MainWindow::MainWindow( QWidget* parent )
: QMainWindow( parent )
, ui( new Ui::MainWindow )
{
    ui->setupUi( this );

    /* ���źŲۺ��� */
    connect( ui->upload_file, SIGNAL( clicked() ), this, SLOT( upload() ) ); /* �ϴ� */
    connect( ui->download_file, SIGNAL( clicked() ), this, SLOT( download() ) ); /* ���� */
    connect( ui->move_file, SIGNAL( clicked() ), this, SLOT( move() ) ); /* �ƶ��ļ� */
    connect( ui->rename_file, SIGNAL( clicked() ), this, SLOT( rename() ) ); /* �������ļ� */
    connect( ui->delete_file, SIGNAL( clicked() ), this, SLOT( del() ) ); /* ɾ���ļ� */
    connect( ui->mkdir, SIGNAL( clicked() ), this, SLOT( mkdir() ) ); /* �½��ļ��� */
    connect( ui->open_dir, SIGNAL( clicked() ), this, SLOT( in_dir() ) ); /* �����ļ��� */
    connect( ui->regist, SIGNAL( clicked() ), this, SLOT( regist() ) ); /* �����û���֤��Ϣ */
    connect( ui->list, SIGNAL( clicked() ), this, SLOT( list() ) ); /* ��ʾ�ļ��б� */
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::upload()
{
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "ѡ���ļ�" ) );
    fileDialog->setDirectory( "./" );
    fileDialog->setNameFilter( tr( "File()" ) );
    fileDialog->setFileMode( QFileDialog::ExistingFiles );
    fileDialog->setViewMode( QFileDialog::Detail );

    QString file_name;
    if ( fileDialog->exec() )
    {
        fileDialog->selectFile( file_name );
    }
    QString cmd = join_cmd( { "starClient.exe", "--upload", qstr2str( file_name ).c_str() } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::download()
{
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "ѡ���ļ�" ) );
    fileDialog->setDirectory( "./" );
    fileDialog->setNameFilter( tr( "File()" ) );
    fileDialog->setFileMode( QFileDialog::ExistingFiles );
    fileDialog->setViewMode( QFileDialog::Detail );

    QString download_path;
    if ( fileDialog->exec() )
    {
        fileDialog->selectFile( download_path );
    }
    QString file_name = ui->listWidget->currentItem()->text();
    QString cmd       = join_cmd( { "starClient.exe",
                              "--f",
                              qstr2str( download_path ).c_str(),
                              "--download",
                              qstr2str( file_name ).c_str() } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::move()
{
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "ѡ���ļ�" ) );
    fileDialog->setDirectory( "./" );
    fileDialog->setNameFilter( tr( "File()" ) );
    fileDialog->setFileMode( QFileDialog::ExistingFiles );
    fileDialog->setViewMode( QFileDialog::Detail );

    QString move_path;
    if ( fileDialog->exec() )
    {
        fileDialog->selectFile( move_path );
    }
    QString file_name = ui->listWidget->currentItem()->text();
    QString cmd       = join_cmd( { "starClient.exe",
                              "--f",
                              qstr2str( file_name ).c_str(),
                              "--move",
                              qstr2str( move_path ).c_str() } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::rename()
{
    bool flag           = true;
    QString rename_name = QInputDialog::getText(
    this, tr( "�ļ�������" ), tr( "�������ļ�����" ), QLineEdit::Normal, tr( "new_name" ), &flag );
    QString file_name = ui->listWidget->currentItem()->text();
    QString cmd       = join_cmd( { "starClient.exe",
                              "--f",
                              qstr2str( file_name ).c_str(),
                              "--rename",
                              qstr2str( rename_name ).c_str() } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::del()
{
    QString file_name = ui->listWidget->currentItem()->text();
    QString cmd = join_cmd( { "starClient.exe", "--rm", qstr2str( file_name ).c_str() } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::mkdir()
{
    bool flag;
    QString dir_name = QInputDialog::getText(
    this, tr( "�������ļ���" ), tr( "�������ļ�������" ), QLineEdit::Normal, tr( "new_dir" ), &flag );
    QString cmd = join_cmd( { "starClient.exe", "--mkdir", qstr2str( dir_name ).c_str() } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::in_dir() { this->list(); }

void MainWindow::login()
{
    QString cmd = join_cmd( { "starClient.exe", "--login" } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::regist()
{
    bool flag;
    QString user_name = QInputDialog::getText(
    this, tr( "������֤��Ϣ" ), tr( "�������û�����" ), QLineEdit::Normal, tr( "admin" ), &flag );
    QString user_pwd = QInputDialog::getText(
    this, tr( "������֤��Ϣ" ), tr( "���������룺" ), QLineEdit::Normal, tr( "xxxx" ), &flag );
    QString cmd = join_cmd( { "starClient.exe",
                              "--u",
                              qstr2str( user_name ).c_str(),
                              "--reset",
                              qstr2str( user_pwd ).c_str() } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::list()
{
    QString cmd = join_cmd( { "starClient.exe", "--list" } ); /* ƴ���� */
    QProcess process;
    process.start( cmd );
    QStringList files;
    /* ѭ����ȡ */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        files.push_back( output );
    }
    QDir cur_dir( "./" );
    cur_dir.setFilter( QDir::Dirs );
    QFileInfoList list = cur_dir.entryInfoList();
    ui->listWidget->addItems( files );
    QStringList dirs;
    for ( auto item : list )
    {
        dirs << item.fileName();
    }
    ui->listWidget->addItems( dirs );
}

QString join_cmd( std::initializer_list< QString > args )
{
    std::vector< QString > arg_list = args;
    QString cmd_line;
    for ( auto item : arg_list )
    {
        cmd_line.push_back( item );
        cmd_line.push_back( " " );
    }
    return QString();
}
