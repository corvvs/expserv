CXXFLAGS		:=	-Wall -Wextra -Werror -std=c++98 --pedantic -g -fsanitize=address
SRCS_SERVER		:=	server.cpp asocket.cpp\
					socketlistening.cpp\
					socketconnected.cpp\
					eventselectloop.cpp\
					eventpollloop.cpp\
					eventkqueueloop.cpp\
					channel.cpp\
					connection.cpp\
					requesthttp.cpp\
					responsehttp.cpp\
					parserhelper.cpp\
					HeaderHTTPItem.cpp\
					IndexRange.cpp\
					time.cpp\
					http_error.cpp\
					http.cpp\
					httpserver.cpp\
					test_common.cpp

OBJS_SERVER		:=	$(SRCS_SERVER:.cpp=.o)
SRCS_CLIENT		:=	client.cpp asocket.cpp\
					socketlistening.cpp\
					socketconnected.cpp\
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

up:	down all
	./$(NAME_SERVER) &

down:
	pkill $(NAME_SERVER) || :
