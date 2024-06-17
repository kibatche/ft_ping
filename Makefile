CC = gcc

SRC = ft_ping.c utils.c
CFLAGS = -Wall -Wextra -Werror
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
	$(CC) -o $@ -c $< $(CFLAGS) -Iincludes

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm $(NAME)

re: fclean all

.PHONY: clean re all fclean ft_ping