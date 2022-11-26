.SUFFIXES:
.SUFFIXES: .o .a .c


# Where to install the executables
insdir = ../../bin

PROG = ottoimp

SRC = ottoimp.c
OBJ = $(SRC:.c=.o)

LIB = -lotto

CPPFLAGS=-I../../include
CFLAGS=-g -Wall -Werror -Wno-unknown-pragmas -Wno-char-subscripts

target: $(PROG)

clean::
	/bin/rm -f $(PROG) *.o

rebuild: clean target

install: $(PROG)
	/usr/bin/cp -f $(PROG) $(insdir)
	@/usr/bin/chmod 755 $(insdir)/$(PROG) $(insdir)

uninstall:
	/usr/bin/rm -f $(insdir)/$(PROG)

distclean: clean uninstall

$(PROG): $(OBJ) ../../lib/libotto.a Makefile
	@/usr/bin/echo "Linking $(PROG)"
	$(CC) -o $@ $(OBJ) -L../../lib $(LIB)
	@/usr/bin/echo "Success!!"

$(OBJ): $(SRC) ../../include/otto.h

