.SUFFIXES:
.SUFFIXES: .o .a .c

# Where to install the executables
insdir = ../../bin

PROG = ottoxlat

SRCS = ottoxlat.c


OBJS = $(SRCS:.c=.o)

LDPATHS = -L../lib

LDLIBS = -lottolib

CFLAGS=-g -Wall -Werror -Wno-unknown-pragmas -Wno-char-subscripts -I../include

all: $(PROG)
	@echo Make $(PROG) done

install: $(PROG)
	@echo cp $(PROG) $(insdir)
	@rm -f $(insdir)/$(PROG)
	@cp $(PROG) $(insdir)
	@chmod 755 $(insdir)/$(PROG) $(insdir)

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDPATHS) $(LDLIBS)

clean::
	/bin/rm -f *.o $(PROG)

