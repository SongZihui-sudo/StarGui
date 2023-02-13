#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include <QInputDialog>
#include <QNetworkProxy>
#include <thread>

star::Logger::ptr g_logger( STAR_NAME( "global_logger" ) );

MainWindow::MainWindow( QWidget* parent )
: QMainWindow( parent )
, ui( new Ui::MainWindow )
{
    ui->setupUi( this );
    this->setWindowFlags( Qt::FramelessWindowHint );
    this->setAttribute( Qt::WA_TranslucentBackground );
    this->setWindowOpacity( 0.8 );

    m_bDragging = false;

    /* 绑定信号槽函数 */
    connect( ui->upload_file, SIGNAL( clicked() ), this, SLOT( upload() ) ); /* 上传 */
    connect( ui->download_file, SIGNAL( clicked() ), this, SLOT( download() ) ); /* 下载 */
    connect( ui->move_file, SIGNAL( clicked() ), this, SLOT( movefile() ) ); /* 移动文件 */
    connect( ui->rename_file, SIGNAL( clicked() ), this, SLOT( rename() ) ); /* 重命名文件 */
    connect( ui->delete_file, SIGNAL( clicked() ), this, SLOT( del() ) ); /* 删除文件 */
    connect( ui->mkdir, SIGNAL( clicked() ), this, SLOT( mkdir() ) ); /* 新建文件夹 */
    connect( ui->open_dir, SIGNAL( clicked() ), this, SLOT( in_dir() ) ); /* 进入文件夹 */
    connect( ui->regist, SIGNAL( clicked() ), this, SLOT( regist() ) ); /* 设置用户认证信息 */
    connect( ui->list, SIGNAL( clicked() ), this, SLOT( list() ) ); /* 显示文件列表 */
    connect( ui->close, SIGNAL( clicked() ), this, SLOT( close_window() ) ); /* 关闭窗口 */
    connect( ui->minium, SIGNAL( clicked() ), this, SLOT( minium_window() ) ); /* 最小化 */
    connect( ui->min_big, SIGNAL( clicked() ), this, SLOT( maxium_window() ) ); /* 最大化 */

    this->m_settings.reset( new star::config( "client_settings.json" ) ); /* 读取配置文件 */
    this->m_addr              = this->m_settings->get( "address" ).asCString(); /* 地址 */
    this->m_port              = this->m_settings->get( "port" ).asInt64();      /* 端口 */
    m_master_server_info.addr = this->m_settings->get( "master_server" )["address"].asCString();
    m_master_server_info.port = this->m_settings->get( "master_server" )["port"].asInt64();
    this->m_dir.currentPath()      = QCoreApplication::applicationDirPath();
    this->max_chunk_size      = this->m_settings->get( "max_chunk_size" ).asInt();
    /* 用户的认证信息 */
    this->m_user.user_name = this->m_settings->get( "user" )["name"].asCString();
    this->m_user.user_pwd  = this->m_settings->get( "user" )["pwd"].asCString();
    this->m_thread_num     = this->m_settings->get( "thread_num" ).asInt();
    this->m_buffer_size    = this->m_settings->get( "buffer_size" ).asInt64();

    /* 连接下 master server 判断它在没在线 */
    QTcpSocket* tcpSocket = new QTcpSocket();
    tcpSocket->setProxy( QNetworkProxy::NoProxy );
    tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                              this->m_master_server_info.port );

    /* 等待连接成功 */
    if ( !tcpSocket->waitForConnected() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        ui->status_bar->setText( "INFO: Can not connect to the Master Server!" );
        return;
    }
    else
    {
        this->list();
    }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::paintEvent( QPaintEvent* event )
{
    QStyleOption opt;
    opt.initFrom( this );
    QPainter painter( this );
    style()->drawPrimitive( QStyle::PE_Widget, &opt, &painter, this );
}

