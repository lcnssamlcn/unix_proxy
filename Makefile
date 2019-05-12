C = gcc
CFLAGS = -Wall -O3
SRCDIR = src
SRC = server.c
EXEC = server
OBJDIR = obj
OBJ = $(addprefix $(OBJDIR)/,$(SRC:.c=.o))


all: make_objdir $(OBJ)
	$(C) $(CFLAGS) $(OBJ) -o $(EXEC)

make_objdir: 
	[ ! -e $(OBJDIR) ] && mkdir $(OBJDIR) || true

$(OBJDIR)/server.o:
	$(C) $(CFLAGS) -c $(SRCDIR)/server.c -o $(OBJDIR)/server.o

.PHONY: clean
clean:
	[ -e $(OBJDIR) ] && rm -R $(OBJDIR) || true
	[ -e $(EXEC) ] && rm $(EXEC) || true
