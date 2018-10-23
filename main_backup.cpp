#include <stdio.h>

//#include "AuthenHandle.h"
//#include "configure.h"
//#include "NetSocketCommand.h"



#ifdef WIN32 //for windows nt/2000/xp


 
//#include "gelsserver.h"
#pragma comment(lib,"Ws2_32.lib")
#include <windows.h>
#else         //for unix



#include <pthread.h>
#include <sys/socket.h>
//    #include <sys/types.h>


//    #include <sys/signal.h>


//    #include <sys/time.h>


#include <netinet/in.h>     //socket


//    #include <netdb.h>


#include <unistd.h>            //gethostname


// #include <fcntl.h>


#include <arpa/inet.h>


#include <string.h>            //memset


typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
#ifdef M_I386
typedef int socklen_t;
#endif


#define BOOL             int
#define INVALID_SOCKET    -1
#define SOCKET_ERROR     -1
#define TRUE             1
#define FALSE             0
#endif        //end #ifdef WIN32


static int count111 = 0;
static time_t oldtime = 0, nowtime = 0;


#include <list>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>


const int server_port = 6768;            //服务器启动的端口;
const int server_thread_pool_num = 2;    //服务器启动线程池的线程数;

//typedef boost::shared_ptr<boost::asio::io_service> io_service_sptr;
//typedef boost::shared_ptr<boost::asio::io_service::work> work_sptr;
//typedef boost::shared_ptr<boost::thread> thread_sptr;

//boost::asio::io_service m_io;
//io_service_sptr io_service(new  boost::asio::io_service);

using namespace std;
using boost::asio::ip::tcp;


#ifdef _LINUX
pthread_mutex_t listLock;
#endif
#ifdef WIN32
CRITICAL_SECTION  listLock;
#endif

char szBuff[256] = {0} ;


int   nConnectCount = 0 ;


map<int, int> g_mapThreadId;  //线程ID 映射;


bool InsertMapThreadId(int nThreadId)
{
map<int, int>::iterator mapThreadIdIt = g_mapThreadId.find(nThreadId);
if (mapThreadIdIt == g_mapThreadId.end())
{
//没有找到插入并返回true；
g_mapThreadId.insert( std::make_pair(nThreadId, g_mapThreadId.size()+1) );
return true;
}
else
{
//已经存在不插入返回false
return false;
}
}


class io_service_pool
/*: public boost::noncopyable*/
{
public:


explicit io_service_pool(std::size_t pool_size)
: next_io_service_(0)
{ 

for (std::size_t i = 0; i < pool_size; ++ i)
{
io_service_sptr io_service(new boost::asio::io_service);
work_sptr work(new boost::asio::io_service::work(*io_service));
io_services_.push_back(io_service);
work_.push_back(work);
}


/*
work_sptr work(new boost::asio::io_service::work(*io_service));
//io_services_.push_back(&m_io);
work_.push_back(work);
}*/

}


void start()
{ 

for (std::size_t i = 0; i < io_services_.size(); ++ i)
{
boost::shared_ptr<boost::thread> thread(new boost::thread(
boost::bind(&boost::asio::io_service::run, io_services_[i])));
threads_.push_back(thread);
}

/*boost::shared_ptr<boost::thread> thread(new boost::thread(
boost::bind(&boost::asio::io_service::run, io_service)));
threads_.push_back(thread);*/

}


void join()
{
for (std::size_t i = 0; i < threads_.size(); ++ i)
{
//threads_[i]->join();
threads_[i]->timed_join(boost::posix_time::seconds(1));
} 
}


void stop()
{ 
for (std::size_t i = 0; i < io_services_.size(); ++ i)
{
io_services_[i]->stop();
}

 //io_service->stop();
}


boost::asio::io_service& get_io_service()
{
boost::mutex::scoped_lock lock(mtx);
boost::asio::io_service& io_service = *io_services_[next_io_service_];
++ next_io_service_;
if (next_io_service_ == io_services_.size())
{
next_io_service_ = 0;
}
return io_service;
}


private:
typedef boost::shared_ptr<boost::asio::io_service> io_service_sptr;
typedef boost::shared_ptr<boost::asio::io_service::work> work_sptr;
typedef boost::shared_ptr<boost::thread> thread_sptr;


boost::mutex mtx;


std::vector<io_service_sptr> io_services_;
std::vector<work_sptr> work_;
std::vector<thread_sptr> threads_; 
std::size_t next_io_service_;
boost::thread_group threads;
};


boost::mutex cout_mtx;
int packet_size = 0;
enum {MAX_PACKET_LEN = 4096};


class session
{
public:
session(boost::asio::io_service& io_service)
: socket_(io_service)
, recv_times(0)
{


bDeleteFlag = FALSE ;
memset(data_,0x00,sizeof(data_));


}


virtual ~session()
{
boost::mutex::scoped_lock lock(cout_mtx);
socket_.close() ;
nConnectCount -- ;
}


tcp::socket& socket()
{
return socket_;
}


//暂时不需要这个函数
inline void requestRead()
{
socket_.async_read_some(boost::asio::buffer(data_,MAX_PACKET_LEN ),//
boost::bind(&session::handle_read, this,
boost::asio::placeholders::error,
boost::asio::placeholders::bytes_transferred));
}



void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
if (!error)
{
if(bytes_transferred > 0)
{
sendData(data_,bytes_transferred);


}
    
requestRead() ;
}
else
{
bDeleteFlag = TRUE;
//socket_.close() ;
nConnectCount -- ;
}
}