//窗口可拖动
void MainWindow::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        QRect rect = ui->centralWidget->rect(); /* rect是鼠标实现可拖动的区域 */
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
    if ( event->buttons() & Qt::LeftButton ) /* 只响应鼠标所见 */
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
    ui->status_bar->setText( "INFO: Start upload file....................." );
    if ( !this->login() )
    {
        return;
    }
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "Choose File" ) );
    fileDialog->setDirectory( "./" );
    fileDialog->setNameFilter( tr( "File()" ) );
    fileDialog->setFileMode( QFileDialog::ExistingFiles );
    fileDialog->setViewMode( QFileDialog::Detail );

    QString file_name = fileDialog->getOpenFileName();
    QFileInfo temp( file_name );
    file_name = temp.fileName();
    
    QString path = QDir( this->m_dir.absolutePath() ).absoluteFilePath( file_name );
    size_t item_index = ui->process_list->count();
    ui->process_list->addItem( "Upload: " + this->m_dir.absolutePath() );
    bool flag{ false };

    std::thread upload_thread(
        [&]()
        {
        /* 读取，存储上传断点信息的临时文件 */
        QString temp_file_name = "upload-temp-" + file_name;
        QString temp_path = QDir( this->m_dir.absolutePath() ).absoluteFilePath( temp_file_name );
        QFile temp_file( temp_path );
        size_t beg_index = 0;

        if ( !temp_file.open( QIODevice::ReadOnly ) )
        {
            beg_index = 0;
        }

        /* 按行读 */
        size_t i = 0;
        while ( !temp_file.atEnd() )
        {
            QByteArray buffer = temp_file.readLine();
            qint64 j          = buffer.toLongLong();
            if ( j >= beg_index )
            {
                beg_index = j;
            }
            i++;
        }
        temp_file.close();

        QByteArray buffer;
        fileitem fi( path, this->max_chunk_size ); /* 要上传的文件 */

        /* 按块读文件并发生给服务端 */

        qint64 temp_index = beg_index;
        if ( temp_index > fi.size() )
        {
            ui->status_bar->setText( "INFO: Reach the end of the file!" );
            return;
        }

        /* 连接 master server */
        QTcpSocket* tcpSocket = new QTcpSocket();
        tcpSocket->setProxy( QNetworkProxy::NoProxy );
        tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                                  this->m_master_server_info.port );

        /* 等待连接成功 */
        if ( !tcpSocket->waitForConnected() )
        {
            FATAL_STD_STREAM_LOG( g_logger )
            << "%D"
            << "Can not connect to the Master Server: "
            << qstr2str( this->m_master_server_info.addr ) << ":"
            << S( this->m_master_server_info.port ) << " "
            << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
            ui->status_bar->setText( "INFO: Can not connect to the Master Server!" );
            return;
        }

        while ( fi.bread_chunk( beg_index, buffer ) )
        {
            /* 打开断点信息临时文件，记录断点信息 */
            if ( !temp_file.open( QIODevice::ReadWrite | QIODevice::Append ) )
            {
                FATAL_STD_STREAM_LOG( g_logger ) << "Open Temp file Error!" << star::Logger::endl();
                ui->status_bar->setText( "INFO: Open Temp file Error!" );
                return;
            }

            if ( !this->append( file_name, buffer, i, tcpSocket ) )
            {
                FATAL_STD_STREAM_LOG( g_logger )
                << "upload chunk " << S( i ) << "Error!" << star::Logger::endl();
                temp_file.close();
                tcpSocket = new QTcpSocket();
                tcpSocket->setProxy( QNetworkProxy::NoProxy );
                tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                                          this->m_master_server_info.port );
                flag = true;

                /* 等待连接成功 */
                if ( !tcpSocket->waitForConnected() )
                {
                    FATAL_STD_STREAM_LOG( g_logger )
                    << "%D"
                    << "Can not connect to the Master Server: "
                    << qstr2str( this->m_master_server_info.addr ) << ":"
                    << S( this->m_master_server_info.port ) << " "
                    << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
                    ui->status_bar->setText("INFO: Can not connect to the Master Server!" );

                    flag = true;
                    return;
                }

                continue;
            }

            /* 显示进度 */
            INFO_STD_STREAM_LOG( g_logger ) << "upload: " << S( beg_index ) << " B "
                                            << S( fi.size() ) << " B " << star::Logger::endl();
            QString item = path + "upload: " + file_name + " " + S( beg_index ).c_str() + " B "
                           + S( fi.size() ).c_str() + " B ";
            ui->process_list->setCurrentRow( item_index );
            QListWidgetItem* current_item = new QListWidgetItem( item );
            ui->process_list->setCurrentItem( current_item );

            //system( "cls" );    

            /* 写入断点信息 */
            std::string break_info = S( beg_index ) + "\n";
            temp_file.write( break_info.c_str() );
            i++;
            temp_file.close();
        }

        star::protocol::Protocol_Struct prs(
        104, "", "", "", 0, "file End", { S( -1 ), qstr2str( this->m_addr ) } );

        star::protocol::ptr pr( new star::protocol( "protocol", prs ) );
        pr->Serialize();
        QString jsonStr = pr->toStr().c_str();
        tcpSocket->write( jsonStr.toUtf8() );

        if ( !tcpSocket->waitForReadyRead() )
        {
            FATAL_STD_STREAM_LOG( g_logger )
            << "%D"
            << "Can not connect to the Master Server: "
            << qstr2str( this->m_master_server_info.addr ) << ":"
            << S( this->m_master_server_info.port ) << " "
            << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
            return;
        }

        jsonStr = tcpSocket->readAll();
        pr->toJson( qstr2str( jsonStr ) );
        pr->Deserialize();
        prs = pr->get_protocol_struct();
        if ( prs.bit == 131 || prs.data == "finish" )
        {
            DEBUG_STD_STREAM_LOG( g_logger )
            << "%D"
            << "Upload File Successfully" << star::Logger::endl();
            ui->status_bar->setText( "INFO: Upload File Successfully, " + path  );
        }
        else
        {
            FATAL_STD_STREAM_LOG( g_logger )
            << "upload file: " << qstr2str( file_name ) << "path: " << qstr2str( path )
            << star::Logger::endl();
            ui->status_bar->setText( "INFO: Upload File Error, " + path );
        }

        tcpSocket->close();
        });
    /* 分离线程 */
    upload_thread.detach();

    QEventLoop loop;
    loop.exec();

    if ( flag )
    {
        this->list();
    }
}

