.SUFFIXES:
.SUFFIXES: .o .a .c

# Where to install the executables
insdir = ../../bin

PROG = ottosend

SRCS = ottosend.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

SHRS = ../ottolib/ottocfg.o   \
       ../ottolib/ottodb.o    \
       ../ottolib/ottoipc.o   \
       ../ottolib/ottojob.o   \
       ../ottolib/ottoutil.o  \
       ../ottolib/signals.o   \
       ../ottolib/simplelog.o

CFLAGS=-g -Wall -Werror -Wno-unknown-pragmas -Wno-char-subscripts -I../ottolib

all: $(PROG)
	@echo Make $(PROG) done

install: $(PROG)
	@echo cp $(PROG) $(insdir)
	@rm -f $(insdir)/$(PROG)
	@cp $(PROG) $(insdir)
	@chmod 755 $(insdir)/$(PROG) $(insdir)

$(PROG): $(OBJS) $(SHRS) Makefile
	$(CC) -o $@ $(OBJS) $(SHRS)

clean::
	/bin/rm -f *.o *.d $(PROG)

-include $(DEPS)   # include all dependancy files in the makefile

# rule to generate a dependancy file by using the C preprocessor
%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

