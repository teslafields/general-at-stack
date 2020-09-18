PROGRAM=modemapp
SRCS= main.c system.c at-control.c at-queue.c at-recv.c
OUTDIR=	../out
PROGRAMPATH= $(addprefix $(OUTDIR)/, $(PROGRAM))

CC?= $(CROSS_COMPILE)gcc
CFLAGS?=
LDFLAGS?=
LIBS?=
define?=-D SIM7100 -D PPP -D UNIXSV -D QDEBUG
WARN?=
ifeq ($(WARN),full)
WARNINGS= -W -Wall -ansi -Wextra -pedantic -Wstrict-overflow=5 -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wunused-variable
else
WARNINGS= -W -Wall
endif

LINKER= -lpthread

DEBUG= -ggdb

export SOURCES
export DEBUG
export WARNINGS

all: $(PROGRAM)

precomp:
	$(CC) $(define) $(SRCS) -E

$(PROGRAM):
	mkdir -p ${OUTDIR}
	$(CC) $(define) $(SRCS) -o $(PROGRAMPATH) $(DEBUG) $(LINKER) $(CFLAGS) $(LDFLAGS) $(LIBS) $(WARNINGS)

clean:
	rm -f $(PROGRAMPATH)
