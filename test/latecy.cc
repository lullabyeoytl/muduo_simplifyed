#include "../TcpConnection.hpp"
#include "../EventLoop.hpp"
#include "../TcpServer.hpp"
void onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        // printf("new connection\n");
    }
}

void onMessage(const TcpConnectionPtr& conn,
               Buffer* buf,
               Timestamp)
{
    buf->retrieveAll();

    static std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n"
        "Hello World";

    conn->send(response);
}

int main()
{
    EventLoop loop;
    InetAddress addr(8080);
    TcpServer server(&loop, addr, "TestServer",TcpServer::kReusePort);

    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setThreadNum(12);

    server.start();
    loop.loop();
}