bool MainWindow::append( QString file_name, QString buffer, size_t index, QTcpSocket* tcpSocket )
{
    /* 追加的内容不能太多,只能追加一个chunk的大小 */
    if ( buffer.size() > this->max_chunk_size )
    {
        ERROR_STD_STREAM_LOG( g_logger )
        << "Buffer String is so bug.The size should smaller than max chunk size!"
        << star::Logger::endl();
        return false;
    }

    /* 与 master server 通信 */
    star::protocol::Protocol_Struct ps( 104,
                                  qstr2str( this->m_addr ),
                                  qstr2str( file_name ),
                                        qstr2str( this->m_dir.absolutePath() ),
                                  buffer.size(),
                                  qstr2str( buffer ),
                                  { S( index ), qstr2str( this->m_addr ) } );

   star::protocol::ptr current_protocol( new star::protocol( "upload_file", ps ) );

    current_protocol->set_protocol_struct( ps );
    current_protocol->Serialize();

    QString send_msg = current_protocol->toStr().c_str();

    if ( send_msg[0] != u'{' || send_msg[send_msg.size() - 2] != u'}' )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "Send package Error!, Maybe Package is not Complete!" << star::Logger::endl();
        return false;
    }

    /* 发生块数据 */
    tcpSocket->write( send_msg.toUtf8(), ( current_protocol->toStr().size() + sizeof( std::string ) ) );

    /* 等待服务端的回复 */
    if ( !tcpSocket->waitForReadyRead() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return false;
    }

    /* 接受 master server 的回复 */
    QString buf = tcpSocket->readAll();

    current_protocol->toJson( qstr2str( buf ) );
    current_protocol->Deserialize();
    ps = current_protocol->get_protocol_struct();

    if ( ps.bit == 132 && ps.data == "true" )
    {
        INFO_STD_STREAM_LOG( g_logger )
        << "package: " << S( index ) << " Store Successfully!" << star::Logger::endl();
    }
    else
    {
        ERROR_STD_STREAM_LOG( g_logger )
        << "package: " << S( index ) << " Store Error!" << star::Logger::endl();
        return false;
    }

    return true;
}

star::file_meta_data MainWindow::get_file_meta_data( QString file_name, QString file_path )
{
    /* 询问这个文件的元数据 */
    star::protocol::Protocol_Struct ps(
    101, "", qstr2str( file_name ), qstr2str( this->m_dir.absolutePath() ), 0, "", {} );
    star::protocol::ptr current_protocol( new star::protocol( "get_file_meta_data", ps ) );
    current_protocol->Serialize();
    QString jsonStr = current_protocol->toStr().c_str();

    /* 创建socket */
    QTcpSocket* tcpSocket = new QTcpSocket();
    tcpSocket->setProxy( QNetworkProxy::NoProxy );

    tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                              this->m_master_server_info.port );

    /* 连接 master server */
    if ( !tcpSocket->waitForConnected() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return star::file_meta_data();
    }

    /* 发送询问消息 */
    tcpSocket->write( jsonStr.toUtf8() );

    if ( !tcpSocket->waitForReadyRead() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return star::file_meta_data();
    }

    /* 读取回复 */
    QString buf = tcpSocket->readAll();
    current_protocol->toJson( qstr2str( buf ) );
    current_protocol->Deserialize();

    ps = current_protocol->get_protocol_struct();
    star::file_meta_data temp;

    /* 读取服务端的回复 */
    if ( ps.bit == 107 && ps.data == "File Find!" )
    {
        temp.f_name = ps.file_name;
        temp.f_path = ps.path;
        int i       = 0;
        while ( i < ps.customize.size() )
        {
            star::chunk_meta_data cur;
            cur.from = ps.customize[i];
            i++;
            cur.port = std::stoi( ps.customize[i] );
            i++;
            cur.index = std::stoi( ps.customize[i] );
            i++;
            temp.chunk_list.push_back( cur );
        }
    }
    else
    {
        ERROR_STD_STREAM_LOG( g_logger ) << "ERROR SERVER REPLY!" << star::Logger::endl();
        return star::file_meta_data();
    }

    INFO_STD_STREAM_LOG( g_logger ) << "Get File Meta Data Successfully" << star::Logger::endl();

    /* 关闭连接 */
    tcpSocket->close();

    temp.num_chunk = temp.chunk_list.size();
    return temp;
}



