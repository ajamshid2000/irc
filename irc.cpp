
#include "Clients.hpp"

Clients clients_bj;
std::string server_password;
std::map<std::string, int> nick_to_fd;

void handle_command(Client &client, const std::string &line)
{
    std::cout << "it came inside handle commande\n";
    std::string cmd;
    std::string rest;
    std::istringstream iss(line);
    iss >> cmd;
    std::getline(iss, rest);
    rest = rest.substr(rest.find(' ') + 1);

    if (cmd == "PASS")
        pass(client, rest);
    else if (cmd == "NICK")
        nick(client, rest);
    else if (cmd == "USER")
        user(client, rest);

    if (client.pass_ok && !client.nickname.empty() && !client.username.empty() && !client.registered)
    {
        client.registered = true;
        send_msg(client.fd, ":server 001 " + client.nickname + " :Welcome to the IRC server\r\n");
    }
    if (cmd == "PRIVMSG")
        privmsg(client, rest);
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

    Client &client = clients_bj.get_client(fd);
    client.recieve_buffer += buf;

    // if (client.recieve_buffer[1] == '\r')
    size_t pos;
    int i;
    i = 0;
    while ((pos = client.recieve_buffer.find("\r\n")) != std::string::npos)
    {
        std::cout << i << " it came inwhile" << std::endl;
        // std::cout << client.recieve_buffer << std::endl;
        // send_msg(client.fd,  client.recieve_buffer);
        std::string line = client.recieve_buffer.substr(0, pos);
        client.recieve_buffer.erase(0, pos + 2);
        handle_command(client, line);
        i++;
    }
    // if (pollout event not set)
    // the send_buffer should be checked over here if there is data pollout event must be set
    // if pollout event set
    // call send_msg
    // reset the event to
}

int main(int argc, char **argv)
{
    int port = pars_args_and_port(argc, argv);
    if (port < 0)
        return 0;
    server_password = argv[2];

    int listen_fd = socket_prep_and_binding(port);
    if (listen_fd < 0)
        return 0;

    std::cout << "Server started on port " << port << "\n";
    std::vector<pollfd> &pollfds = clients_bj.get_pollfds();
    pollfds.push_back((pollfd){listen_fd, POLLIN, 0});

    while (true)
    {
        std::cout << "waits before pll\n";
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
                        clients_bj.add_client(client_fd);
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
    }

    close(listen_fd);
    return 0;
}