PROG= 2048
CFLAGS= -march=native -O2 -pipe
LDADD= -lncurses
MKMAN= no
BINDIR=/usr/local/bin

# FreeBSD
MK_MAN= no
WITHOUT_DEBUG_FILES= 
WITHOUT_PROFILE=
WITHOUT_SSP=
WITHOUT_PIE=

.include <bsd.prog.mk>