void MainWindow::download()
{
    if ( !this->login() )
    {
        return;
    }
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "Choose File" ) );
    fileDialog->setDirectory( "./" );
    fileDialog->setNameFilter( tr( "File()" ) );
    fileDialog->setFileMode( QFileDialog::ExistingFiles );
    fileDialog->setViewMode( QFileDialog::Detail );

    QString download_path = fileDialog->getExistingDirectory();
    QString file_name = ui->listWidget->currentItem()->text();
    
    /* 获取文件的元数据 */
    star::file_meta_data current_file = this->get_file_meta_data( file_name );

    if ( current_file.chunk_list.empty() )
    {
        WERN_STD_STREAM_LOG( g_logger ) << "The file chunk list is empty!" << star::Logger::endl();
        return;
    }
    
    size_t item_index = ui->process_list->count();
    ui->process_list->addItem( "Download: " + file_name );
    bool flag{ false };

    std::thread download_thread(
    [&]() {
        ui->status_bar->setText( "Downloading file: " + file_name + " ................" );
        QString file_buffer;
        QTcpSocket* tcpSocket = new QTcpSocket();
        tcpSocket->setProxy( QNetworkProxy::NoProxy );

        /* 读取，存储下载断点信息的临时文件 */
        QString temp_file_name = "download-temp-" + file_name;
        QString temp_path = QDir( this->m_dir.absolutePath() ).absoluteFilePath( temp_file_name );
        QFile temp_file( temp_path );
        size_t beg_index         = 0;
        size_t start_chunk_index = 0;
        if ( !temp_file.open( QIODevice::ReadOnly ) )
        {
            beg_index = 0;
        }
        else
        {
            /* 按行读 */

            while ( !temp_file.atEnd() )
            {
                QString buffer = temp_file.readLine();
                qint64 j       = buffer.toLongLong();
                if ( j >= beg_index )
                {
                    beg_index = j;
                }
            }
            temp_file.close();
        }

        if ( !current_file.chunk_list.empty() )
        {
            int i = beg_index;
            int chunk_index
            = beg_index % 67108864; /* 与64mb取余, 如果有余数，除以数据包的大小，计算出数据包的偏移 */
            if ( chunk_index > 0 )
            {
                start_chunk_index = chunk_index / this->max_chunk_size;
            }
            chunk_index = beg_index / 67108864;
            std::vector< star::chunk_meta_data > chunk_list;

            if ( chunk_index > current_file.chunk_list.size() )
            {
                chunk_index = 0;
            }
            for ( size_t i = chunk_index; i < current_file.chunk_list.size(); i++ )
            {
                chunk_list.push_back( current_file.chunk_list[i] );
            }

            for ( auto item : chunk_list )
            {
                star::protocol::Protocol_Struct ps( 110,
                                                    "",
                                                    qstr2str( file_name ),
                                                    qstr2str( this->m_dir.absolutePath() ),
                                                    0,
                                                    "",
                                                    { S( chunk_index ), S( start_chunk_index ) } );
                start_chunk_index++;
                chunk_index++;
                QString chunk_server_addr = item.from.c_str();
                int64_t chunk_server_port = item.port;

                tcpSocket->connectToHost( QHostAddress( chunk_server_addr ), chunk_server_port );

                /* 等待连接成功 */
                if ( !tcpSocket->waitForConnected() )
                {
                    FATAL_STD_STREAM_LOG( g_logger )
                    << "%D"
                    << "Can not connect to the Master Server: "
                    << qstr2str( this->m_master_server_info.addr ) << ":"
                    << S( this->m_master_server_info.port ) << " "
                    << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
                    return;
                }

                star::protocol::ptr current_protocol( new star::protocol( "download_file", ps ) );
                current_protocol->Serialize();
                tcpSocket->write( current_protocol->toStr().c_str(),
                                  current_protocol->toStr().size() );
                tcpSocket->flush();
                size_t k = 0;
                while ( true )
                {
                    QString buf = "";
                    while ( true )
                    {
                        /* 接收数据 */
                        if ( !tcpSocket->waitForReadyRead() )
                        {
                            FATAL_STD_STREAM_LOG( g_logger )
                            << "%D"
                            << "Can not connect to the Master Server: "
                            << qstr2str( this->m_master_server_info.addr ) << ":"
                            << S( this->m_master_server_info.port ) << " "
                            << "Error: " << qstr2str( tcpSocket->errorString() )
                            << star::Logger::endl();
                            return;
                        }

                        buf += tcpSocket->read( this->m_buffer_size * 2 );
                        tcpSocket->flush();

                        /* 判断数据包是不是完全 */
                        if ( buf[0] != '{' || buf[buf.size() - 2] != '}' )
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }

                    star::protocol::Protocol_Struct got_data_buf;
                    star::protocol::ptr data_protocol( new star::protocol( "get_dataa", got_data_buf ) );
                    data_protocol->Serialize();
                    data_protocol->toJson( qstr2str( buf ) );
                    data_protocol->Deserialize();
                    got_data_buf = data_protocol->get_protocol_struct();

                    if ( got_data_buf.bit == 115 )
                    {
                        /* 打开断点信息临时文件，记录断点信息 */
                        if ( !temp_file.open( QIODevice::ReadWrite | QIODevice::Append ) )
                        {
                            FATAL_STD_STREAM_LOG( g_logger )
                            << "Open Temp file Error!" << star::Logger::endl();
                            return;
                        }

                        if ( got_data_buf.data == "chunk end" )
                        {
                            break;
                        }
                        
                        INFO_STD_STREAM_LOG( g_logger ) << "Stare package: " << S( k ) << "Successfully"
                                                        << star::Logger::endl();

                        QString item = this->m_dir.absolutePath() + "download: " + file_name + " "
                                       + S( k ).c_str() + " packages.";
                        ui->process_list->setCurrentRow( item_index );
                        QListWidgetItem* current_item = new QListWidgetItem( item );
                        ui->process_list->setCurrentItem( current_item );

                        //system( "cls" );
                        current_file.f_name = got_data_buf.file_name;
                        current_file.f_path = got_data_buf.path;
                        file_buffer         = got_data_buf.data.c_str();
                        int index           = std::stoi( got_data_buf.customize[0] );
                        /* 追加写入当前的文件块 */
                        QDir folder( download_path );
                        QString write_path = folder.absoluteFilePath( file_name );
                        fileitem fi( write_path, this->max_chunk_size );
                        fi.bwrite_chunk( file_buffer.toUtf8() );
                        /* 向临时文件写入下载断点信息 */
                        std::string break_info = S( i ) + "\n";
                        temp_file.write( break_info.c_str() );
                        ps.reset( 141, "", "", "", 0, "recv data ok", {} );
                        star::protocol::ptr reply_server(
                        new star::protocol( "reply_server", ps ) );
                        reply_server->Serialize();
                        tcpSocket->write( reply_server->toStr().c_str(),
                                          reply_server->toStr().size() );
                        tcpSocket->flush();
                        i += file_buffer.size();
                        temp_file.close();
                    }
                    else
                    {
                        ERROR_STD_STREAM_LOG( g_logger )
                        << "Error Server Reply!" << star::Logger::endl();
                    }
                    k++;
                }
            }
            flag = true;
            tcpSocket->close();
            ui->status_bar->setText( "Downloading file: " + file_name + " Successfully!" );
        }
        else
        {
            flag = true;
            ERROR_STD_STREAM_LOG( g_logger ) << "Not Find the file!" << star::Logger::endl();
        }
    } );

    /* 分离线程 */
    download_thread.detach();

    QEventLoop loog;
    loog.exec();

    if ( flag )
    {
        ui->status_bar->setText( "Download File " + file_name + " ok!" );
    }
}

