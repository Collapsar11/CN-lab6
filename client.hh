#ifndef __CLIENT_H__
#define __CLIENT_H__

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
#include <condition_variable>
#define TEST 0

bool is_connected = false;

int tcp_socket;

std::mutex console_mutex;
std::condition_variable cv;
bool message_received = false;
bool waiting_for_send_response = false;

int time_response_count = 0;
const int TOTAL_TIME_REQUESTS = 100;
std::mutex count_mutex;

int print_menu();
void recv_msg_child_thread();
std::pair<std::string, unsigned int> getIPandPort();
std::pair<std::string, unsigned int> getIDandMsg();

#endif