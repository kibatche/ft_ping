GCC = gcc

RM = rm -f

CCFLAGS = -Wall -Wextra -Werror

ping_SOURCES = ./ping.c ping_common.c ping_echo.c ping_address.c \
  ping_router.c ping_timestamp.c ping_common.h  ping_impl.h ping.h libping.c

OBJS = ${ping_SOURCES:.c=.o}
NAME = pping

INCL = -I ./

all: ${NAME}

${NAME}: ${OBJS}
	${GCC} ${CCFLAGS} ${OBJS} -g -o ${NAME} ${INCL}

%.o: %.c
	${GCC} ${CCFLAGS} -g -c $< -o $@ ${INCL}

clean:
	${RM} ${OBJS}
	@make -C clean

fclean: clean
	${RM} ${NAME}
	@make -C fclean

re: fclean all

.PHONY:	clean re fclean


