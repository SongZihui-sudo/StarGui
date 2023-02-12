#include <ctime>
#include <iomanip>
#include <sstream>

#include "./common.h"

std::thread::id GetThreadId( ) { return std::this_thread::get_id( ); }

std::uint64_t getTime( )
{
    auto now   = std::chrono::system_clock::now( );
    auto now_c = std::chrono::system_clock::to_time_t( now );
    std::ostringstream ss;
    ss << std::put_time( localtime( &now_c ), "%Y%m%d%H%M" );
    std::string timeid = ss.str( );
    std::uint64_t ret = strtoll( timeid.c_str( ), nullptr, 0 ); /* 获取到的时间为世界时相比于北京时间小八个小时 */
    return ret;
}

int get_length( long x )
{
    int leng = 1;
    while ( x > 10 )
    {
        x /= 10;
        leng++;
    }
    return leng;
}