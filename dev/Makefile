CXXFLAGS		:=	-Wall -Wextra -Werror -std=c++98 --pedantic -g -fsanitize=address
SRCS_SERVER		:=	server.cpp asocket.cpp\
					listeningsocket.cpp\
					connectedsocket.cpp\
					eventselectloop.cpp\
					eventpollloop.cpp\
					test_common.cpp

OBJS_SERVER		:=	$(SRCS_SERVER:.cpp=.o)
SRCS_CLIENT		:=	client.cpp asocket.cpp\
					listeningsocket.cpp\
					connectedsocket.cpp\
					eventselectloop.cpp\
					test_common.cpp
OBJS_CLIENT		:=	$(SRCS_CLIENT:.cpp=.o)

NAME_CLIENT		:= webclient
NAME_SERVER		:= webserv
NAMES 			:= $(NAME_CLIENT) $(NAME_SERVER)
RM				:= rm -rf

.PHONY			:	all
all				:	$(NAMES)

$(NAME_SERVER):	$(OBJS_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(NAME_CLIENT):	$(OBJS_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	$(RM) $(OBJS_SERVER) $(OBJS_CLIENT)

fclean: clean
	$(RM) $(NAMES)

re: fclean all
