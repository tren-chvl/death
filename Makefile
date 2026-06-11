NAME		= Death
CC			= cc
CFLAGS		= -Wall -Wextra -Werror -I include -g3 -no-pie
SRCDIR		= src
OBJDIR		= obj

SRC         = $(SRCDIR)/main.c \
			  $(SRCDIR)/death.c \
			  $(SRCDIR)/find.c \
			  $(SRCDIR)/detect.c \
			  $(SRCDIR)/inject.c \
			  $(SRCDIR)/anti_process.c \
			  $(SRCDIR)/utils.c \
			  $(SRCDIR)/rodata.c \
			  $(SRCDIR)/persistence.c \
			  $(SRCDIR)/metamorphic.c

OBJ			= $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

all: $(NAME) war_helper_install

war_helper:
	$(MAKE) -C war_helper

war_helper_install: war_helper
	@mkdir -p /tmp/war_helper
	@cp war_helper/war_helper /tmp/war_helper/
	@chmod +x /tmp/war_helper/war_helper

$(NAME): $(OBJ) client.o server.o
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)
	$(CC) $(CFLAGS) client.o -o client
	$(CC) $(CFLAGS) server.o -o server

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)
	$(MAKE) -C war_helper clean || true
	rm -f client.o
	rm -f server.o

fclean: clean
	rm -f $(NAME)
	rm -f /tmp/war_helper/war_helper
	rm -f client
	rm -f server

re: fclean all

.PHONY: all clean fclean re war_helper war_helper_install