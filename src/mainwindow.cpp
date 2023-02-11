#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include <QProcess>
#include <QInputDialog>

MainWindow::MainWindow( QWidget* parent )
: QMainWindow( parent )
, ui( new Ui::MainWindow )
{
    ui->setupUi( this );
    this->setWindowFlags( Qt::FramelessWindowHint );
    this->setAttribute( Qt::WA_TranslucentBackground );
    this->setWindowOpacity( 0.8 );

    m_bDragging = false;
    /* ���źŲۺ��� */
    connect( ui->upload_file, SIGNAL( clicked() ), this, SLOT( upload() ) ); /* �ϴ� */
    connect( ui->download_file, SIGNAL( clicked() ), this, SLOT( download() ) ); /* ���� */
    connect( ui->move_file, SIGNAL( clicked() ), this, SLOT( movefile() ) ); /* �ƶ��ļ� */
    connect( ui->rename_file, SIGNAL( clicked() ), this, SLOT( rename() ) ); /* �������ļ� */
    connect( ui->delete_file, SIGNAL( clicked() ), this, SLOT( del() ) ); /* ɾ���ļ� */
    connect( ui->mkdir, SIGNAL( clicked() ), this, SLOT( mkdir() ) ); /* �½��ļ��� */
    connect( ui->open_dir, SIGNAL( clicked() ), this, SLOT( in_dir() ) ); /* �����ļ��� */
    connect( ui->regist, SIGNAL( clicked() ), this, SLOT( regist() ) ); /* �����û���֤��Ϣ */
    connect( ui->list, SIGNAL( clicked() ), this, SLOT( list() ) ); /* ��ʾ�ļ��б� */
    connect( ui->close, SIGNAL( clicked() ), this, SLOT( close_window() ) ); /* �رմ��� */
    connect( ui->minium, SIGNAL( clicked() ), this, SLOT( minium_window() ) ); /* ��С�� */
    connect( ui->min_big, SIGNAL( clicked() ), this, SLOT( maxium_window() ) ); /* ��� */

    this->list();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::paintEvent( QPaintEvent* event )
{
    QStyleOption opt;
    opt.initFrom( this );
    QPainter painter( this );
    style()->drawPrimitive( QStyle::PE_Widget, &opt, &painter, this );
}

//���ڿ��϶�
void MainWindow::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        QRect rect = ui->centralWidget->rect(); /* rect�����ʵ�ֿ��϶������� */
        rect.setBottom( rect.top() + 220 );
        if ( rect.contains( event->pos() ) )
        {
            m_bDragging       = true;
            m_poStartPosition = event->globalPos();
            m_poFramePosition = frameGeometry().topLeft();
        }
    }
    QWidget::mousePressEvent( event );
}

void MainWindow::mouseMoveEvent( QMouseEvent* event )
{
    if ( event->buttons() & Qt::LeftButton ) /* ֻ��Ӧ������� */
    {
        if ( m_bDragging )
        {
            QPoint delta = event->globalPos() - m_poStartPosition;
            this->move( m_poFramePosition + delta );
        }
    }
    QWidget::mouseMoveEvent( event );
}

void MainWindow::mouseReleaseEvent( QMouseEvent* event )
{
    m_bDragging = false;
    QWidget::mouseReleaseEvent( event );
}

void MainWindow::upload()
{
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "Choose File" ) );
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

    fileDialog->setWindowTitle( QStringLiteral( "Choose File" ) );
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

void MainWindow::movefile()
{
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "Choose File" ) );
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
    nullptr, tr( "File rename" ), tr( "Input file name:" ), QLineEdit::Normal, tr( "new_name" ), &flag );
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
    nullptr, tr( "Make new Dir" ), tr( "Input file name:" ), QLineEdit::Normal, tr( "new_dir" ), &flag );
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

void MainWindow::in_dir() 
{
    QString dir_name = ui->listWidget->currentItem()->text();
    QString path;
    if ( dir_name == "." )
    {
        return;
    }
    else if ( dir_name == ".." )
    {
        if ( this->m_dir.cdUp() )
        {
            path = this->m_dir.path();
        }
        else
        {
            return;
        }
    }
    else
    {
        this->m_dir.cd( dir_name );
    };
    this->list();
}

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
    QString user_name = QInputDialog::getText( nullptr,
                                               tr( "Set authentication information" ),
                                               tr( "Input user name:" ),
                                               QLineEdit::Normal,
                                               tr( "admin" ),
                                               &flag );
    QString user_pwd  = QInputDialog::getText( nullptr,
                                              tr( "Set authentication information" ),
                                              tr( "Input password:" ),
                                              QLineEdit::Normal,
                                              tr( "xxxx" ),
                                              &flag );
    QString cmd       = join_cmd( { "starClient.exe",
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
    ui->listWidget->clear();                                  /* ����б��е����� */
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

    QFileInfoList list = this->m_dir.entryInfoList();
    ui->listWidget->addItems( files );
    QStringList dirs;
    for ( auto item : list )
    {
        QString temp;
        temp.push_back( item.fileName() );
        if ( item.isFile() )
        {
            temp.push_back( "                                                           " );
            temp.push_back( item.suffix() );
            temp.push_back( " file" );
        }
        else
        {
            temp.push_back( "                                                           " );
            temp.push_back( " dir" );
        }
        dirs << temp;
    }
    ui->listWidget->addItems( dirs );
}

void MainWindow::close_window() { bool b = this->close(); }

void MainWindow::minium_window() { this->showMinimized(); }

void MainWindow::maxium_window() 
{ 
    if ( this->isMaximized() )
    {
        this->showNormal();
        return;
    }
    this->showMaximized();
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
