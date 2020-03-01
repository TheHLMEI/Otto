.SUFFIXES:
.SUFFIXES: .o .a .c

# Where to install the executables
insdir = ../lib
hdrdir = ../include

LIB = libottolib.a

SRCS = ottocfg.c         \
       ottocond.c        \
       ottocrud.c        \
       ottodb.c          \
       otto_dtl_writer.c \
       ottoipc.c         \
       otto_jil_reader.c \
       otto_jil_writer.c \
       ottojob.c         \
       ottolog.c         \
       otto_mpp_reader.c \
       otto_mpp_writer.c \
       otto_pid_writer.c \
       ottosignal.c      \
       otto_sum_writer.c \
       ottoutil.c

OBJS = $(SRCS:.c=.o)

CFLAGS=-g -Wall -Werror -Wno-unknown-pragmas -Wno-char-subscripts

all: $(LIB)
	@echo Make $(LIB) done

install: $(LIB)
	@echo cp $(LIB) $(insdir)
	@rm -f $(insdir)/$(LIB)
	@cp $(LIB) $(insdir)
	@chmod 755 $(insdir)/$(LIB) $(insdir)
	@echo cp *.h $(hdrdir)
	@cp *.h $(hdrdir)

$(LIB): $(OBJS)
	ar -crs $(LIB) $(OBJS)

clean::
	/bin/rm -f *.o $(LIB)

