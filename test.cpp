#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define PORT 8080
#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096

// 设置非阻塞
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

    set_nonblocking(server_fd);

    // 创建 epoll
    int epoll_fd = epoll_create1(0);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    epoll_event events[MAX_EVENTS];

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            // 新连接
            if (fd == server_fd) {
                sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);

                set_nonblocking(client_fd);

                epoll_event client_ev{};
                client_ev.events = EPOLLIN | EPOLLET; // 边缘触发
                client_ev.data.fd = client_fd;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);

                std::cout << "New client connected\n";
            }
            else {
                char buffer[BUFFER_SIZE];
                while (true) {
                    ssize_t bytes = read(fd, buffer, BUFFER_SIZE);

                    if (bytes <= 0) break;

                    write(fd, buffer, bytes); // echo
                }
            }
        }
    }

    close(server_fd);
    return 0;
}