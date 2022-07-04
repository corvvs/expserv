CXXFLAGS		:=	-Wall -Wextra -Werror -std=c++98 --pedantic -g -fsanitize=address
MAIN_TEST		:=	cgi_test.cpp
MAIN_SERVER		:=	server.cpp
MODS_SERVER		:=	asocket.cpp\
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
SRCS_SERVER		:=	$(MAIN_SERVER) $(MODS_SERVER)
OBJS_SERVER		:=	$(SRCS_SERVER:.cpp=.o)
SRCS_CLIENT		:=	client.cpp asocket.cpp\
					socketlistening.cpp\
					socketconnected.cpp\
					test_common.cpp
OBJS_CLIENT		:=	$(SRCS_CLIENT:.cpp=.o)
SRCS_TEST		:=	$(MAIN_TEST) $(MODS_SERVER)
OBJS_TEST		:=	$(SRCS_TEST:.cpp=.o)


NAME_TEST		:=	testserv
NAME_CLIENT		:=	webclient
NAME_SERVER		:=	webserv
NAMES 			:=	$(NAME_CLIENT) $(NAME_SERVER)
RM				:=	rm -rf
CF				:=	clang-format -i

.PHONY			:	all
all				:	cf $(NAMES)

cf:
	$(CF) *.{c,h}pp

$(NAME_SERVER): $(OBJS_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(NAME_TEST): $(OBJS_TEST)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(NAME_CLIENT): $(OBJS_CLIENT)
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
