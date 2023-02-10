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

    /* 绑定信号槽函数 */
    connect( ui->upload_file, SIGNAL( clicked() ), this, SLOT( upload() ) ); /* 上传 */
    connect( ui->download_file, SIGNAL( clicked() ), this, SLOT( download() ) ); /* 下载 */
    connect( ui->move_file, SIGNAL( clicked() ), this, SLOT( move() ) ); /* 移动文件 */
    connect( ui->rename_file, SIGNAL( clicked() ), this, SLOT( rename() ) ); /* 重命名文件 */
    connect( ui->delete_file, SIGNAL( clicked() ), this, SLOT( del() ) ); /* 删除文件 */
    connect( ui->mkdir, SIGNAL( clicked() ), this, SLOT( mkdir() ) ); /* 新建文件夹 */
    connect( ui->open_dir, SIGNAL( clicked() ), this, SLOT( in_dir() ) ); /* 进入文件夹 */
    connect( ui->regist, SIGNAL( clicked() ), this, SLOT( regist() ) ); /* 设置用户认证信息 */
    connect( ui->list, SIGNAL( clicked() ), this, SLOT( list() ) ); /* 显示文件列表 */
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::upload()
{
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "选择文件" ) );
    fileDialog->setDirectory( "./" );
    fileDialog->setNameFilter( tr( "File()" ) );
    fileDialog->setFileMode( QFileDialog::ExistingFiles );
    fileDialog->setViewMode( QFileDialog::Detail );

    QString file_name;
    if ( fileDialog->exec() )
    {
        fileDialog->selectFile( file_name );
    }
    QString cmd = join_cmd( { "starClient.exe", "--upload", qstr2str( file_name ).c_str() } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    /* 循环读取 */
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

    fileDialog->setWindowTitle( QStringLiteral( "选择文件" ) );
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
                              qstr2str( file_name ).c_str() } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    /* 循环读取 */
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

    fileDialog->setWindowTitle( QStringLiteral( "选择文件" ) );
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
                              qstr2str( move_path ).c_str() } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    /* 循环读取 */
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
    this, tr( "文件重命名" ), tr( "请输入文件名：" ), QLineEdit::Normal, tr( "new_name" ), &flag );
    QString file_name = ui->listWidget->currentItem()->text();
    QString cmd       = join_cmd( { "starClient.exe",
                              "--f",
                              qstr2str( file_name ).c_str(),
                              "--rename",
                              qstr2str( rename_name ).c_str() } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    /* 循环读取 */
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
    QString cmd = join_cmd( { "starClient.exe", "--rm", qstr2str( file_name ).c_str() } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    /* 循环读取 */
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
    this, tr( "创建新文件夹" ), tr( "请输入文件夹名：" ), QLineEdit::Normal, tr( "new_dir" ), &flag );
    QString cmd = join_cmd( { "starClient.exe", "--mkdir", qstr2str( dir_name ).c_str() } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    /* 循环读取 */
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
    QString cmd = join_cmd( { "starClient.exe", "--login" } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    /* 循环读取 */
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
    this, tr( "设置认证信息" ), tr( "请输入用户名：" ), QLineEdit::Normal, tr( "admin" ), &flag );
    QString user_pwd = QInputDialog::getText(
    this, tr( "设置认证信息" ), tr( "请输入密码：" ), QLineEdit::Normal, tr( "xxxx" ), &flag );
    QString cmd = join_cmd( { "starClient.exe",
                              "--u",
                              qstr2str( user_name ).c_str(),
                              "--reset",
                              qstr2str( user_pwd ).c_str() } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    /* 循环读取 */
    while ( !process.atEnd() )
    {
        QString output = QLatin1String( process.readLine() );
        output.replace( "\n", " " );
        ui->status_bar->setText( output );
    }
}

void MainWindow::list()
{
    QString cmd = join_cmd( { "starClient.exe", "--list" } ); /* 拼命令 */
    QProcess process;
    process.start( cmd );
    QStringList files;
    /* 循环读取 */
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
