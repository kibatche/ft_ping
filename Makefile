CC = gcc
RM = rm -rf

SRC = ft_ping.c utils.c
CFLAGS = -Wall -Wextra -Werror -g
NAME = ft_ping

SRCDIR = ./src
OBJDIR = ./obj

OBJS = $(addprefix $(OBJDIR)/,$(SRC:.c=.o))
DEPS = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME) -Iincludes

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $< $(CFLAGS) -Iincludes

clean:
	$(RM) $(OBJDIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: clean re all fclean