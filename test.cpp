#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include <iostream>


using std::cout;
using std::endl;
using std::string;
using boost::asio::ip::tcp;


class CMyTcpConnection
    : public boost::enable_shared_from_this<CMyTcpConnection>
{
public:
    CMyTcpConnection(boost::asio::io_service &ser)
        :m_nSocket(ser)
    {
    }
    typedef boost::shared_ptr<CMyTcpConnection> CPMyTcpCon;


    static CPMyTcpCon CreateNew(boost::asio::io_service& io_service)
    {
        return CPMyTcpCon(new CMyTcpConnection(io_service));
    }


   
public:
    void start()
    {
        for (int i = 0; i != 100; ++i)
        {
            boost::shared_ptr<string> pStr(new string);
            *pStr = boost::lexical_cast<string>(boost::this_thread::get_id());
            *pStr += "\r\n";
            boost::asio::async_write(m_nSocket,boost::asio::buffer(*pStr),
                boost::bind(&CMyTcpConnection::HandleWrite,shared_from_this(),
                 boost::asio::placeholders::error,
                 boost::asio::placeholders::bytes_transferred,
                 pStr,i)
                );
        }
    }
    tcp::socket& socket()
    {
        return m_nSocket;
    }
private:
    void HandleWrite(const boost::system::error_code& error
        ,std::size_t bytes_transferred
        ,boost::shared_ptr<string> pStr,int nIndex)
    {
        if (!error)
        {
            boost::mutex::scoped_lock lk(m_ioMutex);
            cout << "发送序号=" << nIndex << ",线程id=" << boost::this_thread::get_id() << endl;
        }
        else
        {
            cout << "连接断开" << endl;
        }
    }
private:
    tcp::socket m_nSocket;
    boost::mutex m_ioMutex;
};


class CMyService
    : private boost::noncopyable
{
public:
    CMyService(string const &strIP,string const &strPort,int nThreads)
        :m_tcpAcceptor(m_ioService)
        ,m_nThreads(nThreads)
    {
        tcp::resolver resolver(m_ioService);
        tcp::resolver::query query(strIP,strPort);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
        m_tcpAcceptor.open(endpoint.protocol());
        m_tcpAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_tcpAcceptor.bind(endpoint);
        m_tcpAcceptor.listen();


        StartAccept();
    }
    ~CMyService(){Stop();}
public:
    void Stop() 
    { 
        m_ioService.stop();
        for (std::vector<boost::shared_ptr<boost::thread> >::const_iterator it = m_listThread.begin();
            it != m_listThread.end(); ++ it)
        {
            (*it)->join();
        }
    }
    void Start()
    {
        for (int i = 0; i != m_nThreads; ++i)
        {
            boost::shared_ptr<boost::thread> pTh(new boost::thread(
                boost::bind(&boost::asio::io_service::run,&m_ioService)));
            m_listThread.push_back(pTh);
        }
    }
private:
    void HandleAccept(const boost::system::error_code& error
        ,boost::shared_ptr<CMyTcpConnection> newConnect)
    {
        if (!error)
        {
            newConnect->start();
        }
        StartAccept();
    }


    void StartAccept()
    {
        CMyTcpConnection::CPMyTcpCon newConnect = CMyTcpConnection::CreateNew(m_tcpAcceptor.get_io_service());
        m_tcpAcceptor.async_accept(newConnect->socket(),
            boost::bind(&CMyService::HandleAccept, this,
            boost::asio::placeholders::error,newConnect));
    }
private:
    boost::asio::io_service m_ioService;
    boost::asio::ip::tcp::acceptor m_tcpAcceptor;
    std::vector<boost::shared_ptr<boost::thread> > m_listThread;
    std::size_t m_nThreads;
};


int main(int argc, char* argv[])
{
    try
    {
        if (argc != 4)
        {
            std::cerr << "<IP> <port> <threads>\n";
            return 1;
        }
        int nThreads = boost::lexical_cast<int>(argv[3]);
        CMyService mySer(argv[1],argv[2],nThreads);
        mySer.Start();
        getchar();
        mySer.Stop();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
