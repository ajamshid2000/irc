#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 512

struct Client
{
    int fd;
    std::string nickname;
    std::string username;
    bool pass_ok;
    bool registered;
    std::string buffer;
};

std::string server_password;
std::map<int, Client> clients;
std::map<std::string, int> nick_to_fd;

bool is_valid_nick(const std::string &nick)
{
    return !nick.empty() && nick.length() <= 9;
}

void disconnect_client(int fd)
{
    std::cout << "Client disconnected (fd " << fd << ")\n";
    if (clients[fd].nickname != "")
        nick_to_fd.erase(clients[fd].nickname);
    close(fd);
    clients.erase(fd);
}

void send_msg(int fd, const std::string &msg)
{
    send(fd, msg.c_str(), msg.length(), 0);
}

void handle_command(Client &client, const std::string &line)
{
    std::cout << "it came inside handle commande\n";
    std::string cmd;
    std::istringstream iss(line);
    iss >> cmd;

    if (cmd == "PASS")
    {
        std::string pass;
        iss >> pass;
        std::cout <<pass << std::endl;
        if (pass == server_password)
            client.pass_ok = true;
        else
            disconnect_client(client.fd);
    }
    else if (cmd == "NICK")
    {
        std::string nick;
        iss >> nick;
        std::cout << nick << std::endl;
        if (!client.pass_ok)
            disconnect_client(client.fd);
        else if (!is_valid_nick(nick) || nick_to_fd.count(nick))
        {
            send_msg(client.fd, ":server 433 * " + nick + " :Nickname is already in use\r\n");
        }
        else
        {
            if (!client.nickname.empty())
                nick_to_fd.erase(client.nickname);
            client.nickname = nick;
            nick_to_fd[nick] = client.fd;
        }
    }
    else if (cmd == "USER")
    {
        std::string user;
        iss >> user;
        std::cout << user << std::endl;
        if (!client.pass_ok)
            disconnect_client(client.fd);
        else
        {
            client.username = user;
        }
    }

    if (client.pass_ok && !client.nickname.empty() && !client.username.empty() && !client.registered)
    {
        client.registered = true;
        send_msg(client.fd, ":server 001 " + client.nickname + " :Welcome to the IRC server\r\n");
    }
    if (cmd == "PRIVMSG")
    {
        std::cout << "it does come inside privmsg \n";
        std::string pass;
        iss >> pass;
        send_msg(client.fd, pass + "\r\n");
    }
}

void handle_data(int fd)
{
    // std::cout << "came inside habdle data\n";
    char buf[BUFFER_SIZE + 1];
    memset(buf, 0, sizeof(buf));
    ssize_t bytes = recv(fd, buf, BUFFER_SIZE, 0);

    if (bytes <= 0)
    {
        disconnect_client(fd);
        return;
    }

    Client &client = clients[fd];
    client.buffer += buf;
    
    // if (client.buffer[1] == '\r')
    size_t pos;
    int i;
     i = 0;
    while ((pos = client.buffer.find("\r\n")) != std::string::npos)
    {
        std::cout << i << " it came inwhile" << std::endl;
        // std::cout << client.buffer << std::endl;
        // send_msg(client.fd,  client.buffer);
        std::string line = client.buffer.substr(0, pos);
        client.buffer.erase(0, pos + 2);
        handle_command(client, line);
        i++;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>\n";
        return 1;
    }

    int port = atoi(argv[1]);
    server_password = argv[2];

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        return 1;
    }

    std::cout << "Server started on port " << port << "\n";

    std::vector<pollfd> pollfds;
    pollfds.push_back((pollfd){listen_fd, POLLIN, 0});

    while (true)
    {
        if (poll(&pollfds[0], pollfds.size(), -1) < 0)
        {
            perror("poll");
            break;
        }

        for (size_t i = 0; i < pollfds.size(); ++i)
        {
            if (pollfds[i].revents & POLLIN)
            {
                if (pollfds[i].fd == listen_fd)
                {
                    int client_fd = accept(listen_fd, NULL, NULL);
                    if (client_fd >= 0)
                    {
                        pollfds.push_back((pollfd){client_fd, POLLIN, 0});
                        clients[client_fd] = Client();
                        clients[client_fd].fd = client_fd;
                        std::cout << "New connection: fd " << client_fd << "\n";
                    }
                }
                else
                {
                    std::cout << "came in else" << std::endl;
                    handle_data(pollfds[i].fd);
                }
            }
        }

        // Clean up disconnected clients
        for (size_t i = 1; i < pollfds.size();)
        {
            if (clients.find(pollfds[i].fd) == clients.end())
            {
                pollfds.erase(pollfds.begin() + i);
            }
            else
            {
                ++i;
            }
        }
    }

    close(listen_fd);
    return 0;
}