BOOL sendData(char* szData,int nLength)
{
boost::asio::ip::tcp::endpoint  endpoint1 = socket_.remote_endpoint();

#ifdef WIN32

         int nThreadID = GetCurrentThreadId();

#else

         int nThreadID = pthread_self();

#endif

//int nThreadID = ::GetCurrentThreadId();
InsertMapThreadId(nThreadID);
printf("in    socket:%d  remoteip:%s threadId:%lld 0x:%x theadIdnum:%d ", socket_.remote_endpoint().port(), socket_.remote_endpoint().address().to_string().c_str() , nThreadID, nThreadID) ;
printf("threadNum:%d \r\n", g_mapThreadId.size());

if(bDeleteFlag || szData == NULL || nLength <= 0 )
return FALSE ;


boost::asio::async_write(socket_, boost::asio::buffer(szData, nLength),
boost::bind(&session::handle_write, this, boost::asio::placeholders::error));


        return TRUE ;
}


void handle_write(const boost::system::error_code& error)
{
//int nThreadID = GetCurrentThreadId();

#ifdef WIN32

         int nThreadID = GetCurrentThreadId();

#else

         int nThreadID = pthread_self();

#endif
InsertMapThreadId(nThreadID);
printf("write socket:%d  remoteip:%s threadId:%lld 0x:%x  ", socket_.remote_endpoint().port(), socket_.remote_endpoint().address().to_string().c_str() , nThreadID, nThreadID) ;
printf("threadNum:%d \r\n", g_mapThreadId.size());
if (!error)
{//写入正确
 
}
else
{
bDeleteFlag = TRUE;
//socket_.close() ;
nConnectCount -- ;
}
}


public:
BOOL            bDeleteFlag ;


private:
tcp::socket     socket_;
char            data_[MAX_PACKET_LEN];
int             recv_times;
};


typedef list<session* >  SessionList ;
SessionList              sessionList ;
class server
{
public:
server(short port, int thread_cnt)
: io_service_pool_(thread_cnt)
, acceptor_(io_service_pool_.get_io_service(), tcp::endpoint(tcp::v4(), port))
{
session* new_session = new session(io_service_pool_.get_io_service());
acceptor_.async_accept(new_session->socket(),
boost::bind(&server::handle_accept, this, new_session, boost::asio::placeholders::error));


	#ifdef _LINUX
	pthread_mutex_lock(&listLock);
    sessionList.push_back(new_session) ;
	#endif

	#ifdef _WIN32
      EnterCriticalSection(&listLock);
    sessionList.push_back(new_session) ;
      LeaveCriticalSection(&listLock); 
	#endif 
      //EnterCriticalSection(&listLock);
//sessionList.push_back(new_session) ;
      //LeaveCriticalSection(&listLock);   
}


void handle_accept(session* new_session, const boost::system::error_code& error)
{
if (!error)
{
//new_session->readRequest(Packet_Is_Head,sizeof(PacketHead)); //先请求包头
   new_session->requestRead() ;
   nConnectCount ++ ;
}
else
{
            new_session->bDeleteFlag = TRUE ;
}


  new_session = new session(io_service_pool_.get_io_service());
acceptor_.async_accept(new_session->socket(),
boost::bind(&server::handle_accept, this, new_session, boost::asio::placeholders::error));

	#ifdef _LINUX
	pthread_mutex_lock(&listLock);
    sessionList.push_back(new_session) ;
  int nThreadID = pthread_self();
  printf("链接数量 %d  threadId:%lld 0x:%x \r\n",nConnectCount,  nThreadID, nThreadID) ;
	#endif

	#ifdef _WIN32
      EnterCriticalSection(&listLock);
    sessionList.push_back(new_session) ;
      LeaveCriticalSection(&listLock); 

         int nThreadID = GetCurrentThreadId();
  printf("链接数量 %d  threadId:%lld 0x:%x \r\n",nConnectCount,  nThreadID, nThreadID) ;
	#endif  



 
}


void run()
{
io_service_pool_.start();
io_service_pool_.join();
}


private:


io_service_pool io_service_pool_;
tcp::acceptor acceptor_;
};


int main()
{
  //boost

#ifdef _LINUX
pthread_mutex_init(&listLock, NULL);
#endif
#ifdef WIN32
InitializeCriticalSection(&listLock) ;
#endif

printf("server run! server port :%d thread_poo_num:%d \n", server_port, server_thread_pool_num);


  //创建线程数量，要先检测CPU线程数量，然后再创建相应的线程数
server svr(server_port, server_thread_pool_num);
svr.run();



while(true)
{
#ifdef _LINUX
sleep(1);
#endif

#ifdef WIN32
 Sleep(1);
#endif
}

#ifdef _LINUX
pthread_mutex_destroy(&listLock);
#endif
#ifdef WIN32
   DeleteCriticalSection(&listLock);
#endif


   printf("server end\n ");




   return 0;
}
