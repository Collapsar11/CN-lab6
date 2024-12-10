#include "my_packet.hh"
#include <vector>

MyPacket::MyPacket() {}
MyPacket::MyPacket(int type, std::string message, int id)
{
    this->type = type;
    this->message = message;
    this->id = id;
}

void MyPacket::set_packet(int type, std::string message, int id)
{
    this->type = type;
    this->message = message;
    this->id = id;
}

int MyPacket::get_type()
{
    return this->type;
}

int MyPacket::get_id()
{
    return this->id;
}

std::string MyPacket::get_message()
{
    return message;
}

std::string MyPacket::packet_to_string()
{
    std::ostringstream oss;
    oss << PACKET << "|" << type << "|" << id << "|" << message << "|" << TAIL_FLAG;
    std::string str = oss.str();
    return str;
}

MyPacket parsePacket(std::string buffer)
{
    std::string flag = PACKET;
    std::vector<std::string> parts;
    std::string temp;
    std::stringstream ss(buffer);

    while (std::getline(ss, temp, '|'))
        parts.push_back(temp);

    int type = std::stoi(parts[1]);
    int id = std::stoi(parts[2]);
    std::string message = parts[3];

    return MyPacket(type, message, id);
}