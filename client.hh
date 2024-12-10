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

bool is_connected = false;

int tcp_socket;

std::mutex console_mutex;

#endif