void MainWindow::movefile()
{
    if ( !this->login() )
    {
        return;
    }
    QFileDialog* fileDialog = new QFileDialog( this );

    fileDialog->setWindowTitle( QStringLiteral( "Choose File" ) );
    fileDialog->setDirectory( "./" );
    fileDialog->setNameFilter( tr( "File()" ) );
    fileDialog->setFileMode( QFileDialog::ExistingFiles );
    fileDialog->setViewMode( QFileDialog::Detail );

    QString move_path = fileDialog->getExistingDirectory();
    QString file_name = ui->listWidget->currentItem()->text();
    star::protocol::Protocol_Struct ps;

    /* 初始化协议内容 */
    ps.bit       = 117;
    ps.file_name = qstr2str( file_name );
    ps.path      = qstr2str( this->m_dir.absolutePath() );
    ps.customize.push_back( qstr2str( move_path ) );
    star::protocol::ptr current_protocol( new star::protocol( "move_file", ps ) );
    current_protocol->Serialize();

    /* 与 master server 建立连接 */
    QTcpSocket* tcpSocket = new QTcpSocket();
    tcpSocket->setProxy( QNetworkProxy::NoProxy );
    tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                              this->m_master_server_info.port );

    /* 等待连接成功 */
    if ( !tcpSocket->waitForConnected() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return;
    }

    tcpSocket->write( current_protocol->toStr().c_str(), current_protocol->toStr().size() );
    tcpSocket->flush();

    /* 接收数据 */
    if ( !tcpSocket->waitForReadyRead() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return;
    }

    QString buffer = tcpSocket->readAll();
    tcpSocket->flush();
    current_protocol->toJson( qstr2str( buffer ) );
    current_protocol->Deserialize();
    ps = current_protocol->get_protocol_struct();

    /* 判断是否接受成功 */
    if ( ps.bit == 125 && ps.data == "ok" )
    {
        /* 在本地文件系统移动文件 */
        QString old_name = this->m_dir.absolutePath();
        QString new_name = move_path;
        bool x           = QFile::rename( old_name, new_name );
        if ( !x )
        {
            ERROR_STD_STREAM_LOG( g_logger )
            << "Move File in local File system Error!" << star::Logger::endl();
        }

        INFO_STD_STREAM_LOG( g_logger ) << "move file successfully" << star::Logger::endl();
        tcpSocket->close();
        return;
    }

    tcpSocket->close();
    ERROR_STD_STREAM_LOG( g_logger ) << "move file Error!" << star::Logger::endl();
}

