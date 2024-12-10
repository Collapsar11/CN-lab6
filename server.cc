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
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // htonl(INADDR_ANY);
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
        ss << "Client ID: " << pair.first + 1 << ", IP: " << inet_ntoa((pair.second).client_addr.sin_addr) << ", Port: " << ntohs((pair.second).client_addr.sin_port);
    return ss.str();
}
int MyServer::find_client_socket(int list_id)
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

        pthread_t t;
        struct thread_info info = {list_id, client_socket, client_addr, this};
        pthread_create(&t, NULL, process_client, (void *)&info);
    }
}
void *process_client(void *thread_info)
{
    struct thread_info info = *((struct thread_info *)thread_info);
    int list_id = info.list_id;
    int client_socket = info.client_socket;
    sockaddr_in client_addr = info.client_addr;
    MyServer *server = info.server;

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
            std::cout << "  Receive message: " << recv_packet.get_message() << std::endl;
            recv_message(client_socket, list_id, -1, recv_packet.get_message(), *server);
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
    // std::lock_guard<std::mutex> lock(mtx);
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
void recv_message(int client_socket, int client_id, int dst_id, std::string message, MyServer server)
{
    try
    {
        std::string ack_msg = "Server received your message: " + message;
        MyPacket ack_packet(REQ_TYPE::SEND, ack_msg);
        std::string ack_str = ack_packet.packet_to_string();

        std::cout << "  Sending ACK packet: [" << ack_str << "]" << std::endl;
        if (send(client_socket, ack_str.c_str(), ack_str.size(), 0) == -1)
            std::cerr << "Failed to send acknowledgment" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in recv_message: " << e.what() << std::endl;
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