#include "my_packet.hh"
#include "client.hh"

int print_menu()
{
    int opt;
    if (!is_connected)
    {
        std::cout << "\033[33m************功能菜单************" << std::endl;
        std::cout << "* 1. 连接                      *" << std::endl;
        std::cout << "* 2. 退出                      *" << std::endl;
        std::cout << "\033[34m(Client)\033[0m 请输入序号选择功能: ";
    }
    else
    {
        std::cout << "\033[33m************功能菜单************" << std::endl;
        std::cout << "* 1. 获取时间                  *" << std::endl;
        std::cout << "* 2. 获取名字                  *" << std::endl;
        std::cout << "* 3. 获取客户端列表            *" << std::endl;
        std::cout << "* 4. 发送消息                  *" << std::endl;
        std::cout << "* 5. 断开连接                  *" << std::endl;
        std::cout << "* 6. 退出                      *" << std::endl;
        std::cout << "********************************\033[0m" << std::endl;
        std::cout << "\033[34m(Client)\033[0m 请输入序号选择功能: ";
    }
    std::cin >> opt;
    if (opt == 2)
        is_connected ? 2 : 6;
    return opt;
}

// void recv_msg_child_thread()
// {
//     std::string message;
//     while (is_connected)
//     {
//         char buffer[MAXSIZE];
//         memset(buffer, 0, sizeof(buffer));
//         recv(tcp_socket, buffer, sizeof(buffer) - 1, 0);
//         MyPacket packet = parsePacket(buffer);
//         memset(buffer, 0, sizeof(buffer));
//         // std::lock_guard<std::mutex> lock(mtx);
//         recv_response_time++;
//         std::cout << "\033[35m(Server)\033[0m " << packet.get_message() << " " << recv_response_time << std::endl;
//     }
// }
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
            std::cout << "\n\033[31m连接已断开\033[0m" << std::endl;
            break;
        }

        MyPacket packet = parsePacket(buffer);

        switch (packet.get_type())
        {
        case REQ_TYPE::SEND:
            std::cout << "\033[35m(Server)\033[0m " << packet.get_message() << std::endl;
            break;
        case REQ_TYPE::DISCON:
            is_connected = false;
            std::cout << "\n\033[31m服务器已断开连接\033[0m" << std::endl;
            break;
        default:
            std::cout << "\033[35m(Server)\033[0m " << packet.get_message() << std::endl;
        }
    }

    if (!is_connected)
        close(tcp_socket);
}

std::pair<std::string, unsigned int> getIPandPort()
{
    std::pair<std::string, unsigned int> dst;
    std::string ip;
    std::string port;
    std::cout << "\033[34m(Client)\033[0m 请输入服务器的IP地址(点分十进制表示法): " << std::endl;
    std::cin >> ip;
    dst.first = ip;
    std::cout << "\033[34m(Client)\033[0m 请输入服务器的端口: " << std::endl;
    std::cin >> port;
    dst.second = std::stoul(port);
    return dst;
}

std::pair<std::string, unsigned int> getIDandMsg()
{
    std::pair<std::string, unsigned int> dst;
    std::string id;
    std::string msg;
    std::cout << "\033[34m(Client)\033[0m 请输入发送客户端的编号: " << std::endl;
    std::cin >> id;
    std::cout << "\033[34m(Client)\033[0m 请输入发送内容: " << std::endl;
    std::cin >> msg;
    dst.first = msg;
    dst.second = std::stoul(id);
    return dst;
}

int main()
{
    while (true)
    {
        int opt = print_menu();
        // std::cout << "input == " << opt;
        if (opt == 2 && !is_connected)
            break;
        if (opt == 1 && !is_connected)
        {
            tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

            std::pair<std::string, unsigned int> dst = getIPandPort();

            struct sockaddr_in serv_addr;
            std::memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr(dst.first.c_str());
            serv_addr.sin_port = htons(dst.second);

            if (connect(tcp_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != -1)
            {
                std::cout << "\033[34m(Client)\033[0m 连接成功！" << std::endl;
                is_connected = true;
                std::thread t(recv_msg_child_thread);
                t.detach();
            }
            else
            {
                std::cout << "\033[34m(Client)\033[0m 连接失败!" << std::endl;
                is_connected = false;
            }
            continue;
        }
        if (is_connected)
        {
            MyPacket packet;
            std::pair<std::string, unsigned int> dst; // = getIDandMsg();
            switch (opt)
            {
            // case 1:
            //     std::cout << "\033[34m(Client)\033[0m 已连接到服务端！" << std::endl;
            //     break;
            case 1:
                packet.set_packet(REQ_TYPE::TIME);
                break;
            case 2:
                packet.set_packet(REQ_TYPE::NAME);
                break;
            case 3:
                packet.set_packet(REQ_TYPE::LIST);
                break;
            // case 4:
            //     dst = getIDandMsg();
            //     packet.set_packet(REQ_TYPE::SEND, dst.first, dst.second);
            //     break;
            case 4:
            {
                std::pair<std::string, unsigned int> dst = getIDandMsg();
                // 发送消息给服务器
                MyPacket packet(REQ_TYPE::SEND, dst.first); // 只发送消息内容
                std::string str = packet.packet_to_string();

                if (send(tcp_socket, str.c_str(), str.size(), 0) == -1)
                {
                    std::cout << "\033[31m发送失败：" << strerror(errno) << "\033[0m" << std::endl;
                }
                else
                {
                    std::cout << "\033[32m[System]\033[0m 消息已发送到服务器，请等待响应..." << std::endl;
                }
                continue;
            }
            case 5:
                packet.set_packet(REQ_TYPE::DISCON);
                is_connected = false;
                break;
            case 6:
                packet.set_packet(REQ_TYPE::DISCON);
                is_connected = false;
                break;
            default:
                break;
            }
            std::string str = packet.packet_to_string();
            // std::cout << "\npacket: " << str << std::endl;
            const char *msg = str.c_str();
            // send(tcp_socket, msg, str.size(), 0);
            // std::cout << "\033[34m(Client)\033[0m 请求成功发送！" << std::endl;
            // std::this_thread::sleep_for(std::chrono::seconds(1));
            if (send(tcp_socket, msg, str.size(), 0) == -1)
                throw("请求发送失败!");
            else
            {
                std::cout << "\033[32m[System]\033[0m 请求成功发送, 请稍等..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            if (!is_connected)
                close(tcp_socket);
        }
    }

    return 0;
}