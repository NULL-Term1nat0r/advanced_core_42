# =========================
# CONFIG
# =========================

HOSTTYPE ?= $(shell uname -m)_$(shell uname -s)

NAME = libft_malloc_$(HOSTTYPE).so
LINK = libft_malloc.so

CC = cc
CFLAGS = -Wall -Wextra -fPIC
TEST_CFLAGS = -Wall -Wextra

LIBFT = libft/libft.a

SRCS = src/malloc.c src/list.c src/show_alloc_mem.c
BONUS_SRCS = bonus/src/malloc_bonus.c bonus/src/list_bonus.c bonus/src/show_alloc_mem.c
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

# build shared library
$(NAME): $(LIBFT) $(OBJS)
	$(CC) $(CFLAGS) -shared $(OBJS) -Llibft -lft -lpthread -o $(NAME)
	ln -sf $(NAME) $(LINK)

# compile objects
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bonus: $(LIBFT) $(BONUS_OBJS)
	$(CC) $(CFLAGS) -shared $(BONUS_OBJS) -Llibft -lft -lpthread -o $(NAME)
	ln -sf $(NAME) $(LINK)

# =========================
# TEST BINARY
# =========================

test: $(LIBFT)
	$(CC) $(TEST_CFLAGS) $(TEST_SRC) $(SRCS) -Llibft -lft -lpthread -o $(TEST_BIN)

test_bonus: $(LIBFT)
	$(CC) $(TEST_CFLAGS) $(TEST_BONUS_SRC) $(BONUS_SRCS) -Llibft -lft -lpthread -o $(TEST_BIN_BONUS)



# =========================
# CLEAN
# =========================

clean:
	$(MAKE) -C libft clean
	rm -f $(OBJS)

fclean: clean
	$(MAKE) -C libft fclean
	rm -f $(NAME) $(LINK) $(TEST_BIN)

re: fclean all

.PHONY: all bonus clean fclean re test test_bonus