void MainWindow::rename()
{
     if ( !this->login() )
    {
        return;
    }
    bool flag           = true;
    QString rename_name = QInputDialog::getText(
    nullptr, tr( "File rename" ), tr( "Input file name:" ), QLineEdit::Normal, tr( "new_name" ), &flag );
    QString file_name = ui->listWidget->currentItem()->text();
    star::protocol::Protocol_Struct ps;

    /* 初始化协议内容 */
    ps.bit       = 134;
    ps.file_name = qstr2str( file_name );
    ps.path      = qstr2str( this->m_dir.absolutePath() );
    ps.customize.push_back( qstr2str( rename_name ) );
    star::protocol::ptr current_protocol( new star::protocol( "rename_file", ps ) );
    current_protocol->Serialize();

    /* 与 master server 建立连接 */
    QTcpSocket* tcpSocket = new QTcpSocket();
    tcpSocket->setProxy( QNetworkProxy::NoProxy );
    tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                              this->m_master_server_info.port );

    /* 等待连接成功 */
    if ( !tcpSocket->waitForConnected() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return;
    }

    tcpSocket->write( current_protocol->toStr().c_str(), current_protocol->toStr().size() );
    tcpSocket->flush();

    /* 接收数据 */
    if ( !tcpSocket->waitForReadyRead() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return;
    }

    QString buffer = tcpSocket->readAll();
    tcpSocket->flush();
    current_protocol->toJson( qstr2str( buffer ) );
    current_protocol->Deserialize();
    ps = current_protocol->get_protocol_struct();

    /* 判断是否接受成功 */
    if ( ps.bit == 140 && ps.data == "ok" )
    {
        /* 在本地文件系统移动文件 */
        QString old_name = this->m_dir.absolutePath();
        QString new_name = QDir( this->m_dir.absolutePath() ).absoluteFilePath( rename_name );
        bool x           = QFile::rename( old_name, new_name );
        if ( !x )
        {
            ERROR_STD_STREAM_LOG( g_logger )
            << "Rename File in local File system Error!" << star::Logger::endl();
        }

        INFO_STD_STREAM_LOG( g_logger ) << "Rename file successfully" << star::Logger::endl();
        tcpSocket->close();
        return;
    }

    tcpSocket->close();
    ERROR_STD_STREAM_LOG( g_logger ) << "Rename file Error!" << star::Logger::endl();

    this->list();
}

void MainWindow::del()
{
    QString file_name = ui->listWidget->currentItem()->text();
    ui->status_bar->setText( "INFO: Remove file: " + file_name + "..............." );
    /* 与 master server 通信，删除元数据 */
    QTcpSocket* tcpSocket = new QTcpSocket();
    tcpSocket->setProxy( QNetworkProxy::NoProxy );
    tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                              this->m_master_server_info.port );
    if ( !tcpSocket->waitForConnected() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return;
    }

    star::protocol::Protocol_Struct ps( 135,
                                        qstr2str( this->m_addr ),
                                        qstr2str( file_name ),
                                        qstr2str( this->m_dir.absolutePath() ),
                                        0,
                                        "",
                                        {} );

    star::protocol::ptr cur_protocol( new star::protocol( "delete_file_meta_data", ps ) );
    cur_protocol->Serialize();
    tcpSocket->write( cur_protocol->toStr().c_str(), cur_protocol->toStr().size() );

    if ( !tcpSocket->waitForReadyRead() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return;
    }

    QString buf = tcpSocket->readAll();

    cur_protocol->toJson( qstr2str( buf ) );
    cur_protocol->Deserialize();
    ps = cur_protocol->get_protocol_struct();

    if ( ps.bit == 138 && ps.data == "true" )
    {
        INFO_STD_STREAM_LOG( g_logger ) << "Delete File Successfully!" << star::Logger::endl();
    }
    else
    {
        FATAL_STD_STREAM_LOG( g_logger ) << "Delete File Error!" << star::Logger::endl();
    }

    ui->status_bar->setText( "INFO: Remove file: " + file_name + " ssucceefully!" );
    this->list();
    return;
}

