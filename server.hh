#ifndef __SERVER_H__
#define __SERVER_H__

#include <iostream>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <thread>
#include <mutex>

#include <string>
#include <map>
#include <cstring>
#include <unistd.h>

#include <chrono>

#define PORT 4390
#define MAX_CLIENT 20

std::mutex mtx;

struct client_info
{
    int client_socket;
    sockaddr_in client_addr;
};

class MyServer
{
private:
    int server_socket;
    sockaddr_in server_addr;
    std::map<int, struct client_info> client_list;
    int client_list_use[20];
    bool _is_server_down = false;

public:
    MyServer();
    ~MyServer();

    void _server_on();

    void add_client(int list_id, int client_socket, sockaddr_in client_addr);
    bool client_list_empty();
    void rst_client_list(int list_id);
    int get_server_socket();
    int get_list_id();
    void set_is_server_down();
    bool is_server_down();
    std::string print_list();
    int find_socket(int list_id);
};

struct thread_info
{
    int list_id;
    int client_socket;
    sockaddr_in client_addr;
    MyServer *server;
};

MyServer _server;

void *process_client(void *thread_info);
void get_connect(int client_socket);
void get_time(int client_socket);
void get_name(int client_socket);
void get_list(int client_socket, MyServer server);
void send_message(int client_socket, int client_id, int dst_id, std::string message, MyServer server);
void stop_connect(int client_socket);

#endif