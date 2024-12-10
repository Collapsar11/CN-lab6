#include "my_packet.hh"
#include "server.hh"

MyServer::MyServer()
{
    for (int i = 0; i < MAX_CLIENT; i++)
        client_list_use[i] = 0;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::cout << "Server socket error!" << std::endl;
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        std::cout << "Set socket option error!" << std::endl;
        close(server_socket);
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);
    if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        close(server_socket);
        std::cout << "Bind error!" << std::endl;
        exit(1);
    }
    std::cout << "\033[33mBind Port " << PORT << " succeed!\033[0m" << std::endl;

    listen(server_socket, MAX_CLIENT);
    std::cout << "\033[33mServer listening on port " << PORT << "...\033[0m" << std::endl;
}
MyServer::~MyServer()
{
    close(server_socket);
}
void MyServer::add_client(int list_id, int client_socket, sockaddr_in client_addr)
{
    client_list[list_id].client_socket = client_socket;
    client_list[list_id].client_addr = client_addr;
    client_list_use[list_id] = 1;
}
bool MyServer::client_list_empty()
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (client_list_use[i] == 1)
            return 1;
    }
    return 0;
}
void MyServer::rst_client_list(int list_id)
{
    this->client_list_use[list_id] = 0;
    this->client_list.erase(list_id);
}
int MyServer::get_server_socket()
{
    return this->server_socket;
}
int MyServer::get_list_id()
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (this->client_list_use[i] == 0)
            return i;
    }
    return -1;
}
void MyServer::set_is_server_down()
{
    this->_is_server_down = true;
}
bool MyServer::is_server_down()
{
    return this->_is_server_down;
}
std::string MyServer::print_list()
{
    std::stringstream ss;
    for (const auto &pair : this->client_list)
        ss << "Client ID: " << pair.first + 1 << ", IP: " << inet_ntoa((pair.second).client_addr.sin_addr) << ", Port: " << ntohs((pair.second).client_addr.sin_port) << std::endl;
    return ss.str();
}
int MyServer::find_socket(int list_id)
{
    return client_list_use[list_id] ? client_list[list_id].client_socket : -1;
}
void MyServer::_server_on()
{
    std::cout << "\033[33mWaiting for client to connect...\033[0m" << std::endl;
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr *)&client_addr, &client_addr_size);

        if (client_socket == -1)
        {
            std::cout << "Accept error!" << std::endl;
            continue;
        }

        std::cout << std::endl
                  << "\033[34mA client connected!\033[0m" << std::endl;
        std::cout << "  Client IP: " << inet_ntoa(client_addr.sin_addr) << std::endl;
        std::cout << "  Client Port: " << ntohs(client_addr.sin_port) << std::endl;

        int list_id = get_list_id();
        add_client(list_id, client_socket, client_addr);

        // pthread_t t;
        // struct thread_info info = {list_id, client_socket, client_addr, this};
        // pthread_create(&t, NULL, process_client, (void *)&info);
        thread_info *info = new thread_info{list_id, client_socket, client_addr, this};

        pthread_t t;
        if (pthread_create(&t, NULL, process_client, (void *)info) != 0)
        {
            std::cerr << "Failed to create thread" << std::endl;
            delete info; // 创建失败时释放内存
            continue;
        }
        pthread_detach(t); // 分离线程，让系统自动回收线程资源
    }
}
void *process_client(void *thread_info)
{
    // struct thread_info info = *((struct thread_info *)thread_info);
    // int list_id = info.list_id;
    // int client_socket = info.client_socket;
    // sockaddr_in client_addr = info.client_addr;
    // MyServer *server = info.server;
    struct thread_info *info = (struct thread_info *)thread_info;
    int list_id = info->list_id;
    int client_socket = info->client_socket;
    sockaddr_in client_addr = info->client_addr;
    MyServer *server = info->server;

    delete info;

    char buffer[MAXSIZE];
    ssize_t recv_len;
    while (true)
    {
        recv_len = recv(client_socket, buffer, MAXSIZE - 1, 0);
        if (recv_len <= 0)
            break;

        buffer[recv_len] = '\0';
        std::string recv_content(buffer);
        MyPacket recv_packet = parsePacket(recv_content);

        std::cout << "\n\033[32mRequest from client [" << list_id + 1 << "] \033[0m";
        std::cout << "\n  Type: " << recv_packet.get_type() << std::endl;

        switch (recv_packet.get_type())
        {
        case REQ_TYPE::CONNECT:
            get_connect(client_socket);
            break;
        case REQ_TYPE::TIME:
            get_time(client_socket);
            break;
        case REQ_TYPE::NAME:
            get_name(client_socket);
            break;
        case REQ_TYPE::LIST:
            get_list(client_socket, *server);
            break;
        case REQ_TYPE::SEND:
            std::cout << "  Send message: " << recv_packet.get_message() << "to " << recv_packet.get_id() << std::endl;
            send_message(client_socket, list_id, recv_packet.get_id(), recv_packet.get_message(), *server);
            break;
        case REQ_TYPE::DISCON:
            stop_connect(client_socket);
            server->set_is_server_down();
            break;
        default:
            std::cout << "Invalid request type: " << recv_packet.get_type() << std::endl;
            break;
        }

        if (server->is_server_down())
            break;
    }

    close(client_socket);
    server->rst_client_list(list_id);

    if (server->is_server_down() && !server->client_list_empty())
    {
        std::cout << "\n\033[33mAll the client disconnected! Server terminate!\033[0m" << std::endl;
        close(server->get_server_socket());
        exit(0);
    }

    return nullptr;
}
void get_connect(int client_socket)
{
    MyPacket packet(REQ_TYPE::CONNECT, "connect successfully!");
    std::cout << "  Send message: " << "connect successfully!";
    send(client_socket, packet.packet_to_string().c_str(), packet.packet_to_string().size(), 0);
}
void get_time(int client_socket)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    std::string time_str = asctime(timeinfo);
    if (!time_str.empty() && time_str[time_str.length() - 1] == '\n')
    {
        time_str.erase(time_str.length() - 1);
    }
    MyPacket packet(REQ_TYPE::TIME, time_str);
    std::cout << "  Send message: " << time_str << std::endl;

    send(client_socket, packet.packet_to_string().c_str(), packet.packet_to_string().size(), 0);
}
void get_name(int client_socket)
{
    std::string hostname;
    hostname.resize(256);
    gethostname(&hostname[0], hostname.size());
    MyPacket packet(REQ_TYPE::NAME, hostname);
    std::cout << "  Send message: " << hostname << std::endl;
    send(client_socket, packet.packet_to_string().c_str(), packet.packet_to_string().size(), 0);
}
void get_list(int client_socket, MyServer server)
{
    std::string message(server.print_list());
    MyPacket packet(REQ_TYPE::LIST, message);
    std::cout << "  Send message: " << message << std::endl;
    send(client_socket, packet.packet_to_string().c_str(), packet.packet_to_string().size(), 0);
}
// void send_message(int client_socket, int client_id, int dst_id, std::string message, MyServer server)
// {
//     try
//     {
//         // std::string send_msg = "Server received client" + client_id + "'s message: " + message;
//         std::string send_msg = "Server forwards client " + std::to_string(client_id + 1) + "'s message to you: " + message;
//         MyPacket send_packet(REQ_TYPE::SEND, send_msg, dst_id);
//         std::string send_str = send_packet.packet_to_string();

