
VERSION=0.1

# CC = gcc
CPPFLAGS=-DVERSION_STR=\"$(VERSION)\" 
CFLAGS = -I/usr/local/include/libmpd-1.0 -Wall -g

# LD = gcc
LDFLAGS = -g
LDLIBS = -lmpd

efikampd : efikampd.o term.o
#	$(LD) $(LDFLAGS) -o $@ $+ $(LDLIBS)

efikampd.o : efikampd.c efika.h term.h
term.o : term.c term.h

changes : 
	svn log -v . > CHANGES

clean:
	rm -f efikampd.o term.o
	rm -f *~
	rm -f \#*\#

distclean: clean
	rm -f efikampd
