#include "Clients.hpp"

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, delimiter))
        tokens.push_back(token);

    return tokens;
}

bool isValidChannelName(const std::string &name)
{
    if (name.empty() || name.length() > 200)
        return false;
    if (name[0] != '#' && name[0] != '&')
        return false;
    for (size_t i = 1; i < name.length(); i++)
    {
        if (name[i] == ' ' || name[i] == ',' || name[i] == 7 || name[i] == '\n' || name[i] == '\r' || name[i] == '\t')
            return false;
    }
    return true;
}

void sendNamesList(Client &client, std::string channelName, const std::set<int> clientsInChannel)
{
    std::string nameList;

    for (std::set<int>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); it++)
    {
        const Client &chanClient = clients_bj.get_client(*it);
        if (!nameList.empty())
            nameList += " ";
        if (g_channels.isOperator(channelName, *it))
            nameList += "@";
        nameList += chanClient.nickname;
    }
    send_msg(client.fd, ":server 353 " + client.nickname + " = " + channelName + " :" + nameList + "\r\n");
    send_msg(client.fd, ":server 366 " + client.nickname + " " + channelName + " :End of NAMES list\r\n");
}

void join(Client &client, std::string args)
{
    std::string channelsStr, keysStr;
    std::istringstream iss(args);
    iss >> channelsStr >> keysStr;

    // LOGS DEBUG
    std::cout << "[JOIN DEBUG] args: \"" << args << "\"" << std::endl;
    std::cout << "[JOIN DEBUG] channelsStr: \"" << channelsStr << "\"" << std::endl;
    std::cout << "[JOIN DEBUG] keysStr: \"" << keysStr << "\"" << std::endl;//

    std::vector<std::string> channels = split(channelsStr, ',');
    std::vector<std::string> keys = split(keysStr, ',');

    // LOGS DEBUG
    std::cout << "[JOIN DEBUG] Parsed channels:" << std::endl;
    for (size_t i = 0; i < channels.size(); ++i)
        std::cout << "  [" << i << "] = \"" << channels[i] << "\"" << std::endl;
    std::cout << "[JOIN DEBUG] Parsed keys:" << std::endl;
    for (size_t i = 0; i < keys.size(); ++i)
        std::cout << "  [" << i << "] = \"" << keys[i] << "\"" << std::endl;//

    if (channels.empty() || channels[0].empty())
    {
        send_msg(client.fd, ":server 461 " + client.nickname + " JOIN :Not enough parameters\r\n");
        return;
    }

    for (size_t i = 0; i < channels.size(); ++i)
    {
        const std::string &channelName = channels[i];
        const std::string passw = (i < keys.size()) ? keys[i] : "";

        if (!isValidChannelName(channelName))
        {
            send_msg(client.fd, ":server 403 " + client.nickname + " " + channelName + " :No such channel\r\n");
            continue;
        }

        if (!g_channels.channelExists(channelName))
            g_channels.createChannel(channelName, client.fd, passw);
        else 
        {
            bool hasValidPass = g_channels.checkPassword(channelName, passw);
            bool isInvited = g_channels.isInvited(channelName, client.fd);
            bool isPrivate = g_channels.hasMode(channelName, 'i');
            bool hasKey = g_channels.hasMode(channelName, 'k');
            int limit = g_channels.getUserLimit(channelName);
            size_t currentMembers = g_channels.getClientsInChannel(channelName).size();

            if (hasKey && !hasValidPass && !isInvited)
            {
                send_msg(client.fd, ":server 475 " + client.nickname + " " + channelName + " :Cannot join channel (+k)\r\n");
                continue;
            }
            if (isPrivate && !isInvited)
            {
                send_msg(client.fd, ":server 473 " + client.nickname + " " + channelName + " :Cannot join channel (+i)\r\n");
                continue;
            }
            if (limit != -1 && static_cast<int>(currentMembers) >= limit)
            {
                send_msg(client.fd, ":server 471 " + client.nickname + " " + channelName + " :Cannot join channel (+l) - channel is full\r\n");
                continue;
            }
            if (!g_channels.addClientToChannel(channelName, client.fd))
            {
                // send_msg(client.fd, ":server 443 " + client.nickname + " " + channelName + " :is already on channel\r\n"); is not in numeric repli
                continue;
            }
            if (isInvited)
                g_channels.removeInvite(channelName, client.fd);
        }

        const std::set<int> &clientsInChannel = g_channels.getClientsInChannel(channelName);
        for (std::set<int>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); ++it)
        {
            int fd = *it;
            send_msg(fd, ":" + client.nickname + " JOIN :" + channelName + "\r\n");
        }

        std::string topic = g_channels.getTopic(channelName);
        if (!topic.empty())
            send_msg(client.fd, ":server 332 " + client.nickname + " " + channelName + " :" + topic + "\r\n");

        sendNamesList(client, channelName, clientsInChannel);
    }
}

