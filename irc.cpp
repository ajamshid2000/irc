/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   irc.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ajamshid <ajamshid@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/27 16:05:54 by ajamshid          #+#    #+#             */
/*   Updated: 2025/07/30 19:57:20 by ajamshid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Clients.hpp"

Clients clients_bj;
std::string server_password;
Channels g_channels;

int set_nonblocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return -1;

	return 0;
}
std::string get_username(std::string rest)
{
	std::string username;
	std::istringstream iss(rest);
	iss >> username;
	return username;
}

void handle_command(Client &client, const std::string &line)
{
	std::cout <<"---------" + line + "---------" << std::endl;
	if(client.disconnect)
		return;
	std::string cmd;
	std::string rest;
	std::istringstream iss(line);
	iss >> cmd;
	std::getline(iss, rest);
	if (rest.size() > 1)
		rest = rest.substr(1);
	if (cmd == "CAP" && rest.substr(0, 2) == "LS")
		send_msg(client.fd, "CAP * LS :\r\n");
	else if (cmd == "PASS")
		pass(client, rest);
	else if (cmd == "NICK")
		nick(client, rest);
	else if (cmd == "USER")
		user(client, get_username(rest));
	else if (cmd == "PRIVMSG")
		privmsg(client, rest);
	else if (cmd == "NOTICE")
		notice(client, rest);
	else if (cmd == "PING")
	{
		send_msg(client.fd, "PONG ircserv\r\n");
	}
	else if (cmd == "JOIN")
		join(client, rest);
	else if (cmd == "TOPIC")
		topic(client, rest);
	else if (cmd == "INVITE")
		invite(client, rest);
	else if (cmd == "KICK")
		kick(client, rest);
	else if (cmd == "MODE")
		mode(client, rest);
	if (client.pass_ok && !client.nickname.empty() && !client.username.empty() && !client.registered)
	{
		client.registered = true;
		send_msg(client.fd, ":ircserv 001 " + client.nickname + " :Welcome to the IRC ircserv\r\n");
	}
}

void handle_data(int fd)
{
	char buf[BUFFER_SIZE + 1];
	ssize_t bytes;
	size_t pos;
	int i;

	memset(buf, 0, sizeof(buf));
	bytes = recv(fd, buf, BUFFER_SIZE, 0);
	if (bytes <= 0)
	{
		disconnect_client(fd);
		return;
	}
	clients_bj.add_to_client_recieve_buffer(fd, buf);
	i = 0;
	while ((pos = clients_bj.get_client_recieve_buffer(fd).find("\r\n")) != std::string::npos)
	{
		std::string line = clients_bj.get_client_recieve_buffer(fd).substr(0, pos);
		clients_bj.get_client_recieve_buffer(fd).erase(0, pos + 2);
		handle_command(clients_bj.get_client(fd), line);
		i++;
	}
}

void send_data(std::vector<pollfd> &pollfds, int i)
{
	Client &client = clients_bj.get_client(pollfds[i].fd);
	ssize_t sent = send(pollfds[i].fd, client.send_buffer.c_str(), client.send_buffer.size(), 0);
	if (sent == -1)
	{
		disconnect_client(pollfds[i].fd);
	}
	else if ((size_t)sent < client.send_buffer.size())
	{
		client.send_buffer = client.send_buffer.substr(sent);
	}
	else if ((size_t)sent == client.send_buffer.size())
	{
		client.send_buffer.clear();
		pollfds[i].events &= ~POLLOUT;
	}
	if(client.disconnect == 1)
	{ 
		usleep(100000);
		shutdown(client.fd, SHUT_RDWR);
		disconnect_client(client.fd);
	}
}

int run_fds(std::vector<pollfd> &pollfds, int listen_fd)
{
	int client_fd;
	if (poll(&pollfds[0], pollfds.size(), -1) < 0)
	{
		perror("poll");
		return -1;
	}
	for (size_t i = 0; i < pollfds.size(); ++i)
	{
		if (pollfds[i].revents & POLLIN)
		{
			if (pollfds[i].fd == listen_fd)
			{
				client_fd = accept(listen_fd, NULL, NULL);
				if (client_fd >= 0)
				{
					if (set_nonblocking(client_fd) < 0)
						return -1;
					clients_bj.add_client(client_fd);
				}
			}
			else
				handle_data(pollfds[i].fd);
		}
		if (pollfds[i].revents & POLLOUT)
			send_data(pollfds, i);
	}
	return 0;
}

void reciever(int sig)
{
	if (sig == SIGINT)
	{
		for (size_t i = 0; i < clients_bj.get_pollfds().size(); ++i)
			close(clients_bj.get_pollfds()[i].fd);
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int port;
	int listen_fd;

	port = pars_args_and_port(argc, argv);
	if (port < 0)
		return (0);
	server_password = argv[2];
	listen_fd = socket_prep_and_binding(port);
	if (listen_fd < 0)
		return (0);
	if (set_nonblocking(listen_fd) < 0)
		return 0;

	struct sigaction sa;
	sa.sa_handler = reciever;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGINT, &sa, 0) == -1)
		std::cerr << "sigaction: error handling signal." << std::endl;

	std::cout << "Server started on port " << port << "\n";
	std::vector<pollfd> &pollfds = clients_bj.get_pollfds();
	pollfds.push_back((pollfd){listen_fd, POLLIN, 0});
	while (true)
		if (run_fds(pollfds, listen_fd) < 0)
			break;
	for (size_t i = 0; i < pollfds.size(); ++i)
		close(pollfds[i].fd);
	return (0);
}
