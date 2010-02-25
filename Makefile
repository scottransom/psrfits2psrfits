# Other include directory (for CFITSIO, libsla, which is in PRESTO)
OTHERINCLUDE = /usr/include/cfitsio 
# Other link directory (for CFITSIO, libsla, which is in PRESTO)
OTHERLINK = -L/usr/local/lib -lcfitsio -L$(PRESTO)/lib -lsla 

# Source directory
SRCDIR = $(shell pwd)

# Which C compiler
CC = gcc
CFLAGS = -I$(OTHERINCLUDE) -DSRCDIR=\"$(SRCDIR)\"\
	-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64\
	-g -Wall -W -std=c99 -O
#	-O3 -Wall -W
CLINKFLAGS = $(CFLAGS)

# When modifying the CLIG files, the is the location of the clig binary
CLIG = /usr/bin/clig
# Rules for CLIG generated files
%_cmd.c : %_cmd.cli
	$(CLIG) -o $*_cmd -d $<

OBJS1 = vectors.o sla.o write_psrfits.o rescale.o read_psrfits.o

psrfits2psrfits: psrfits2psrfits.o psrfits2psrfits_cmd.o $(OBJS1)
	$(CC) $(CLINKFLAGS) -o $@ psrfits2psrfits.o psrfits2psrfits_cmd.o $(OBJS1) -lm $(OTHERLINK)

# Default indentation is K&R style with no-tabs,
# an indentation level of 4, and a line-length of 85
indent:
	indent -kr -nut -i4 -l85 *.c
	rm *.c~

clean:
	rm -f *.o *~ *#

cleaner: clean
	rm -f psrfits2psrfits