void topic(Client &client, std::string args)
{
    std::istringstream iss(args);
    std::string channelName;
    std::string newTopic;

    iss >> channelName;
    std::getline(iss, newTopic);
    if (!newTopic.empty() && newTopic[0] == ' ')
        newTopic.erase(0, 1);

    if (channelName.empty())
    {
        send_msg(client.fd, ":server 461 " + client.nickname + " TOPIC :Not enough parameters\r\n");
        return;
    }
    if (!g_channels.isClientInChannel(channelName, client.fd))
    {
        send_msg(client.fd, ":server 442 " + client.nickname + " " + channelName + " :You're not on that channel\r\n");
        return;
    }
    if (g_channels.hasMode(channelName, 't') && !g_channels.isOperator(channelName, client.fd) && !newTopic.empty())
    {
        send_msg(client.fd, ":server 482 " + client.nickname + " " + channelName + " :You're not channel operator\r\n");
        return;
    }
    if (!newTopic.empty())
    {
        g_channels.setTopic(channelName, newTopic);
        const std::set<int> &clients = g_channels.getClientsInChannel(channelName);

        for (std::set<int>::const_iterator it = clients.begin(); it != clients.end(); ++it)
            send_msg(*it, ":" + client.nickname + " TOPIC " + channelName + " :" + newTopic + "\r\n");
    }
    else
        if(!g_channels.hasTopic(channelName))
            send_msg(client.fd, ":server 331 " + client.nickname + " " + channelName + " :No topic is set\r\n");
        else
            send_msg(client.fd, ":server 332 " + client.nickname + " " + channelName + " :" + g_channels.getTopic(channelName) + "\r\n");
}

void invite(Client &client, std::string args)
{
    std::istringstream iss(args);
    std::string nick;
    std::string channelName;
    int target_fd = 0;

    iss >> nick >> channelName;

    if (nick.empty() || channelName.empty())
       return send_msg(client.fd, ":server 461 " + client.nickname + " INVITE :Not enough parameters\r\n");
    if (!g_channels.channelExists(channelName))
        return send_msg(client.fd, ":server 401 " + client.nickname + " " + nick + " :No such channel\r\n");
    if (!g_channels.isClientInChannel(channelName, client.fd))
        return send_msg(client.fd, ":server 442 " + client.nickname + " " + channelName + " :You're not on that channel\r\n");
    if (g_channels.hasMode(channelName, 'i') && !g_channels.isOperator(channelName, client.fd))
        return send_msg(client.fd, ":server 482 " + client.nickname + " " + channelName + " :You're not channel operator\r\n");
    if (clients_bj.nickExists(nick))
        target_fd = clients_bj.get_fd_of(nick);
    else {
        return send_msg(client.fd, ":server 401 " + client.nickname + " " + nick + " :No such nick\r\n");
    }
    if (g_channels.isClientInChannel(channelName, target_fd))
        return send_msg(client.fd, ":server 443 " + client.nickname + " " + nick + " " + channelName + " :is already on channel\r\n");
    
    int limit = g_channels.getUserLimit(channelName);
    size_t currentMembers = g_channels.getClientsInChannel(channelName).size();

    if (limit != -1 && static_cast<int>(currentMembers) >= limit)
        return send_msg(client.fd, ":server 471 " + client.nickname + " " + channelName + " :Cannot invite channel (+l) - channel is full\r\n");

    g_channels.inviteClient(channelName, target_fd);
    send_msg(client.fd, ":server 341 " + client.nickname + " " + nick + " " + channelName + "\r\n");
    send_msg(target_fd, ":" + client.nickname + " INVITE " + nick + " :" + channelName + "\r\n");
}

