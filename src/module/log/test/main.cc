#include "../log.h"

#include <string>

int main()
{
    star::LogManager manager;
    star::Logger::ptr Logger( new star::Logger );
    manager.add_logger( Logger );

    DEBUG_STD_STREAM_LOG( Logger ) << "<< DEBUG >>"
                                            << " "
                                            << "Hello World%n"
                                            << "%0";

    Logger->set_appender( star::Logger::Appender::STD, star::LogLevel::level::DOTKNOW );
    Logger->format( "%p %f %l %m %n", { "Error", "log.cc", "11", "RUN Error!" } );

    DEBUG_FILE_STREAM_LOG( Logger ) << "<< DEBUG >>"
                                             << " "
                                             << "Hello World"
                                             << "%0";

    Logger->set_appender( star::Logger::Appender::FILE, star::LogLevel::level::DOTKNOW );
    Logger->format( "%p %f %l %m %n", { "Error", "log.cc", "11", "RUN Error!" } );

    ALL_LOG_TO_FILE(manager);
    
    return 0;
}