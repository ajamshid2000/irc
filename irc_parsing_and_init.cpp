/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   irc_parsing_and_init.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abdul-rashed <abdul-rashed@student.42.f    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/30 17:22:05 by ajamshid          #+#    #+#             */
/*   Updated: 2025/07/01 02:13:02 by abdul-rashe      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Clients.hpp"

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

int socket_prep_and_binding(int port)
{
    int listen_fd;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create socket for AF_INET(ipv4), and Stream Sockets(for relieability) for robustness we use datagram(fast but unreliable)
    if (listen_fd < 0)
    {
        perror("socket");
        return -1;
    }
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // we set options(to make the port immediatly reusable after quiting the server)
    // we bind the socket to a port and ip (we tell the operating system that cirtain port and ip will be used by this socket)
    sockaddr_in addr;
    addr.sin_family = AF_INET;         // again for ipv4 (AF_INET6(ipv6))
    addr.sin_port = htons(port);       // to change port into network byte order/ to coply with internet protocol
    addr.sin_addr.s_addr = INADDR_ANY; // bind this server to all ip addresses this machine has (localhost(127.0.0.1), lan, or public ip) so to use localhost we do inet_addr("192.168.1.42")
    if (bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return -1;
    }
    if (listen(listen_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        return -1;
    }
    // std::cout << "Server started on port " << port << "\n";
    return listen_fd;
}
