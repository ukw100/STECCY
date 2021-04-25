/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxx11.c - STECCY x11 routines for linux
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2020-2021 Frank Meyer - frank(at)fli4l.de
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
#include <X11/Xlib.h>
#define XK_MISCELLANY                                                                                       // enable misc keysym definitions
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "z80.h"
#include "lxmenu.h"
#include "scancodes.h"
#include "lxmapkey.h"
#include "lxdisplay.h"
#include "lxx11.h"

#define MINIMAL_WIDTH       800
#define MINIMAL_HEIGHT      480

#undef DEBUG                                                                                                // change here

#ifdef DEBUG
#define debug_printf(...)   do { printf(__VA_ARGS__); fflush (stdout); } while (0)
#else
#define debug_printf(...)
#endif

#define MAX_KEYCODES        0x80

uint16_t keycode_to_scancode[MAX_KEYCODES] =
{
    SCANCODE_NONE,                                                                                          // 0x00
    SCANCODE_NONE,                                                                                          // 0x01
    SCANCODE_NONE,                                                                                          // 0x02
    SCANCODE_NONE,                                                                                          // 0x03
    SCANCODE_NONE,                                                                                          // 0x04
    SCANCODE_NONE,                                                                                          // 0x05
    SCANCODE_NONE,                                                                                          // 0x06
    SCANCODE_NONE,                                                                                          // 0x07
    SCANCODE_NONE,                                                                                          // 0x08
    SCANCODE_ESC,                                                                                           // 0x09
    SCANCODE_1,                                                                                             // 0x0A
    SCANCODE_2,                                                                                             // 0x0B
    SCANCODE_3,                                                                                             // 0x0C
    SCANCODE_4,                                                                                             // 0x0D
    SCANCODE_5,                                                                                             // 0x0E
    SCANCODE_6,                                                                                             // 0x0F

    SCANCODE_7,                                                                                             // 0x10
    SCANCODE_8,                                                                                             // 0x11
    SCANCODE_9,                                                                                             // 0x12
    SCANCODE_0,                                                                                             // 0x13
    SCANCODE_SHARP_S,                                                                                       // 0x14
    SCANCODE_ACCENT,                                                                                        // 0x15
    SCANCODE_BACKSPACE,                                                                                     // 0x16
    SCANCODE_TAB,                                                                                           // 0x17
    SCANCODE_Q,                                                                                             // 0x18
    SCANCODE_W,                                                                                             // 0x19
    SCANCODE_E,                                                                                             // 0x1A
    SCANCODE_R,                                                                                             // 0x1B
    SCANCODE_T,                                                                                             // 0x1C
    SCANCODE_Z,                                                                                             // 0x1D
    SCANCODE_U,                                                                                             // 0x1E
    SCANCODE_I,                                                                                             // 0x1F

    SCANCODE_O,                                                                                             // 0x20
    SCANCODE_P,                                                                                             // 0x21
    SCANCODE_NONE,                                                                                          // 0x22
    SCANCODE_PLUS,                                                                                          // 0x23
    SCANCODE_ENTER,                                                                                         // 0x24
    SCANCODE_LCTRL,                                                                                         // 0x25
    SCANCODE_A,                                                                                             // 0x26
    SCANCODE_S,                                                                                             // 0x27
    SCANCODE_D,                                                                                             // 0x28
    SCANCODE_F,                                                                                             // 0x29
    SCANCODE_G,                                                                                             // 0x2A
    SCANCODE_H,                                                                                             // 0x2B
    SCANCODE_J,                                                                                             // 0x2C
    SCANCODE_K,                                                                                             // 0x2D
    SCANCODE_L,                                                                                             // 0x2E
    SCANCODE_NONE,                                                                                          // 0x2F

    SCANCODE_NONE,                                                                                          // 0x30
    SCANCODE_CIRCUMFLEX,                                                                                    // 0x31
    SCANCODE_LSHIFT,                                                                                        // 0x32
    SCANCODE_HASH,                                                                                          // 0x33
    SCANCODE_Y,                                                                                             // 0x34
    SCANCODE_X,                                                                                             // 0x35
    SCANCODE_C,                                                                                             // 0x36
    SCANCODE_V,                                                                                             // 0x37
    SCANCODE_B,                                                                                             // 0x38
    SCANCODE_N,                                                                                             // 0x39
    SCANCODE_M,                                                                                             // 0x3A
    SCANCODE_COMMA,                                                                                         // 0x3B
    SCANCODE_DOT,                                                                                           // 0x3C
    SCANCODE_MINUS,                                                                                         // 0x3D
    SCANCODE_RSHIFT,                                                                                        // 0x3E
    SCANCODE_KEYPAD_PF3,                                                                                    // 0x3F

    SCANCODE_LALT,                                                                                          // 0x40
    SCANCODE_SPACE,                                                                                         // 0x41
    SCANCODE_NONE,                                                                                          // 0x42
    SCANCODE_F1,                                                                                            // 0x43
    SCANCODE_F2,                                                                                            // 0x44
    SCANCODE_F3,                                                                                            // 0x45
    SCANCODE_F4,                                                                                            // 0x46
    SCANCODE_F5,                                                                                            // 0x47
    SCANCODE_F6,                                                                                            // 0x48
    SCANCODE_F7,                                                                                            // 0x49
    SCANCODE_F8,                                                                                            // 0x4A
    SCANCODE_F9,                                                                                            // 0x4B
    SCANCODE_F10,                                                                                           // 0x4C
    SCANCODE_KEYPAD_PF1,                                                                                    // 0x4D
    SCANCODE_SCROLL,                                                                                        // 0x4E
    SCANCODE_KEYPAD_7,                                                                                      // 0x4F

    SCANCODE_KEYPAD_8,                                                                                      // 0x50
    SCANCODE_KEYPAD_9,                                                                                      // 0x51
    SCANCODE_KEYPAD_PF4,                                                                                    // 0x52
    SCANCODE_KEYPAD_4,                                                                                      // 0x53
    SCANCODE_KEYPAD_5,                                                                                      // 0x54
    SCANCODE_KEYPAD_6,                                                                                      // 0x55
    SCANCODE_KEYPAD_PLUS,                                                                                   // 0x56
    SCANCODE_KEYPAD_1,                                                                                      // 0x57
    SCANCODE_KEYPAD_2,                                                                                      // 0x58
    SCANCODE_KEYPAD_3,                                                                                      // 0x59
    SCANCODE_KEYPAD_0,                                                                                      // 0x5A
    SCANCODE_KEYPAD_COMMA,                                                                                  // 0x5B
    SCANCODE_NONE,                                                                                          // 0x5C
    SCANCODE_NONE,                                                                                          // 0x5D
    SCANCODE_LESS,                                                                                          // 0x5E
    SCANCODE_F11,                                                                                           // 0x5F

    SCANCODE_F12,                                                                                           // 0x60
    SCANCODE_NONE,                                                                                          // 0x61
    SCANCODE_NONE,                                                                                          // 0x62
    SCANCODE_NONE,                                                                                          // 0x63
    SCANCODE_NONE,                                                                                          // 0x64
    SCANCODE_NONE,                                                                                          // 0x65
    SCANCODE_NONE,                                                                                          // 0x66
    SCANCODE_NONE,                                                                                          // 0x67
    SCANCODE_KEYPAD_ENTER,                                                                                  // 0x68
    SCANCODE_RCTRL,                                                                                         // 0x69
    SCANCODE_KEYPAD_PF2,                                                                                    // 0x6A
    SCANCODE_NONE,                                                                                          // 0x6B
    SCANCODE_RALT,                                                                                          // 0x6C
    SCANCODE_NONE,                                                                                          // 0x6D
    SCANCODE_HOME,                                                                                          // 0x6E
    SCANCODE_U_ARROW,                                                                                       // 0x6F

    SCANCODE_PAGE_UP,                                                                                       // 0x70
    SCANCODE_L_ARROW,                                                                                       // 0x71
    SCANCODE_R_ARROW,                                                                                       // 0x72
    SCANCODE_END,                                                                                           // 0x73
    SCANCODE_D_ARROW,                                                                                       // 0x74
    SCANCODE_PAGE_DOWN,                                                                                     // 0x75
    SCANCODE_INSERT,                                                                                        // 0x76
    SCANCODE_DELETE,                                                                                        // 0x77
    SCANCODE_NONE,                                                                                          // 0x78
    SCANCODE_NONE,                                                                                          // 0x79
    SCANCODE_NONE,                                                                                          // 0x7A
    SCANCODE_NONE,                                                                                          // 0x7B
    SCANCODE_NONE,                                                                                          // 0x7C
    SCANCODE_NONE,                                                                                          // 0x7D
    SCANCODE_NONE,                                                                                          // 0x7E
    SCANCODE_PAUSE,                                                                                         // 0x7F
};

