OPTS	    = -O2 -Wall -Wextra -Werror -Wstrict-prototypes
INCDIRS	    = -I. -I../src/font -I../src/tape -I../src/zxram -I../src/zxscr -I../src/zxio -I../src/zxkbd -I../src/z80

FB_FLAGS    = $(OPTS) $(INCDIRS) -DFRAMEBUFFER
X11_FLAGS   = $(OPTS) $(INCDIRS) -DX11

FB_OBJ      = fb-obj/lxmain.o fb-obj/z80.o fb-obj/zxram.o fb-obj/zxscr.o fb-obj/zxio.o fb-obj/tape.o fb-obj/lxfb.o fb-obj/lxkbd.o fb-obj/lxmenu.o \
	      fb-obj/lxfont.o fb-obj/lxmapkey.o fb-obj/lxdisplay.o fb-obj/lxjoystick.o
X11_OBJ     = x11-obj/lxmain.o x11-obj/z80.o x11-obj/zxram.o x11-obj/zxscr.o x11-obj/zxio.o x11-obj/tape.o x11-obj/lxx11.o x11-obj/lxmenu.o \
	      x11-obj/lxfont.o x11-obj/lxmapkey.o x11-obj/lxdisplay.o x11-obj/lxjoystick.o
INC	    = lxdisplay.h lxfb.h lxfont.h lxjoystick.h lxkbd.h lxmapkey.h lxmenu.h lxx11.h scancodes.h	\
	      ../src/font/font.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h \
	      ../src/zxio/zxio.h ../src/zxkbd/zxkbd.h ../src/z80/z80.h

all: steccy xsteccy

steccy: $(FB_OBJ)
	$(CC) $(FB_OBJ) -lpthread -o steccy

xsteccy: $(X11_OBJ)
	$(CC) $(X11_OBJ) -lX11 -o xsteccy

install: steccy-install xsteccy-install

steccy-install: steccy
	cp steccy /usr/local/bin

xsteccy-install: xsteccy
	cp xsteccy /usr/local/bin
	@mkdir -p /usr/local/lib
	cp steccy.png /usr/local/lib

fb-obj/z80.o: ../src/z80/z80.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/z80.o ../src/z80/z80.c
fb-obj/zxscr.o: ../src/zxscr/zxscr.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/zxscr.o ../src/zxscr/zxscr.c
fb-obj/zxram.o: ../src/zxram/zxram.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/zxram.o ../src/zxram/zxram.c
fb-obj/zxio.o: ../src/zxio/zxio.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/zxio.o ../src/zxio/zxio.c
fb-obj/tape.o: ../src/tape/tape.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/tape.o ../src/tape/tape.c
fb-obj/lxfb.o: lxfb.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxfb.o lxfb.c
fb-obj/lxkbd.o: lxkbd.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxkbd.o lxkbd.c
fb-obj/lxdisplay.o: lxdisplay.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxdisplay.o lxdisplay.c
fb-obj/lxmapkey.o: lxmapkey.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxmapkey.o lxmapkey.c
fb-obj/lxjoystick.o: lxjoystick.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxjoystick.o lxjoystick.c
fb-obj/lxmenu.o: lxmenu.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxmenu.o lxmenu.c
fb-obj/lxfont.o: lxfont.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxfont.o lxfont.c
fb-obj/lxmain.o: lxmain.c $(INC)
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxmain.o lxmain.c

x11-obj/z80.o: ../src/z80/z80.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/z80.o ../src/z80/z80.c
x11-obj/tape.o: ../src/tape/tape.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/tape.o ../src/tape/tape.c
x11-obj/zxscr.o: ../src/zxscr/zxscr.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/zxscr.o ../src/zxscr/zxscr.c
x11-obj/zxram.o: ../src/zxram/zxram.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/zxram.o ../src/zxram/zxram.c
x11-obj/zxio.o: ../src/zxio/zxio.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/zxio.o ../src/zxio/zxio.c
x11-obj/lxx11.o: lxx11.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxx11.o lxx11.c
x11-obj/lxdisplay.o: lxdisplay.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxdisplay.o lxdisplay.c
x11-obj/lxmapkey.o: lxmapkey.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxmapkey.o lxmapkey.c
x11-obj/lxjoystick.o: lxjoystick.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxjoystick.o lxjoystick.c
x11-obj/lxmenu.o: lxmenu.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxmenu.o lxmenu.c
x11-obj/lxfont.o: lxfont.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxfont.o lxfont.c
x11-obj/lxmain.o: lxmain.c $(INC)
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxmain.o lxmain.c

clean:
	rm -f fb-obj/*.o x11-obj/*.o steccy xsteccy
