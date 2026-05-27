NAME = libftprintf.a

CC     = cc
CFLAGS = -Wall -Wextra -fPIC

LIBFT = ../libft/libft.a

SRCS = ft_printf.c \
       ft_putchar.c \
       ft_flagcheck.c \
       ft_putstr.c \
       ft_putnbr_base.c \
       ft_puthex.c \
       ft_putpointer.c \
       ft_numlen_base.c
OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(LIBFT):
	$(MAKE) -C ../libft

$(NAME): $(LIBFT) $(OBJS)
	ar rcs $(NAME) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
