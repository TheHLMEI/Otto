.SUFFIXES:

# Where to install the executables
insdir = ../../bin

PROG = ottomsp

SRC = ottomsp.c
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)

SHRS = ../ottolib/ottocfg.o         \
       ../ottolib/ottocond.o        \
       ../ottolib/ottocrud.o        \
       ../ottolib/ottodb.o          \
       ../ottolib/ottoipc.o         \
       ../ottolib/ottojob.o         \
       ../ottolib/otto_mpp_reader.o \
       ../ottolib/otto_mpp_writer.o \
       ../ottolib/ottoutil.o        \
       ../ottolib/signals.o         \
       ../ottolib/simplelog.o

CFLAGS=-g -Wall -Werror -Wno-unknown-pragmas -Wno-char-subscripts -I../ottolib

all: $(PROG)
	@echo Make $(PROG) done

install: $(PROG)
	@echo cp $(PROG) $(insdir)
	@rm -f $(insdir)/$(PROG)
	@cp $(PROG) $(insdir)
	@chmod 755 $(insdir)/$(PROG) $(insdir)

$(PROG): $(OBJ) $(SHRS) Makefile
	$(CC) -o $@ $(OBJ) $(SHRS)

clean::
	/bin/rm -f *.o *.d $(PROG)

-include $(DEP)   # include all dependancy files in the makefile

# rule to specifically generate the program's object
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

# rule to generate a dependancy file by using the C preprocessor
%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

