.SUFFIXES:
.SUFFIXES: .o .a .c

# Where to install the executables
insdir = ../lib
hdrdir = ../include

SRCS = ottocfg.c         \
       ottocond.c        \
       ottocrud.c        \
       ottodb.c          \
       otto_dtl_writer.c \
       ottoipc.c         \
       otto_jil_reader.c \
       otto_jil_writer.c \
       ottojob.c         \
       otto_mpp_reader.c \
       otto_mpp_writer.c \
       otto_pid_writer.c \
       otto_sum_writer.c \
       ottoutil.c        \
       signals.c         \
       simplelog.c

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

CFLAGS=-g -Wall -Werror -Wno-unknown-pragmas -Wno-char-subscripts


install: all

all: $(OBJS)
	@echo Make all done

clean::
	/bin/rm -f *.o *d

-include $(DEPS)   # include all dependancy files in the makefile

# rule to generate a dependancy file by using the C preprocessor
%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