void MainWindow::mkdir()
{
    if ( !this->login() )
    {
        return;
    }
    bool flag;
    QString dir_name = QInputDialog::getText(
    nullptr, tr( "Make new Dir" ), tr( "Input file name:" ), QLineEdit::Normal, tr( "new_dir" ), &flag );

    QString path = QDir( this->m_dir.absolutePath() ).absoluteFilePath( dir_name );
    DEBUG_STD_STREAM_LOG( g_logger ) << qstr2str( path ) << star::Logger::endl();
    /* 创建一个文件夹 */
    QDir dir( path );
    if ( !dir.exists() )
    {
        bool ismkdir = dir.mkdir( path );
        if ( !ismkdir )
        {
            FATAL_STD_STREAM_LOG( g_logger ) << "Create path fail" << star::Logger::endl();
            return;
        }
        else
        {
            INFO_STD_STREAM_LOG( g_logger ) << "Create fullpath success" << star::Logger::endl();
        }
    }
    else
    {
        FATAL_STD_STREAM_LOG( g_logger ) << "fullpath exist" << star::Logger::endl();
        return;
    }
}

void MainWindow::in_dir() 
{
    QString dir_name = ui->listWidget->currentItem()->text();
    if ( dir_name == "." )
    {
        return;
    }
    else if ( dir_name == ".." )
    {
        if ( this->m_dir.cdUp() )
        {
            this->list();
            return;
        }
        else
        {
            return;
        }
    }
    else
    {
        this->m_dir.cd(dir_name );
    };
    this->list();
}

bool MainWindow::login( QString user_name , QString user_password )
{
    INFO_STD_STREAM_LOG( g_logger ) << "%D"
                                    << "login to the master server"
                                    << "%s%n";
    if ( user_name == "UNKNOW" )
    {
        user_name = this->m_user.user_name;
    }
    if ( user_password == "UNKNOW" )
    {
        user_password = this->m_user.user_pwd;
    }

    star::protocol::Protocol_Struct ps(
    118, "", qstr2str( user_name ), qstr2str( user_password ), 0, "", {} );
    star::protocol::ptr current_protocol( new star::protocol( "user_login", ps ) );
    current_protocol->Serialize();
    QString jsonStr = current_protocol->toStr().c_str();

    /* 与 master server 通信 */
    QTcpSocket* tcpSocket = new QTcpSocket();
    tcpSocket->setProxy( QNetworkProxy::NoProxy );
    tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                              this->m_master_server_info.port );
    if ( !tcpSocket->waitForConnected() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return false;
    }
    tcpSocket->write( jsonStr.toUtf8(), jsonStr.size() );

    if ( !tcpSocket->waitForReadyRead() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return false;
    }

    /* 读取回复 */
    QString buffer = tcpSocket->readAll();
    tcpSocket->close();

    current_protocol->toJson( qstr2str( buffer ) );
    current_protocol->Deserialize();
    ps = current_protocol->get_protocol_struct();

    if ( ps.bit == 120 && ps.data == "true" )
    {
        INFO_STD_STREAM_LOG( g_logger ) << "User authentication succeeded." << star::Logger::endl();
        return true;
    }

    ERROR_STD_STREAM_LOG( g_logger ) << "User authentication failed." << star::Logger::endl();

    return false;
}

void MainWindow::regist()
{
    ui->status_bar->setText( "INFO: Reset user authentication information ..........." );
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
    star::protocol::Protocol_Struct ps;
    ps.bit       = 119;
    ps.file_name = qstr2str( user_name );
    ps.path      = qstr2str( user_pwd );
    star::protocol::ptr current_protocol( new star::protocol( "reset_info", ps ) );
    current_protocol->Serialize();
    QString jsonStr = current_protocol->toStr().c_str();
    /* 与 master server 通信 */
    QTcpSocket* tcpSocket = new QTcpSocket();
    tcpSocket->setProxy( QNetworkProxy::NoProxy );
    tcpSocket->connectToHost( this->m_master_server_info.addr, this->m_master_server_info.port );

    /* 等待连接成功 */
    if ( !tcpSocket->waitForConnected() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return ;
    }

    INFO_STD_STREAM_LOG( g_logger ) << "Connect Master Server Successfully!" << star::Logger::endl();
    tcpSocket->write( jsonStr.toUtf8(), jsonStr.size() );
    tcpSocket->waitForBytesWritten();

    /* 等待服务端的回复 */
    if ( !tcpSocket->waitForReadyRead() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return ;
    }

    QString buffer = tcpSocket->readAll();
    current_protocol->toJson( qstr2str( buffer ) );
    current_protocol->Deserialize();
    ps = current_protocol->get_protocol_struct();
    tcpSocket->close();

    if ( ps.bit == 121 && ps.data == "true" )
    {
        ui->status_bar->setText(
        "INFO: User authentication password reset succeeded." );
        return ;
    }

     ui->status_bar->setText( "User authentication password reset failed." );

    return ;
}

