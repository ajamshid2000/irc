/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Clients.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abdul-rashed <abdul-rashed@student.42.f    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/30 12:56:51 by ajamshid          #+#    #+#             */
/*   Updated: 2025/07/01 02:00:37 by abdul-rashe      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Clients.hpp"

std::vector<pollfd> &Clients::get_pollfds()
{
	return (this->pollfds);
}

std::string Clients::get_nick(int fd)
{
	return (this->client[fd].nickname);
}
// std::vector<int> fds;                  // 4
// std::vector<pollfd> pollfds;           // 3
// std::map<int, Client> client;          // 2
// std::map<std::string, int> nick_to_fd; // 1

int Clients::remove_client(int fd)
{
	nick_to_fd.erase(client[fd].nickname); // 1
	client.erase(fd);                      // 2
	for (size_t i = 1; i < pollfds.size(); ++i)
		if (pollfds[i].fd == fd)
			// i could delete it in the disconnect function. no neet to do all of these
			pollfds.erase(pollfds.begin() + i);
	return (0);
}

Client &Clients::get_client(int fd)
{
	return (this->client[fd]);
}

void Clients::add_client(int fd)
{
	pollfds.push_back((pollfd){fd, POLLIN, 0});
	client[fd] = Client();
	client[fd].fd = fd;
	return ;
}

std::map<int, Client> &Clients::get_clients()
{
	return (client);
}

void Clients::add_to_client_recieve_buffer(int fd, std::string data)
{
	client[fd].recieve_buffer += data;
}
std::string &Clients::get_client_recieve_buffer(int fd)
{
	return (client[fd].recieve_buffer);
}
void	Clients::add_to_client_send_buffer(int fd, std::string data)
{
    client[fd].send_buffer += data;
}
std::string &Clients::get_client_send_buffer(int fd)
{
    return (client[fd].send_buffer);
}
std::map<std::string, int> &Clients::get_nick_to_fd()
{
    return nick_to_fd;
}
int Clients::get_fd_of(std::string nick)
{
    return nick_to_fd[nick];
}