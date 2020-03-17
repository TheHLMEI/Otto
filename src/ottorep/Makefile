.SUFFIXES:

# Where to install the executables
insdir = ../../bin

PROG = ottorep

SRC = ottorep.c
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)

SHRS = ../ottolib/ottocfg.o         \
       ../ottolib/ottocond.o        \
       ../ottolib/ottodb.o          \
       ../ottolib/ottojob.o         \
       ../ottolib/ottoutil.o        \
       ../ottolib/otto_dtl_writer.o \
       ../ottolib/otto_jil_writer.o \
       ../ottolib/otto_pid_writer.o \
       ../ottolib/otto_sum_writer.o \
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


