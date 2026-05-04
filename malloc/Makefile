# =========================
# CONFIG
# =========================

HOSTTYPE ?= $(shell uname -m)_$(shell uname -s)

NAME = libft_malloc_$(HOSTTYPE).so
LINK = libft_malloc.so

CC = cc
CFLAGS = -Wall -Wextra  -fPIC

LIBFT = libft/libft.a

SRCS = src/malloc.c src/free.c src/realloc.c src/show_alloc_mem.c src/utils.c
OBJS = $(SRCS:.c=.o)

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

clean:
	$(MAKE) -C libft clean
	rm -f $(OBJS)

fclean: clean
	$(MAKE) -C libft fclean
	rm -f $(NAME) $(LINK)

re: fclean all

.PHONY: all clean fclean re