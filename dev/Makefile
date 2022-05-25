CXXFLAGS		:=	-Wall -Wextra -Werror -std=c++98 --pedantic -g -fsanitize=address
SRCS_SERVER		:= server.cpp socket.cpp test_common.cpp
OBJS_SERVER		:=	$(SRCS_SERVER:.cpp=.o)
SRCS_CLIENT		:= client.cpp socket.cpp test_common.cpp
OBJS_CLIENT		:=	$(SRCS_CLIENT:.cpp=.o)

NAME_CLIENT		:= client
NAME_SERVER		:= server
NAMES 			:= $(NAME_CLIENT) $(NAME_SERVER)
RM				:= rm -rf

.PHONY			:	all
all				:	$(NAMES)

$(NAME_SERVER):	$(OBJS_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(NAME_CLIENT):	$(OBJS_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAMES)

re: fclean all
