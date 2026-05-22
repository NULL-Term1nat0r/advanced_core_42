# =========================
# CONFIG
# =========================

HOSTTYPE ?= $(shell uname -m)_$(shell uname -s)

NAME = libft_malloc_$(HOSTTYPE).so
LINK = libft_malloc.so

CC = cc
CFLAGS = -Wall -Wextra -fPIC
TEST_CFLAGS = -Wall -Wextra

LIBFT  = libft/libft.a
PRINTF = printf/libftprintf.a

PRINTF_SRCS = printf/ft_printf.c \
              printf/ft_putchar.c \
              printf/ft_flagcheck.c \
              printf/ft_putstr.c \
              printf/ft_putnbr_base.c \
              printf/ft_puthex.c \
              printf/ft_putpointer.c \
              printf/ft_numlen_base.c
PRINTF_OBJS = $(PRINTF_SRCS:.c=.o)

SRCS = src/malloc.c src/list.c src/show_alloc_mem.c
BONUS_SRCS = bonus/src/malloc_bonus.c bonus/src/list_bonus.c bonus/src/show_alloc_mem.c bonus/src/mutex_bonus.c
OBJS = $(SRCS:.c=.o)
BONUS_OBJS = $(BONUS_SRCS:.c=.o)
TEST_SRC = src/test.c
TEST_BONUS_SRC = bonus/src/test_bonus.c
TEST_BIN_BONUS = test_bin_bonus
TEST_BIN = test_bin

# =========================
# RULES
# =========================

all: $(NAME)

# build libft first
$(LIBFT):
	$(MAKE) -C libft

# build printf library
$(PRINTF): $(LIBFT)
	$(MAKE) -C printf

# build shared library
$(NAME): $(LIBFT) $(PRINTF) $(OBJS) $(PRINTF_OBJS)
	$(CC) $(CFLAGS) -shared $(OBJS) $(PRINTF_OBJS) -Llibft -lft -lpthread -o $(NAME)
	ln -sf $(NAME) $(LINK)

# compile objects
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bonus: $(LIBFT) $(PRINTF) $(BONUS_OBJS) $(PRINTF_OBJS)
	$(CC) $(CFLAGS) -shared $(BONUS_OBJS) $(PRINTF_OBJS) -Llibft -lft -lpthread -o $(NAME)
	ln -sf $(NAME) $(LINK)

# =========================
# TEST BINARY
# =========================

test: $(LIBFT) $(PRINTF)
	$(CC) $(TEST_CFLAGS) $(TEST_SRC) $(SRCS) $(PRINTF_SRCS) -Llibft -lft -lpthread -o $(TEST_BIN)

test_bonus: $(LIBFT) $(PRINTF)
	$(CC) $(TEST_CFLAGS) $(TEST_BONUS_SRC) $(BONUS_SRCS) $(PRINTF_SRCS) -Llibft -lft -lpthread -o $(TEST_BIN_BONUS)



# =========================
# CLEAN
# =========================

clean:
	$(MAKE) -C libft clean
	$(MAKE) -C printf clean
	rm -f $(OBJS) $(BONUS_OBJS) $(PRINTF_OBJS)

fclean: clean
	$(MAKE) -C libft fclean
	$(MAKE) -C printf fclean
	rm -f $(NAME) $(LINK) $(TEST_BIN) $(TEST_BIN_BONUS)

re: fclean all

.PHONY: all bonus clean fclean re test test_bonus