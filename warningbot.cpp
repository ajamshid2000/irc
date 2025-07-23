/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   warningbot.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ajamshid <ajamshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/23 19:31:14 by ajamshid          #+#    #+#             */
/*   Updated: 2025/07/23 19:57:05 by ajamshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>
#include <fcntl.h>
#include <vector>

#define BUFFER_SIZE 512

struct Client
{
    int fd = 0;
    std::string nickname;
    std::string username;
    bool pass_ok = 0;
    bool registered = 0;
    std::string channels;
    std::string recieve_buffer;
    std::string send_buffer;
};

Client warning_bot;
struct pollfd pollfds;
std::vector<std::string> bad_words = {"fuck", "shit"};

void send_bot_msg(const std::string &message)
{
    pollfds.events |= POLLOUT;
    warning_bot.send_buffer += message;
    return;
}

int check_for_bad_words(std::string message)
{
    size_t pos;
    for(size_t i = 0; i < bad_words.size(); ++i)
    {
        if((pos = message.find(bad_words[i])) != std::string::npos)
            return (int)pos + 1;
    }
    return 0;
}

int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return -1;

    return 0;
}

void handle_command(const std::string &line)
{
    std::string cmd;
    std::string rest;
    std::istringstream iss(line);
    iss >> cmd;
    std::getline(iss, rest);
    if (rest.size() > 1)
        rest = rest.substr(1);
    if (cmd[0] == ':')
    {
        if (std::string(cmd.c_str() + 1) == "server")
            std::cout << rest.c_str() + rest.find(':') + 1 << std::endl;
        else
        {
            if (rest.find("INVITE") != std::string::npos)
                send_bot_msg("JOIN " + std::string(rest.c_str() + rest.find(':') + 1) + "\r\n");
            else if(check_for_bad_words(rest.c_str() + rest.find(':') + 1))
            {
                cmd = cmd.c_str() + 1;
                std::istringstream iss(rest);
                std::string privmsg;
                iss >> privmsg;
                std::string channel;
                iss >> channel;
                std::string message;
                std::getline(iss, message, ':');
                send_bot_msg("PRIVMSG " + channel + " :\033[31m@" + cmd + ", please refrain from using bad words\033[0m\r\n");
            }
        }
    }
}

int handle_bot_data(int fd)
{
    char buf[BUFFER_SIZE + 1];
    ssize_t bytes;
    size_t pos;
    int i;

    memset(buf, 0, sizeof(buf));
    bytes = recv(fd, buf, BUFFER_SIZE, 0);
    if (bytes <= 0)
    {
        std::cerr << "server disconnected" << std::endl;
        return (-1);
    }
    warning_bot.recieve_buffer += buf;
    i = 0;
    while ((pos = warning_bot.recieve_buffer.find("\r\n")) != std::string::npos)
    {
        std::string line = warning_bot.recieve_buffer.substr(0, pos);
        warning_bot.recieve_buffer.erase(0, pos + 2);
        handle_command(line);
        i++;
    }
    if ((warning_bot.recieve_buffer.find(EOF)) != std::string::npos)
    {
        std::cerr << "server disconnected1" << std::endl;
        return (-1);
    }
    return 0;
}

int send_bot_data()
{

    ssize_t sent = send(pollfds.fd, warning_bot.send_buffer.c_str(), warning_bot.send_buffer.size(), 0);

    if (sent == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("send");
            return -1;
        }
    }
    else if ((size_t)sent < warning_bot.send_buffer.size())
    {

        warning_bot.send_buffer = warning_bot.send_buffer.substr(sent);
    }
    else if ((size_t)sent == warning_bot.send_buffer.size())
    {
        warning_bot.send_buffer.clear();
        pollfds.events &= ~POLLOUT;
    }

    return 0;
}

int is_all_digits(std::string str)
{
    for (std::string::iterator begin = str.begin(); begin != str.end(); begin++)
    {
        if (!std::isdigit(*begin))
            return 0;
    }
    return 1;
}

int pars_args_and_port(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "wrong number of arguments, must have <port> <password>" << std::endl;
        return -1;
    }
    if (!is_all_digits(argv[1]))
    {
        std::cerr << "invalid port";
        return -1;
    }
    int port = atoi(argv[1]);
    if (port > 65535)
    {
        std::cerr << "invalid port";
        return -1;
    }
    return port;
}

int socket_prep_and_connect(int port)
{
    int fd;
    fd = socket(AF_INET, SOCK_STREAM, 0); // create socket for AF_INET(ipv4), and Stream Sockets(for relieability) for robustness we use datagram(fast but unreliable)
    if (fd < 0)
    {
        perror("socket");
        return -1;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;         // again for ipv4 (AF_INET6(ipv6))
    addr.sin_port = htons(port);       // to change port into network byte order/ to coply with internet protocol
    addr.sin_addr.s_addr = INADDR_ANY; // bind this server to all ip addresses this machine has (localhost(127.0.0.1), lan, or public ip) so to use localhost we do inet_addr("192.168.1.42")
    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        return -1;
    }
    return fd;
}

int run_fds()
{
    if (poll(&pollfds, 1, -1) < 0)
    {
        perror("poll");
        return -1;
    }
    if (pollfds.revents & POLLIN)
        if (handle_bot_data(pollfds.fd) < 0)
            return -1;
    if (pollfds.revents & POLLOUT)
        send_bot_data();
    return 0;
}

int main(int argc, char **argv)
{
    int port;
    int fd;
    std::string server_password;

    port = pars_args_and_port(argc, argv);
    if (port < 0)
        return (0);
    server_password = argv[2];
    fd = socket_prep_and_connect(port);
    if (fd < 0)
    {
        return (0);
    }
    if (set_nonblocking(fd) < 0)
    {
        perror("fcntl");
        return 0;
    }
    std::cout << "connected to the server" << std::endl;
    pollfds = (struct pollfd){fd, POLLIN, 0};
    send_bot_msg("PASS " + server_password + "\r\nNICK warningbot\r\nUSER warningbot\r\n");
    while (true)
        if (run_fds() < 0)
            break;
    close(fd);
    return (0);
}