void kick(Client &client, std::string args)
{
    std::istringstream iss(args);
    std::string channelsStr, nick, comment;
    iss >> channelsStr >> nick;
    int target_fd = 0;

    // LOGS DEBUG
    std::cout << "[KICK DEBUG] args: \"" << args << "\"" << std::endl;
    std::cout << "[KICK DEBUG] channelsStr: \"" << channelsStr << "\"" << std::endl;
    std::cout << "[KICK DEBUG] nick: \"" << nick << "\"" << std::endl;//

    std::vector<std::string> channels = split(channelsStr, ',');
    std::getline(iss, comment);

    // LOGS DEBUG
    std::cout << "[KICK DEBUG] Parsed channels:" << std::endl;
    for (size_t i = 0; i < channels.size(); ++i)
        std::cout << "  [" << i << "] = \"" << channels[i] << "\"" << std::endl;
    std::cout << "[KICK DEBUG] Parsed nick:" << std::endl;
    std::cout << "' "<< nick << " '" << std::endl;
    std::cout << "[KICK DEBUG] Parsed nicks:" << std::endl;
    std::cout << "' "<< comment << " '" << std::endl;//

    if (channelsStr.empty() || nick.empty())
        return send_msg(client.fd, ":server 461 " + client.nickname + " KICK :Not enough parameters\r\n");
    if (!clients_bj.nickExists(nick))
        return;
    target_fd = clients_bj.get_fd_of(nick);

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (!g_channels.channelExists(channels[i]))
        {
            send_msg(client.fd, ":server 403 " + client.nickname + " " + channels[i] + " :No such channel\r\n");
            continue;
        }
        if (!g_channels.isClientInChannel(channels[i], client.fd))
        {
            send_msg(client.fd, ":server 442 " + client.nickname + " " + channels[i] + " :You're not on that channel\r\n");
            continue;
        }
        if (!g_channels.isClientInChannel(channels[i], target_fd))
        {
            send_msg(client.fd, ":server 442 " + client.nickname + " " + channels[i] + " :They’re not on that channel\r\n");
            continue;
        }
        if (!g_channels.isOperator(channels[i], client.fd))
        {
            send_msg(client.fd, ":server 482 " + client.nickname + " " + channels[i] + " :You're not channel operator\r\n");
            continue;
        }
        if (comment.empty())
            comment = "Kicked";
        std::string kick_msg = ":" + client.nickname + " KICK " + channels[i] + " " + nick + " " + comment + "\r\n";
        std::cout << "' kick_msg = "<< kick_msg << " '" << std::endl;
        const std::set<int> clientsInChannel = g_channels.getClientsInChannel(channels[i]);
        for (std::set<int>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); ++it)
        {
            int fd = *it;
            send_msg(fd, kick_msg);
        }
        g_channels.removeClientFromChannel(channels[i], target_fd);
    }
}

bool isValidModeString(Client &client, const std::string& modes)
{
    if (modes.empty())
        return false;
    if (modes[0] != '+' && modes[0] != '-')
        return false;

    const std::string validModes = "itklo";
    for (size_t i = 1; i < modes.size(); ++i)
    {
        if (validModes.find(modes[i]) == std::string::npos)
        {
            send_msg(client.fd, ":server 472 " + client.nickname + " " + modes[i] + " :is unknown mode char to me\r\n");
            return false;
        }
    }
    return true;
}

bool isValidLimit(const std::string &param)
{
    if (param.empty())
        return false;

    for (size_t i = 0; i < param.length(); ++i)
    {
        if (!std::isdigit(param[i]))
            return false;
    }

    std::istringstream iss(param);
    long long value;
    iss >> value;

    return value > 0 && value <= INT_MAX;
}

