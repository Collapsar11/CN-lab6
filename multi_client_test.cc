// multi_client_test.cc
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "my_packet.hh"
#include <cstring>

#define CLIENT_COUNT 3
#define MAXSIZE 1024
const char *SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 4390;

int tcp_socket;
bool is_connected = false;
bool message_received = false;
bool waiting_for_send_response = false;
std::mutex console_mutex;
std::condition_variable cv;
int time_response_count = 0;
const int TOTAL_TIME_REQUESTS = 100;
std::mutex count_mutex;

void recv_msg_child_thread()
{
    while (is_connected)
    {
        char buffer[MAXSIZE];
        memset(buffer, 0, sizeof(buffer));

        ssize_t recv_size = recv(tcp_socket, buffer, sizeof(buffer) - 1, 0);
        if (recv_size <= 0)
        {
            is_connected = false;
            std::cout << "\n\033[31mConnection closed\033[0m" << std::endl;
            break;
        }

        MyPacket packet = parsePacket(buffer);
        {
            std::lock_guard<std::mutex> lock(console_mutex);

            if (packet.get_type() == REQ_TYPE::TIME)
            {
                std::cout << "\n\033[35m(Server->Client[" << getpid() << "])\033[0m "
                          << packet.get_message() << std::endl;
                {
                    std::lock_guard<std::mutex> count_lock(count_mutex);
                    time_response_count++;
                    if (time_response_count == TOTAL_TIME_REQUESTS)
                    {
                        message_received = true;
                        cv.notify_one();
                    }
                }
            }
        }
    }

    if (!is_connected)
        close(tcp_socket);
}

void run_client()
{
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(tcp_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != -1)
    {
        std::cout << "\033[32m(Console)\033[0m Client[" << getpid() << "] connected!" << std::endl;
        is_connected = true;

        std::thread t(recv_msg_child_thread);
        t.detach();

        time_response_count = 0;
        std::cout << "\033[32m(Console)\033[0m Client[" << getpid() << "] sending "
                  << TOTAL_TIME_REQUESTS << " time requests..." << std::endl;

        auto start_time = std::chrono::steady_clock::now();

        for (int i = 0; i < TOTAL_TIME_REQUESTS; i++)
        {
            MyPacket packet;
            packet.set_packet(REQ_TYPE::TIME);
            std::string str = packet.packet_to_string();
            if (send(tcp_socket, str.c_str(), str.size(), 0) == -1)
            {
                std::cout << "\033[31mSend failed:" << strerror(errno) << "\033[0m" << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        {
            std::unique_lock<std::mutex> lock(console_mutex);
            cv.wait_for(lock, std::chrono::seconds(10), []
                        { return message_received || !is_connected; });
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << "\033[32m(Console)\033[0m Client[" << getpid()
                  << "] received " << time_response_count << " out of "
                  << TOTAL_TIME_REQUESTS << " responses in "
                  << duration.count() << "ms" << std::endl;
    }
    else
    {
        std::cout << "\033[31mConnection failed for client " << getpid() << "\033[0m" << std::endl;
    }

    close(tcp_socket);
}

int main()
{
    std::vector<pid_t> client_pids;

    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            run_client();
            exit(0);
        }
        else if (pid > 0)
        {
            client_pids.push_back(pid);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else
        {
            std::cerr << "Fork failed!" << std::endl;
        }
    }

    for (pid_t pid : client_pids)
    {
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}