void MainWindow::list()
{
    if ( !this->login() )
    {
        return;
    }
    QString path = this->m_dir.absolutePath();
    
    /* 初始化协议 */
    star::protocol::Protocol_Struct ps;
    ps.bit = 126;
    star::protocol::ptr current_protocol( new star::protocol( "get_file_info", ps ) );
    current_protocol->Serialize();
    QString jsonStr = current_protocol->toStr().c_str();

    /* 与 master server 通信 */
    QTcpSocket* tcpSocket = new QTcpSocket();

    tcpSocket->setProxy( QNetworkProxy::NoProxy );

    tcpSocket->connectToHost( QHostAddress( this->m_master_server_info.addr ),
                              this->m_master_server_info.port );

    /* 等待连接成功 */
    if ( !tcpSocket->waitForConnected() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return;
    }

    tcpSocket->write( jsonStr.toUtf8(), jsonStr.size() );

    /* 等待服务端的回复 */
    if ( !tcpSocket->waitForReadyRead() )
    {
        FATAL_STD_STREAM_LOG( g_logger )
        << "%D"
        << "Can not connect to the Master Server: " << qstr2str( this->m_master_server_info.addr )
        << ":" << S( this->m_master_server_info.port ) << " "
        << "Error: " << qstr2str( tcpSocket->errorString() ) << star::Logger::endl();
        return;
    }

    /* 读取回复 */
    QString buffer = tcpSocket->readAll();
    tcpSocket->close();

    current_protocol->toJson( qstr2str( buffer ) );
    current_protocol->Deserialize();
    ps = current_protocol->get_protocol_struct();

    QStringList files;
    /* 读取元数据 */
    if ( ps.bit == 127 )
    {
        int i = 0;
        while ( i < ps.customize.size() )
        {
            QString temp_file_name = ps.customize[i].c_str();
            i++;
            QString temp_file_path = ps.customize[i].c_str();
            i++;
            if ( temp_file_path == path )
            {
                files << temp_file_name;
            }
            else
            {
                continue;
            }
        }
    }
    ui->listWidget->clear(); /* 清空列表中的内容 */
    QFileInfoList list = this->m_dir.entryInfoList();
    QStringList dirs;
    for ( auto item : list )
    {
        if ( item.isDir() )
        {
            dirs << item.fileName();
        }
    }
    ui->listWidget->addItems( dirs );
    for ( auto item : files )
    {
        if ( item.indexOf( "copy-" ) == -1 )
        {
            ui->listWidget->addItem( item );
        }
    }
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
    return cmd_line;
}

MainWindow::fileitem::fileitem( QString path, size_t chunk_size )
{
    this->m_path       = path;
    this->m_chunk_size = chunk_size;
}

bool MainWindow::fileitem::bread_chunk( size_t& index, QByteArray& buffer )
{
    QFile file( this->m_path );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        FATAL_STD_STREAM_LOG( g_logger ) << "Open File Error!" << star::Logger::endl();
        return false;
    }

    qint64 file_size = file.size(); /* 获取文件的大小 */
    if ( index >= file_size )
    {
        WERN_STD_STREAM_LOG( g_logger )
        << "The end of the file has been reached.!" << star::Logger::endl();
        return false;
    }

    file.seek( index ); /* 定位到index的位置开始读 */

    /* 读一个块 */
    if ( file_size < this->m_chunk_size )
    {
        buffer = file.read( file_size );
        index += file_size;
    }
    else
    {
        buffer = file.read( this->m_chunk_size );
        index += this->m_chunk_size;
    }

    file.close();
    return true;
}

bool MainWindow::fileitem::bwrite_chunk( QByteArray buffer )
{
    QFile file( this->m_path );
    if ( !file.open( QIODevice::ReadWrite | QIODevice::Append ) )
    {
        FATAL_STD_STREAM_LOG( g_logger ) << "Open File Error!" << star::Logger::endl();
        return false;
    }
    file.write( buffer );

    file.close();
    return true;
}

bool txtFileitem::readAll( QString& buffer )
{
    QFile file( this->m_path );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        FATAL_STD_STREAM_LOG( g_logger ) << "Open File Error!" << star::Logger::endl();
        return false;
    }

    buffer = file.readAll();

    file.close();
    return true;
}

qint64 MainWindow::fileitem::size()
{
    QFile file( this->m_path );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        FATAL_STD_STREAM_LOG( g_logger ) << "Open File Error!"
                                         << "%n%0";
        return -1;
    }
    return file.size();
}