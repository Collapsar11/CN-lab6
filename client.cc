#include "my_packet.hh"
#include "client.hh"

int print_menu()
{
    int opt;
    std::lock_guard<std::mutex> lock(console_mutex);
    {
        if (!is_connected)
        {
            std::cout << "\033[33m************Function Menu************" << std::endl;
            std::cout << "* 1. Connect                     *" << std::endl;
            std::cout << "* 2. Exit                        *" << std::endl;
            std::cout << "\033[34m(Client)\033[0m Please enter the number to choose the function: ";
        }
        else
        {
            std::cout << "\033[33m************Function Menu************" << std::endl;
            std::cout << "* 1. Get Time                   *" << std::endl;
            std::cout << "* 2. Get Name                   *" << std::endl;
            std::cout << "* 3. Get Client List            *" << std::endl;
            std::cout << "* 4. Send Message               *" << std::endl;
            std::cout << "* 5. Disconnect                 *" << std::endl;
            std::cout << "********************************\033[0m" << std::endl;
            std::cout << "\033[34m(Client)\033[0m Please enter the number to choose the function: ";
        }
    }
    std::cin >> opt;
    if (opt == 2)
        is_connected ? 2 : 6;
    return opt;
}

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
            switch (packet.get_type())
            {
            case REQ_TYPE::SEND:
                std::cout << "\n\033[35m(Server)\033[0m " << packet.get_message() << std::endl;
                break;
            case REQ_TYPE::DISCON:
                is_connected = false;
                std::cout << "\n\033[31mServer has disconnected\033[0m" << std::endl;
                break;
            default:
                std::cout << "\033[35m(Server)\033[0m " << packet.get_message() << std::endl;
            }
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
    std::cout << "\033[34m(Client)\033[0m Please enter the server's IP address (in dotted decimal notation): " << std::endl;
    std::cin >> ip;
    dst.first = ip;
    std::cout << "\033[34m(Client)\033[0m Please enter the server's port: " << std::endl;
    std::cin >> port;
    dst.second = std::stoul(port);
    return dst;
}

std::pair<std::string, unsigned int> getIDandMsg()
{
    std::pair<std::string, unsigned int> dst;
    std::string id;
    std::string msg;
    std::cout << "\033[34m(Client)\033[0m Please enter your client ID: " << std::endl;
    std::cin >> id;
    std::cout << "\033[34m(Client)\033[0m Please enter the message content: " << std::endl;
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
                std::cout << "\033[34m(Client)\033[0m Connection successful!" << std::endl;
                is_connected = true;
                std::thread t(recv_msg_child_thread);
                t.detach();
            }
            else
            {
                std::cout << "\033[34m(Client)\033[0m Connection failed!" << std::endl;
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
            case 1:
                packet.set_packet(REQ_TYPE::TIME);
                break;
            case 2:
                packet.set_packet(REQ_TYPE::NAME);
                break;
            case 3:
                packet.set_packet(REQ_TYPE::LIST);
                break;
            case 4:
            {
                std::pair<std::string, unsigned int> dst = getIDandMsg();
                MyPacket packet(REQ_TYPE::SEND, dst.first);
                std::string str = packet.packet_to_string();
                if (send(tcp_socket, str.c_str(), str.size(), 0) == -1)
                    std::cout << "\033[31mSend failed:" << strerror(errno) << "\033[0m" << std::endl;
                else
                    std::cout << "\033[32m(Console)\033[0m Message has been sent to the server, please wait for a response..." << std::endl;
                continue;
            }
            case 5:
                packet.set_packet(REQ_TYPE::DISCON);
                is_connected = false;
                break;
            default:
                break;
            }
            std::string str = packet.packet_to_string();
            const char *msg = str.c_str();
            if (send(tcp_socket, msg, str.size(), 0) == -1)
                throw("Request send failed!");
            else
            {
                std::cout << "\033[32m(Console)\033[0m Request successfully sent, please wait..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            if (!is_connected)
                close(tcp_socket);
        }
    }

    return 0;
}