# =========================
# CONFIG
# =========================

HOSTTYPE ?= $(shell uname -m)_$(shell uname -s)

NAME = libft_malloc_$(HOSTTYPE).so
LINK = libft_malloc.so

CC = cc
CFLAGS = -Wall -Wextra -fPIC

LIBFT = libft/libft.a

SRCS = src/malloc.c src/free.c src/realloc.c src/show_alloc_mem.c src/utils.c
OBJS = $(SRCS:.c=.o)

TEST_SRC = src/test.c
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
	$(CC) $(CFLAGS) -shared $(OBJS) -Llibft -lft -o $(NAME)
	ln -sf $(NAME) $(LINK)

# compile objects
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# =========================
# TEST BINARY
# =========================

test: $(NAME)
	$(CC) $(CFLAGS) $(TEST_SRC) -Llibft -lft -L. -lft_malloc -Wl,-rpath=. -o $(TEST_BIN)

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

.PHONY: all clean fclean re test