void mode(Client &client, std::string args)
{
    (void)client;
    std::istringstream iss(args);
    std::string channelName, modes, argStr;
    iss >> channelName >> modes;

    std::getline(iss, argStr);
    std::vector<std::string> param = split(argStr, ' ');

    std::cout << "[MODE DEBUG] args: \"" << args << "\"" << std::endl;
    std::cout << "[MODE DEBUG] channelName: \"" << channelName<< "\"" << std::endl;
    std::cout << "[MODE DEBUG] modes: \"" << modes << "\"" << std::endl;
    std::cout << "[MODE DEBUG] Parsed Param:" << std::endl;
    for (size_t i = 0; i < param.size(); ++i)
        std::cout << "  [" << i << "] = \"" << param[i] << "\"" << std::endl;

    if (channelName.empty())
        return send_msg(client.fd, ":server 461 " + client.nickname + " MODE :Not enough parameters\r\n");
    if (!g_channels.channelExists(channelName))
        return send_msg(client.fd, ":server 403 " + client.nickname + " " + channelName + " :No such channel\r\n");
    if (!g_channels.isClientInChannel(channelName, client.fd))
        return send_msg(client.fd, ":server 442 " + client.nickname + " " + channelName + " :You're not on that channel\r\n");
    if (!g_channels.isOperator(channelName, client.fd))
        return send_msg(client.fd, ":server 482 " + client.nickname + " " + channelName + " :You're not channel operator\r\n");
    if (modes.empty()) {
        std::string modeStr = g_channels.getModeString(channelName);
        return send_msg(client.fd, ":server 324 " + client.nickname + " " + channelName + " " + modeStr + "\r\n");
    }
    if (!isValidModeString(client, modes))
        return;

    bool adding = true;
    std::string modesStr = "+";
    std::string paramStr;
    size_t paramIndex = 0;

    if (modes[0] == '-')
    {
        adding = false;
        modesStr[0] = '-';
    }

    for (size_t i = 1; i < modes.size(); ++i)
    {
        char mode = modes[i];
        switch (mode)
        {
            case 'i':
            case 't':
                if (adding)
                    g_channels.addMode(channelName, mode);
                else
                    g_channels.removeMode(channelName, mode);
                modesStr += mode;
                break;

            case 'k':
                if (adding)
                {
                    if (paramIndex >= param.size())
                        return send_msg(client.fd, ":server 461 " + client.nickname + " MODE :Not enough parameters\r\n");
                    g_channels.setPassword(channelName, param[paramIndex]);
                    g_channels.addMode(channelName, 'k');
                    modesStr += "k";
                    paramStr += param[paramIndex] + " ";
                    ++paramIndex;
                }
                else
                {
                    g_channels.removePassword(channelName);
                    g_channels.removeMode(channelName, 'k');
                    modesStr += "k";
                }
                break;

            case 'l':
                if (adding)
                {
                    if (paramIndex >= param.size() || !isValidLimit(param[paramIndex]))
                        return send_msg(client.fd, ":server 461 " + client.nickname + " MODE :Not enough parameters\r\n");
                    int limit = std::atoi(param[paramIndex].c_str());
                    g_channels.setUserLimit(channelName, limit);
                    g_channels.addMode(channelName, 'l');
                    modesStr += "l";
                    paramStr += param[paramIndex] + " ";
                    ++paramIndex;
                }
                else
                {
                    g_channels.removeUserLimit(channelName);
                    g_channels.removeMode(channelName, 'l');
                    modesStr += "l";
                }
                break;
            case 'o':
                if (paramIndex >= param.size())
                    return send_msg(client.fd, ":server 461 " + client.nickname + " MODE :Not enough parameters\r\n");
                if (!clients_bj.nickExists(param[paramIndex]))
                    return send_msg(client.fd, ":server 401 " + client.nickname + " " + param[paramIndex] + " :No such nick\r\n");
                int target_fd = clients_bj.get_fd_of(param[paramIndex]);
                if (adding)
                    g_channels.addOperator(channelName, target_fd);
                else
                    g_channels.removeOperator(channelName, target_fd);
                paramStr += param[paramIndex] + " ";
                ++paramIndex;
        }
    }
    
    if (modesStr.size() <= 1)
        return;
    std::string msg = ":" + client.nickname + "!user@host MODE " + channelName + " " + modesStr;
    if (!paramStr.empty())
        msg += " " + paramStr;
    msg += "\r\n";
    const std::set<int> clientsInChannel = g_channels.getClientsInChannel(channelName);
    for (std::set<int>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); ++it)
    {
        int fd = *it;
        send_msg(fd, msg);
    }
}

