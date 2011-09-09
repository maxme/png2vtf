##
## http://twitter.com/maximeb
##

include configure.mk

CC				= gcc
SOURCES_PNG2VTF	= png2vtf.c
OBJS_PNG2VTF	= $(SOURCES_PNG2VTF:.c=.o)
CFLAGS			= -W -Wall -ggdb -DNDEBUG $(CONF_INCS)
LIBS			= $(CONF_LIBS) -lpng
EXEC			= png2vtf

all: $(EXEC)

png2vtf: $(OBJS_PNG2VTF)
	$(CC) $(CFLAGS) $(OBJS_PNG2VTF) $(LIBS) -o png2vtf

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS_PNG2VTF) *~ \#*

distclean: clean
	rm -f $(EXEC)
