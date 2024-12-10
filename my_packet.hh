#ifndef __MY_PACKET_H__
#define __MY_PACKET_H__

#include <iostream>
#include <string>
#include <optional>
#include <array>
#include <sstream>

enum REQ_TYPE
{
    CONNECT = 0,
    TIME,
    NAME,
    LIST,
    SEND,
    DISCON
};
const std::array<std::string, 6> REQ_STR = {"CONNECT", "TIME", "NAME", "LIST", "DISCON", "SEND"};

#define PACKET "PACKET"
#define MAXSIZE 1024 // buffer size

class MyPacket
{
private:
    int type;
    int id;
    std::string message;

public:
    MyPacket();
    MyPacket(int type, std::string message, int id = -1);
    void set_packet(int type, std::string message = "", int id = -1);
    int get_type();
    int get_id();
    std::string get_message();
    std::string packet_to_string();
};

MyPacket parsePacket(std::string buffer);

#endif