//         int target_socket = server.find_socket(dst_id - 1);
//         std::cout << "  Sending message packet: [" << send_str << "] to client" << dst_id << std::endl;
//         //     if (send(target_socket, send_str.c_str(), send_str.size(), 0) == -1)
//         //         std::cerr << "Failed to send acknowledgment" << std::endl;
//         // }
//         if (send(target_socket, send_str.c_str(), send_str.size(), 0) == -1)
//         {
//             std::cerr << "Failed to send message to target client" << std::endl;
//             // 发送失败消息给源客户端
//             MyPacket error_packet(REQ_TYPE::SEND, "Failed to deliver message to target client");
//             send(client_socket, error_packet.packet_to_string().c_str(),
//                  error_packet.packet_to_string().size(), 0);
//         }
//         else
//         {
//             // 发送成功确认消息给源客户端
//             MyPacket ack_packet(REQ_TYPE::SEND, "Message successfully delivered to client " + std::to_string(dst_id));
//             send(client_socket, ack_packet.packet_to_string().c_str(),
//                  ack_packet.packet_to_string().size(), 0);
//         }
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "Error in recv_message: " << e.what() << std::endl;
//         MyPacket error_packet(REQ_TYPE::SEND, "Failed to process message");
//         send(client_socket, error_packet.packet_to_string().c_str(),
//              error_packet.packet_to_string().size(), 0);
//     }
// }
void send_message(int client_socket, int client_id, int dst_id, std::string message, MyServer server)
{
    try
    {
        // 转发消息给目标客户端
        std::string send_msg = "Server forwards client " + std::to_string(client_id + 1) + "'s message to you: " + message;
        MyPacket send_packet(REQ_TYPE::SEND, send_msg, dst_id);
        std::string send_str = send_packet.packet_to_string();

        int target_socket = server.find_socket(dst_id - 1);
        if (target_socket == -1)
        {
            // 发送错误消息给源客户端
            MyPacket error_packet(REQ_TYPE::SEND, "Target client not found");
            send(client_socket, error_packet.packet_to_string().c_str(),
                 error_packet.packet_to_string().size(), 0);
            return;
        }

        // 发送消息给目标客户端
        if (send(target_socket, send_str.c_str(), send_str.size(), 0) == -1)
        {
            // 发送失败消息给源客户端
            MyPacket error_packet(REQ_TYPE::SEND, "Failed to deliver message to target client");
            send(client_socket, error_packet.packet_to_string().c_str(),
                 error_packet.packet_to_string().size(), 0);
        }
        else
        {
            // 发送成功确认消息给源客户端
            MyPacket ack_packet(REQ_TYPE::SEND, "Message successfully delivered to client " + std::to_string(dst_id));
            send(client_socket, ack_packet.packet_to_string().c_str(),
                 ack_packet.packet_to_string().size(), 0);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in send_message: " << e.what() << std::endl;
        MyPacket error_packet(REQ_TYPE::SEND, "Failed to process message");
        send(client_socket, error_packet.packet_to_string().c_str(),
             error_packet.packet_to_string().size(), 0);
    }
}
void stop_connect(int client_socket)
{
    std::string message("disconnect");
    MyPacket packet(REQ_TYPE::DISCON, message);
    std::cout << "  Send message: " << message << std::endl;
    send(client_socket, packet.packet_to_string().c_str(), packet.packet_to_string().size(), 0);
}

int main()
{
    _server._server_on();
    return 0;
}