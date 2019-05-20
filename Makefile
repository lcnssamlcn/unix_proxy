C = gcc
CFLAGS = -Wall -O3
SRCDIR = src
SRC = server.c HTTPHeader.c HTTPProxyRequest.c HTTPProxyResponse.c err_doc.c utilities.c
EXEC = server
OBJDIR = obj
OBJ = $(addprefix $(OBJDIR)/,$(SRC:.c=.o))


all: make_objdir $(OBJ)
	$(C) $(CFLAGS) $(OBJ) -o $(EXEC)

make_objdir: 
	[ ! -e $(OBJDIR) ] && mkdir $(OBJDIR) || true

$(OBJDIR)/server.o:
	$(C) $(CFLAGS) -c $(SRCDIR)/server.c -o $(OBJDIR)/server.o

$(OBJDIR)/HTTPHeader.o:
	$(C) $(CFLAGS) -c $(SRCDIR)/HTTPHeader.c -o $(OBJDIR)/HTTPHeader.o

$(OBJDIR)/HTTPProxyRequest.o:
	$(C) $(CFLAGS) -c $(SRCDIR)/HTTPProxyRequest.c -o $(OBJDIR)/HTTPProxyRequest.o

$(OBJDIR)/HTTPProxyResponse.o:
	$(C) $(CFLAGS) -c $(SRCDIR)/HTTPProxyResponse.c -o $(OBJDIR)/HTTPProxyResponse.o

$(OBJDIR)/err_doc.o:
	$(C) $(CFLAGS) -c $(SRCDIR)/err_doc.c -o $(OBJDIR)/err_doc.o

$(OBJDIR)/utilities.o:
	$(C) $(CFLAGS) -c $(SRCDIR)/utilities.c -o $(OBJDIR)/utilities.o

.PHONY: clean
clean:
	[ -e $(OBJDIR) ] && rm -R $(OBJDIR) || true
	[ -e $(EXEC) ] && rm $(EXEC) || true
