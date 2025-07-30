NAME = ircserv
BONUS = bot
CPPFLAGS = -Wall -Wextra -Werror
INCLUDES = -I ./
SRCS = irc.cpp commands.cpp irc_parsing_and_init.cpp Clients.cpp Channels.cpp commands_channel.cpp
OBJS = $(SRCS:.cpp=.o)
BSRCS = warningbot.cpp
BOBJS = $(BSRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	c++ $(CPPFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	c++ $(CPPFLAGS) $(INCLUDES) -c $< -o $@

bonus: $(BONUS)

$(BONUS): $(BOBJS)
	c++ $(CPPFLAGS) $(BOBJS) -o $(BONUS)

%.o: %.cpp
	c++ $(CPPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)
	rm -f $(BOBJS)

fclean: clean
	rm -f $(NAME)
	rm -f $(BONUS)

re: fclean all

.PHONY: all bonus clean fclean re
