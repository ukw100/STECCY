FB_FLAGS    = -O2 -Wall -Wextra -Werror -DFRAMEBUFFER -I. -I../src/font -I../src/tape -I../src/zxram -I../src/zxscr -I../src/zxio -I../src/zxkbd -I../src/z80
X11_FLAGS   = -O2 -Wall -Wextra -Werror -DX11 -I. -I../src/font -I../src/tape -I../src/zxram -I../src/zxscr -I../src/zxio -I../src/zxkbd -I../src/z80

FB_OBJ      = fb-obj/lxmain.o fb-obj/z80.o fb-obj/zxram.o fb-obj/zxscr.o fb-obj/zxio.o fb-obj/tape.o fb-obj/lxfb.o fb-obj/lxkbd.o fb-obj/lxmenu.o \
	      fb-obj/lxfont.o fb-obj/lxmapkey.o fb-obj/lxdisplay.o fb-obj/lxjoystick.o
X11_OBJ     = x11-obj/lxmain.o x11-obj/z80.o x11-obj/zxram.o x11-obj/zxscr.o x11-obj/zxio.o x11-obj/tape.o x11-obj/lxx11.o x11-obj/lxmenu.o \
	      x11-obj/lxfont.o x11-obj/lxmapkey.o x11-obj/lxdisplay.o x11-obj/lxjoystick.o

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

fb-obj/z80.o: ../src/z80/z80.c lxfb.h lxmenu.h ../src/z80/z80.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h ../src/zxio/zxio.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/z80.o ../src/z80/z80.c
fb-obj/zxscr.o: ../src/zxscr/zxscr.c lxfb.h lxmenu.h ../src/z80/z80.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h ../src/zxio/zxio.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/zxscr.o ../src/zxscr/zxscr.c
fb-obj/zxram.o: ../src/zxram/zxram.c lxfb.h lxmenu.h ../src/z80/z80.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h ../src/zxio/zxio.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/zxram.o ../src/zxram/zxram.c
fb-obj/zxio.o: ../src/zxio/zxio.c lxfb.h lxmenu.h ../src/z80/z80.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h ../src/zxio/zxio.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/zxio.o ../src/zxio/zxio.c
fb-obj/tape.o: ../src/tape/tape.c ../src/z80/z80.h ../src/zxscr/zxscr.h ../src/zxram/zxram.h ../src/tape/tape.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/tape.o ../src/tape/tape.c
fb-obj/lxfb.o: lxfb.c lxdisplay.h lxfb.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxfb.o lxfb.c
fb-obj/lxkbd.o: lxkbd.c ../src/z80/z80.h scancodes.h lxkbd.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxkbd.o lxkbd.c
fb-obj/lxdisplay.o: lxdisplay.c lxfb.h ../src/z80/z80.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h lxdisplay.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxdisplay.o lxdisplay.c
fb-obj/lxmapkey.o: lxmapkey.c scancodes.h lxjoystick.h ../src/z80/z80.h lxmapkey.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxmapkey.o lxmapkey.c
fb-obj/lxjoystick.o: lxjoystick.c lxjoystick.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxjoystick.o lxjoystick.c
fb-obj/lxmenu.o: lxmenu.c lxkbd.h lxdisplay.h lxmapkey.h lxfont.h ../src/z80/z80.h scancodes.h lxjoystick.h lxmenu.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxmenu.o lxmenu.c
fb-obj/lxfont.o: lxfont.c ../src/z80/z80.h lxfont.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxfont.o lxfont.c
fb-obj/lxmain.o: lxmain.c lxkbd.h lxfb.h ../src/z80/z80.h
	@mkdir -p fb-obj
	$(CC) $(FB_FLAGS)   -c -o fb-obj/lxmain.o lxmain.c

x11-obj/z80.o: ../src/z80/z80.c lxx11.h lxmenu.h ../src/z80/z80.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h ../src/zxio/zxio.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/z80.o ../src/z80/z80.c
x11-obj/tape.o: ../src/tape/tape.c ../src/z80/z80.h ../src/zxscr/zxscr.h ../src/zxram/zxram.h ../src/tape/tape.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/tape.o ../src/tape/tape.c
x11-obj/zxscr.o: ../src/zxscr/zxscr.c lxx11.h lxmenu.h ../src/z80/z80.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h ../src/zxio/zxio.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/zxscr.o ../src/zxscr/zxscr.c
x11-obj/zxram.o: ../src/zxram/zxram.c lxx11.h lxmenu.h ../src/z80/z80.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h ../src/zxio/zxio.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/zxram.o ../src/zxram/zxram.c
x11-obj/zxio.o: ../src/zxio/zxio.c lxx11.h lxmenu.h ../src/z80/z80.h ../src/tape/tape.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h ../src/zxio/zxio.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/zxio.o ../src/zxio/zxio.c
x11-obj/lxx11.o: lxx11.c ../src/z80/z80.h lxmenu.h scancodes.h lxmapkey.h lxdisplay.h lxx11.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxx11.o lxx11.c
x11-obj/lxdisplay.o: lxdisplay.c lxx11.h ../src/z80/z80.h ../src/zxram/zxram.h ../src/zxscr/zxscr.h lxdisplay.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxdisplay.o lxdisplay.c
x11-obj/lxmapkey.o: lxmapkey.c scancodes.h lxjoystick.h ../src/z80/z80.h lxmapkey.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxmapkey.o lxmapkey.c
x11-obj/lxjoystick.o: lxjoystick.c lxjoystick.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxjoystick.o lxjoystick.c
x11-obj/lxmenu.o: lxmenu.c lxx11.h lxdisplay.h lxmapkey.h lxfont.h ../src/z80/z80.h scancodes.h lxjoystick.h lxmenu.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxmenu.o lxmenu.c
x11-obj/lxfont.o: lxfont.c ../src/z80/z80.h lxfont.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxfont.o lxfont.c
x11-obj/lxmain.o: lxmain.c lxx11.h ../src/z80/z80.h
	@mkdir -p x11-obj
	$(CC) $(X11_FLAGS)   -c -o x11-obj/lxmain.o lxmain.c

clean:
	rm -f fb-obj/*.o x11-obj/*.o steccy xsteccy