static Display *            display;
static GC                   gc;
static Window               win;
static XEvent               event;
static Atom                 wm_delete_message;
static Atom                 wm_protocols;

static void
create_simple_window(Display* display, int width, int height, int x, int y)
{
  int screen_num        = DefaultScreen(display);
  int win_border_width  = 2;

  win = XCreateSimpleWindow(display, RootWindow(display, screen_num), x, y, width, height, win_border_width, WhitePixel(display, screen_num), BlackPixel(display, screen_num));
  XMapWindow(display, win);
  XFlush(display);
}

static int
create_gc (Display * display, Window win)
{
    unsigned long   valuemask = 0;
    XGCValues       values;
    unsigned int    line_width = 2;                                                                 // line width for the GC
    int             line_style = LineSolid;                                                         // style for lines drawing and
    int             cap_style = CapButt;                                                            // style of the line's edge and
    int             join_style = JoinBevel;                                                         // joined lines
    int             screen_num = DefaultScreen(display);

    gc = XCreateGC(display, win, valuemask, &values);

    if ((long) gc < 0)
    {
        fprintf(stderr, "XCreateGC: failed\n");
        return -1;
    }

    XSetForeground(display, gc, WhitePixel(display, screen_num));
    XSetBackground(display, gc, BlackPixel(display, screen_num));

    XSetLineAttributes(display, gc, line_width, line_style, cap_style, join_style);                 // define the style of lines that will be drawn using this GC
    XSetFillStyle(display, gc, FillSolid);                                                          // define the fill style for the GC. to be 'solid filling'
    return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * x11_init - init X11 routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
x11_init (char * geometry)
{
    char *          display_name;
    unsigned int    width   = MINIMAL_WIDTH;                                                        // default width
    unsigned int    height  = MINIMAL_HEIGHT;                                                       // default height

    if (geometry)
    {
        width = atoi (geometry);

        if (width < MINIMAL_WIDTH)
        {
            width = MINIMAL_WIDTH;
        }

        while (*geometry && *geometry != 'x')
        {
            geometry++;
        }

        if (*geometry == 'x')
        {
            geometry++;
            height = atoi (geometry);

            if (height < MINIMAL_HEIGHT)
            {
                height = MINIMAL_HEIGHT;
            }
        }
    }

    display_name = getenv ("DISPLAY");

    if (display_name == NULL)
    {
        fprintf (stderr, "cannot read environment variable DISPLAY\n");
        return -1;
    }

    display = XOpenDisplay (display_name);

    if (display == NULL)
    {
        fprintf (stderr, "cannot connect to X server '%s'\n", display_name);
        return -1;
    }

#if defined DEBUG
    int             screen_num      = DefaultScreen(display);
    unsigned int    display_width   = DisplayWidth(display, screen_num);
    unsigned int    display_height  = DisplayHeight(display, screen_num);

    printf ("display is %dx%d\n", display_width, display_height);
    printf ("window  is %dx%d\n", width, height);
#endif

    create_simple_window (display, width, height, 0, 0);

    if (create_gc (display, win) < 0)
    {
        return -1;
    }

    XSync(display, False);

    wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
    wm_delete_message = XInternAtom (display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, win, &wm_delete_message, 1);

    lxdisplay_init (width, height);

    XStoreName(display, win, "STECCY");

    XSelectInput(display, win, KeyPressMask | KeyReleaseMask | ExposureMask | FocusChangeMask);
    XAutoRepeatOff (display);

    return 0;
}

uint16_t
get_scancode_from_keycode (unsigned int keycode)
{
    uint16_t    scancode = 0;
    KeySym      keysym;

    keysym = XkbKeycodeToKeysym (display, keycode, 0, 0);

    switch (keysym)                                                                         // see also /usr/include/X11/keysymdef.h
    {
        case 0xFE03:            scancode = SCANCODE_RALT;               break;              // Right Alt (XMing Server)
        case XK_BackSpace:      scancode = SCANCODE_BACKSPACE;          break;              // Backspace
        case XK_Tab:            scancode = SCANCODE_TAB;                break;              // Tab
        case XK_Return:         scancode = SCANCODE_ENTER;              break;              // Enter
        case XK_Pause:          scancode = SCANCODE_PAUSE;              break;              // Pause
        case XK_Scroll_Lock:    scancode = SCANCODE_SCROLL;             break;              // Scroll
        case XK_Escape:         scancode = SCANCODE_ESC;                break;              // Esc
        case XK_Home:           scancode = SCANCODE_HOME;               break;              // Home
        case XK_Left:           scancode = SCANCODE_L_ARROW;            break;              // Left Arrow
        case XK_Up:             scancode = SCANCODE_U_ARROW;            break;              // Up Arrow
        case XK_Right:          scancode = SCANCODE_R_ARROW;            break;              // Right Arrow
        case XK_Down:           scancode = SCANCODE_D_ARROW;            break;              // Down Arrow
        case XK_Page_Up:        scancode = SCANCODE_PAGE_UP;            break;              // Page Up
        case XK_Page_Down:      scancode = SCANCODE_PAGE_DOWN;          break;              // Page Down
        case XK_End:            scancode = SCANCODE_END;                break;              // End
        case XK_Begin:          scancode = SCANCODE_HOME;               break;              // Begin
        case XK_Insert:         scancode = SCANCODE_INSERT;             break;              // Insert
        case XK_Menu:           scancode = SCANCODE_MENU;               break;              // Menu
        case XK_Num_Lock:       scancode = SCANCODE_KEYPAD_PF1;         break;              // KP Num           = KP PF1
        case XK_KP_Enter:       scancode = SCANCODE_KEYPAD_ENTER;       break;              // KP Enter
        case XK_KP_Home:        scancode = SCANCODE_KEYPAD_7;           break;              // KP Home          = KP 7
        case XK_KP_Left:        scancode = SCANCODE_KEYPAD_4;           break;              // KP Left          = KP 4
        case XK_KP_Up:          scancode = SCANCODE_KEYPAD_8;           break;              // KP Up            = KP 8
        case XK_KP_Right:       scancode = SCANCODE_KEYPAD_6;           break;              // KP Right         = KP 6
        case XK_KP_Down:        scancode = SCANCODE_KEYPAD_2;           break;              // KP Down          = KP 2
        case XK_KP_Page_Up:     scancode = SCANCODE_KEYPAD_8;           break;              // KP Page Up       = KP 9
        case XK_KP_Page_Down:   scancode = SCANCODE_KEYPAD_8;           break;              // KP Page Down     = KP 3
        case XK_KP_End:         scancode = SCANCODE_KEYPAD_1;           break;              // KP End           = KP 1
        case XK_KP_Begin:       scancode = SCANCODE_KEYPAD_5;           break;              // KP Begin         = KP 5
        case XK_KP_Insert:      scancode = SCANCODE_KEYPAD_0;           break;              // KP Insert        = KP 0
        case XK_KP_Delete:      scancode = SCANCODE_KEYPAD_COMMA;       break;              // KP Entf          = KP ,
        case XK_KP_Multiply:    scancode = SCANCODE_KEYPAD_PF3;         break;              // KP Multiply      = KP PF3
        case XK_KP_Add:         scancode = SCANCODE_KEYPAD_PLUS;        break;              // KP Add           = KP +
        case XK_KP_Subtract:    scancode = SCANCODE_KEYPAD_PF4;         break;              // KP Minus         = KP PF4
        case XK_KP_Divide:      scancode = SCANCODE_KEYPAD_PF2;         break;              // KP Divide        = KP PF2
        case XK_KP_0:           scancode = SCANCODE_KEYPAD_0;           break;              // KP 0
        case XK_KP_1:           scancode = SCANCODE_KEYPAD_1;           break;              // KP 1
        case XK_KP_2:           scancode = SCANCODE_KEYPAD_2;           break;              // KP 2
        case XK_KP_3:           scancode = SCANCODE_KEYPAD_3;           break;              // KP 3
        case XK_KP_4:           scancode = SCANCODE_KEYPAD_4;           break;              // KP 4
        case XK_KP_5:           scancode = SCANCODE_KEYPAD_5;           break;              // KP 5
        case XK_KP_6:           scancode = SCANCODE_KEYPAD_6;           break;              // KP 6
        case XK_KP_7:           scancode = SCANCODE_KEYPAD_7;           break;              // KP 7
        case XK_KP_8:           scancode = SCANCODE_KEYPAD_8;           break;              // KP 8
        case XK_KP_9:           scancode = SCANCODE_KEYPAD_9;           break;              // KP 9
        case XK_F1:             scancode = SCANCODE_F1;                 break;              // F1
        case XK_F2:             scancode = SCANCODE_F2;                 break;              // F2
        case XK_F3:             scancode = SCANCODE_F3;                 break;              // F3
        case XK_F4:             scancode = SCANCODE_F4;                 break;              // F4
        case XK_F5:             scancode = SCANCODE_F5;                 break;              // F5
        case XK_F6:             scancode = SCANCODE_F6;                 break;              // F6
        case XK_F7:             scancode = SCANCODE_F7;                 break;              // F7
        case XK_F8:             scancode = SCANCODE_F8;                 break;              // F8
        case XK_F9:             scancode = SCANCODE_F9;                 break;              // F9
        case XK_F10:            scancode = SCANCODE_F10;                break;              // F10
        case XK_F11:            scancode = SCANCODE_F11;                break;              // F11
        case XK_F12:            scancode = SCANCODE_F12;                break;              // F12
        case XK_Shift_L:        scancode = SCANCODE_LSHIFT;             break;              // Left Shift
        case XK_Shift_R:        scancode = SCANCODE_RSHIFT;             break;              // Right Shift
        case XK_Control_L:      scancode = SCANCODE_LCTRL;              break;              // Left Ctrl
        case XK_Control_R:      scancode = SCANCODE_RCTRL;              break;              // Right Ctrl
        case XK_Alt_L:          scancode = SCANCODE_LALT;               break;              // Left Alt
        case XK_Alt_R:          scancode = SCANCODE_RALT;               break;              // Right Alt
        case XK_Delete:         scancode = SCANCODE_DELETE;             break;              // Delete

        default:
        {
            if (keysym <= 0x00FF && event.xkey.keycode < MAX_KEYCODES)
            {
                scancode = keycode_to_scancode[event.xkey.keycode];
            }
            else
            {
                scancode = 0;
            }
        }
    }

#ifdef DEBUG
    if (scancode == 0)
    {
        debug_printf ("get_scancode_from_keycode: unknown key: keycode = 0x%02X, keysym = 0x%04lX\n", event.xkey.keycode, keysym);
    }
#endif

    return scancode;
}

void
x11_event (void)
{
    uint16_t scancode;

    long event_mask = KeyPressMask | KeyReleaseMask | ExposureMask | FocusChangeMask;
    // XNextEvent(display, &event);

    if (XCheckWindowEvent(display, win, event_mask, &event))
    {
        if (event.type == KeyPress)
        {
            if (event.xkey.keycode < MAX_KEYCODES)
            {
                scancode = get_scancode_from_keycode (event.xkey.keycode);
                debug_printf ("KeyPress: 0x%02X -> 0x%04X\n", event.xkey.keycode, scancode);

                if (lxmapkey_menu_enabled)
                {
                    lxmapkey_menu_scancode = scancode;
                }
                else
                {
                    lxkeypress (scancode);
                }

                if (scancode == SCANCODE_F12)                                                  // F12
                {
                    steccy_exit = 1;
                }

                debug_printf ("KeyPress: 0x%02X -> 0x%04X\n", event.xkey.keycode, scancode);
            }
        }
        else if (event.type == KeyRelease)
        {
            if (event.xkey.keycode < MAX_KEYCODES)
            {
                scancode = get_scancode_from_keycode (event.xkey.keycode);
                debug_printf ("KeyRelease: 0x%02X -> 0x%04X\n", event.xkey.keycode, scancode);

                if (! lxmapkey_menu_enabled)
                {
                    lxkeyrelease (scancode);
                }
                debug_printf ("KeyRelease: 0x%02X -> 0x%04X\n", event.xkey.keycode, scancode);
            }
        }
        else if (event.type == Expose)
        {
            if (lxmapkey_menu_enabled)
            {
                lxmapkey_menu_scancode = SCANCODE_REDRAW;
            }
            else
            {
                menu_redraw (0xFF);
                z80_display_cached = 0;
            }
            debug_printf( "Expose %d\n", event.type);
        }
        else if (event.type == FocusIn)
        {
            XAutoRepeatOff (display);
            debug_printf( "FocusIn\n");
        }
        else if (event.type == FocusOut)
        {
            XAutoRepeatOn (display);
            debug_printf( "FocusOut\n");
        }
    }

    if (XCheckTypedWindowEvent(display, win, ClientMessage, &event))
    {
        if (event.xclient.message_type == wm_protocols && (Atom) event.xclient.data.l[0] == wm_delete_message)
        {
            steccy_exit = 1;
        }
        debug_printf( "ClientMessage\n");
    }
}

void
fill_rectangle (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    XSetForeground(display, gc, color);
    XFillRectangle(display, win, gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

void
draw_rectangle (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    XSetForeground(display, gc, color);
    XDrawRectangle(display, win, gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

void
x11_flush (void)
{
    XFlush(display);
    XSync(display, False);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * x11_deinit - deinit X11 routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
x11_deinit (void)
{
    XAutoRepeatOn (display);
    XCloseDisplay(display);
}
