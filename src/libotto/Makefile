.SUFFIXES:
.SUFFIXES: .o .a .c

# Where to install the executables
libdir = ../../lib
incdir = ../../include

SRCS = ottocfg.c                 \
       ottocond.c                \
       ottocrud.c                \
       ottodb.c                  \
		 ottohtml.c                \
       ottoipc.c                 \
       ottojob.c                 \
       ottolog.c                 \
       ottoutil.c                \
       otto_cnd_writer.c         \
       otto_csv_reader.c         \
       otto_csv_writer.c         \
       otto_dtl_writer.c         \
		 otto_htmlcsv_writer.c     \
		 otto_htmldtl_writer.c     \
		 otto_htmljil_writer.c     \
		 otto_htmlmspdi_writer.c   \
		 otto_htmlsum_writer.c     \
		 otto_htmlversion_writer.c \
       otto_jil_reader.c         \
       otto_jil_writer.c         \
       otto_mspdi_reader.c       \
       otto_mspdi_writer.c       \
       otto_pid_writer.c         \
       otto_sum_writer.c         \
       signals.c                 \
       strptime.c                \
       linenoise.c

OBJS = $(SRCS:.c=.o)

HDR  = otto.h

LIB  = libotto.a

CFLAGS=-g -Wall -Werror -Wno-unknown-pragmas -Wno-char-subscripts


target: $(LIB)

clean::
	/bin/rm -f $(LIB) *.o

rebuild: clean target

install: target
	/usr/bin/cp -f $(LIB) $(libdir)
	/usr/bin/cp -f $(HDR) $(incdir)

uninstall:
	/usr/bin/rm -f $(libdir)/$(LIB)
	/usr/bin/rm -f $(incdir)/$(HDR)

distclean: clean uninstall


$(LIB): $(OBJS) Makefile
	@/usr/bin/echo "Creating library $(LIB)"
	/usr/bin/ar rcs $(LIB)  $(OBJS)
	@/usr/bin/echo "Success!"

$(OBJS): $(HDR) $(SRCS)

