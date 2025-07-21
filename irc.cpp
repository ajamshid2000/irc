#include "Clients.hpp"

int set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return -1;

	return 0;
}

Clients clients_bj;
std::string server_password;
Channels g_channels;

void handle_command(Client &client, const std::string &line)
{
	std::string cmd;
	std::string rest;
	std::istringstream iss(line);
	iss >> cmd;
	std::getline(iss, rest);
	rest = rest.substr(1);
	if (cmd == "CAP" && rest.substr(0, 2) == "LS")
		send_msg(client.fd, "CAP * LS :\r\n");
	else if (cmd == "PASS")
		pass(client, rest);
	else if (cmd == "NICK")
		nick(client, rest);
	else if (cmd == "USER")
		user(client, rest);
	else if (cmd == "PRIVMSG")
		privmsg(client, rest);
	else if (cmd == "PING")
	{
		send_msg(client.fd, "PONG server\r\n");
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
		send_msg(client.fd, ":server 001 " + client.nickname + " :Welcome to the IRC server\r\n");
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
		if (errno != EAGAIN && errno != EWOULDBLOCK)
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
	std::cout << "Server started on port " << port << "\n";
	std::vector<pollfd> &pollfds = clients_bj.get_pollfds();
	pollfds.push_back((pollfd){listen_fd, POLLIN, 0});
	while (true)
		if (run_fds(pollfds, listen_fd) < 0)
			break;
	close(listen_fd);
	return (0);
}