/*

MODE
221 RPL_UMODEIS          "<user mode string>"                       ?
324 RPL_CHANNELMODEIS    "<channel> <mode> <mode params>"           ?
367 RPL_BANLIST          "<channel> <banid>"                        ?
368 RPL_ENDOFBANLIST                                                ?
401 ERR_NOSUCHNICK       "<nickname> :No such nick/channel"
403 ERR_NOSUCHCHANNEL    "<channel name> :No such channel"          X
442 ERR_NOTONCHANNEL     "<channel> :You're not on that channel"    X
461 ERR_NEEDMOREPARAMS   "<command> :Not enough parameters"
467 ERR_KEYSET           "<channel> :Channel key already set"
472 ERR_UNKNOWNMODE      "<char> :is unknown mode char to me"       x
482 ERR_CHANOPRIVSNEEDED "<channel> :You're not channel operator"
501 ERR_UMODEUNKNOWNFLAG ":Unknown MODE flag"                       ?
502 ERR_USERSDONTMATCH   ":Cant change mode for other users"        ?

JOIN
403 ERR_NOSUCHCHANNEL  "No such channel"
461 ERR_NEEDMOREPARAMS "Not enough parameters"
471 ERR_CHANNELISFULL  "Cannot join channel (+l)" manque lui
473 ERR_INVITEONLYCHAN "Cannot join channel (+i)"
475 ERR_BADCHANNELKEY  "Cannot join channel (+k)"

405 ERR_TOOMANYCHANNELS "You have joined too many channels" optionnal


TOPIC
331 No topic is set
send_msg(client.fd, ":server 331 " + client.nickname + " " + channelName + " :No topic is set\r\n");
332 <topic>
send_msg(client.fd, ":server 332 " + client.nickname + " " + channelName + " :" + topic + "\r\n");
442 You're not on that channel
send_msg(client.fd, ":server 442 " + client.nickname + " " + channelName + " :You're not on that channel\r\n");
461 Not enough parameters
send_msg(client.fd, ":server 461 " + client.nickname + " TOPIC :Not enough parameters\r\n");
482 You're not channel operator
send_msg(client.fd, ":server 482 " + client.nickname + " " + channelName + " :You're not channel operator\r\n");

INVITE
401 ERR_NOSUCHNICK       "<nickname> :No such nick/channel"           X
443 ERR_USERONCHANNEL    "<user> <channel> :is already on channel"    X
442 ERR_NOTONCHANNEL     "<channel> :You're not on that channel"      x
461 ERR_NEEDMOREPARAMS   "<command> :Not enough parameters"           x
482 ERR_CHANOPRIVSNEEDED "<channel> :You're not channel operator"     x
301 RPL_AWAY             "<nick> :<away message>"                     optionnal
341 RPL_INVITING         "<channel> <nick>"                           x

KICK
403 ERR_NOSUCHCHANNEL    "<channel name> :No such channel"            x
442 ERR_NOTONCHANNEL     "<channel> :You're not on that channel"      x
461 ERR_NEEDMOREPARAMS   "<command> :Not enough parameters"           x
482 ERR_CHANOPRIVSNEEDED "<channel> :You're not channel operator"     x

exemple: send_msg(client.fd, ":" SERVER_NAME " 401 " + client.nickname + " " + reciever + " :No such nick/channel\r\n");
*/