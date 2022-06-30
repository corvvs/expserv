CXXFLAGS		:=	-Wall -Wextra -Werror -std=c++98 --pedantic -g -fsanitize=address
SRCS_SERVER		:=	server.cpp asocket.cpp\
					socketlistening.cpp\
					socketconnected.cpp\
					SocketUNIX.cpp\
					eventselectloop.cpp\
					eventpollloop.cpp\
					eventkqueueloop.cpp\
					channel.cpp\
					connection.cpp\
					cgi.cpp\
					requesthttp.cpp\
					responsehttp.cpp\
					parserhelper.cpp\
					HeaderHTTP.cpp\
					ValidatorHTTP.cpp\
					ControlHeaderHTTP.cpp\
					IndexRange.cpp\
					time.cpp\
					http_error.cpp\
					http.cpp\
					CharFilter.cpp\
					ChunkedBody.cpp\
					httpserver.cpp\
					utils_string.cpp\
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
CF				:= clang-format -i

.PHONY			:	all
all				:	$(NAMES)

cf_server:
	$(CF) $(SRCS_SERVER)

cf_client:
	$(CF) $(SRCS_CLIENT)

$(NAME_SERVER): cf_server $(OBJS_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS_SERVER)

$(NAME_CLIENT): cf_client $(OBJS_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS_CLIENT)

clean:
	$(RM) $(OBJS_SERVER) $(OBJS_CLIENT)

fclean: clean
	$(RM) $(NAMES)

re: fclean all

up:	down all
	./$(NAME_SERVER) &

down:
	pkill $(NAME_SERVER) || :
