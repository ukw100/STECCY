/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxmenu.c - menus
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2020 Frank Meyer - frank(at)uclock.de
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>

#include "z80.h"
#include "zxscr.h"
#if defined FRAMEBUFFER
#include "lxfb.h"
#include "lxkbd.h"
#elif defined X11
#include "lxx11.h"
#endif
#include "lxdisplay.h"
#include "lxmapkey.h"
#include "lxfont.h"
#include "lxmenu.h"
#include "scancodes.h"
#include "lxjoystick.h"

#define TRUE                    1
#define FALSE                   0

#define ENTRY_TYPE_NONE         0
#define ENTRY_TYPE_ALERT        1

#define MENU_ENTRY_JOYSTICK     0
#define MENU_ENTRY_RESET        1
#define MENU_ENTRY_ROM          2
#define MENU_ENTRY_LOAD         3
#define MENU_ENTRY_SAVE         4
#define MENU_ENTRY_SNAPSHOT     5
#define MENU_ENTRY_AUTOSTART    6
#define N_MENUS                 7

#define MAXENTRYLEN             20                                                      // max number of characters per entry
#define MAXENTRYNAMELEN         "20"                                                    // should be MAXENTRYLEN as string

#define MAXFILES                64                                                      // max number of files in list

#define MENU_START_X            ((zx_display_width - 800) / 2 + 600)                    // rectangle window for menu
#define MENU_START_Y            ((zx_display_height - (2 * ZX_SPECTRUM_DISPLAY_ROWS + 2 * 32)) / 2)
#define MENU_END_X              (zx_display_width - 1)
#define MENU_END_Y              (zx_display_height - 1)
#define MENU_LOAD_ENTRIES       20                                                      // max number of visible file entries in list
#define MENU_START_Y_OFFSET     32                                                      // y position of first menu entry
#define MENU_STEP_Y             32                                                      // y step for next line in menu
#define MENU_LOAD_STEP_Y        20                                                      // y step for next line in load menu
#define MENU_SAVE_STEP_Y        20                                                      // y step for next line in save menu

#define COLOR_BLACK             0x00000000                                              // black
#define COLOR_BLUE              0x000000F0                                              // blue
#define COLOR_RED               0x00F00000                                              // red
#define COLOR_MAGENTA           0x00F000F0                                              // magenta
#define COLOR_GREEN             0x0000F000                                              // green
#define COLOR_CYAN              0x0000F0F0                                              // cyan
#define COLOR_YELLOW            0x00F0F000                                              // yellow
#define COLOR_WHITE             0x00F0F0F0                                              // white

#define MAX_FILENAMESIZE        256

static const char *             autostart_entries[2] =
{
        "Autostart: No",
        "Autostart: Yes"
};

static char                     files[MAXFILES][MAX_FILENAMESIZE];
static uint_fast8_t             nfiles = 0;

uint32_t
menu_getscancode (void)
{
    uint32_t scancode;

    while (lxmapkey_menu_scancode == 0x0000)
    {
        usleep (10000);
#if defined (X11)
        x11_event ();
#endif
    }

    scancode = lxmapkey_menu_scancode;
    lxmapkey_menu_scancode = 0x0000;

    if (scancode == SCANCODE_REDRAW)
    {
        z80_display_cached = 0;
        zxscr_update_display ();
    }

    return scancode;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_menu_entry () - draw menu entry
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_menu_entry (uint_fast8_t menuidx, const char * str, uint_fast8_t activeidx, uint_fast8_t entry_type, uint8_t step_y)
{
    static char     buf[MAXENTRYLEN + 1];
    uint32_t        fcolor;
    uint32_t        bcolor;

    if (activeidx == menuidx)
    {
        fcolor = COLOR_RED;
    }
    else if (entry_type == ENTRY_TYPE_ALERT)
    {
        fcolor = COLOR_YELLOW;
    }
    else
    {
        fcolor = COLOR_WHITE;
    }

    bcolor = COLOR_BLACK;
    snprintf (buf, MAXENTRYLEN + 1, "%-" MAXENTRYNAMELEN "s", str);
    draw_string ((unsigned char *) buf, MENU_START_Y + MENU_START_Y_OFFSET + menuidx * step_y, MENU_START_X, fcolor, bcolor);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_menu_load () - list files for selection
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_menu_load (uint_fast8_t offsetidx, uint_fast8_t activeidx)
{
    uint_fast8_t    widx;
    uint_fast8_t    idx;

    for (idx = offsetidx, widx = 0; idx < nfiles && widx < MENU_LOAD_ENTRIES; idx++, widx++)
    {
        draw_menu_entry (widx, files[idx], activeidx - offsetidx, ENTRY_TYPE_NONE, MENU_LOAD_STEP_Y);
    }
    return;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * files_cmp () - compare filenames to sort
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
filescmp (const void * p1, const void * p2)
{
    return strcasecmp ((const char *) p1, (const char *) p2);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_load () - handle menu for file selection
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static char *
menu_load (char * path, uint_fast8_t romfiles)
{
    DIR *           dir;
    struct dirent * dp;
    uint8_t         len;
    uint_fast8_t    do_break    = 0;
    uint_fast8_t    activeitem  = 0;
    uint_fast16_t   scancode;
    char *          fname       = (char *) 0;
    uint_fast8_t    offsetidx   = 0;

    fill_rectangle (MENU_START_X, MENU_START_Y, MENU_END_X, MENU_END_Y, COLOR_BLACK);                      // erase menu

    nfiles = 0;

    if (! *path)
    {
        path = ".";
    }

    dir = opendir (path);

    if (dir)
    {
        for (;;)
        {
            dp = readdir (dir);

            if (! dp)
            {
                break;
            }

            len = strlen (dp->d_name);

            if (len > 4)
            {
                uint_fast8_t    do_display = 0;
                char *          p = dp->d_name + len - 4;

                if (romfiles)
                {
                    if (! strcasecmp (p, ".rom"))
                    {
                        do_display = 1;
                    }
                }
                else
                {
                    if (! strcasecmp (p, ".tap") || ! strcasecmp (p, ".tzx") || ! strcasecmp (p, ".z80"))
                    {
                        do_display = 1;
                    }
                }

                if (do_display)
                {
                    if (nfiles < MAXFILES)
                    {
                        strcpy (files[nfiles], dp->d_name);
                        nfiles++;
                    }
                }
            }
        }

        closedir (dir);
    }

    if (nfiles > 1)
    {
        qsort (files, nfiles, MAX_FILENAMESIZE, filescmp);
    }

    draw_menu_load (offsetidx, activeitem);

    while (! do_break && ! steccy_exit)
    {
        scancode = menu_getscancode();

        if (scancode == SCANCODE_ESC)
        {
            break;
        }

        switch (scancode)
        {
            case SCANCODE_REDRAW:
            {
                draw_menu_load (offsetidx, activeitem);
                break;
            }               
            case SCANCODE_D_ARROW:
            case SCANCODE_D_ARROW_EXT:
            {
                if (activeitem < nfiles - 1)
                {
                    activeitem++;

                    if (activeitem - offsetidx > MENU_LOAD_ENTRIES - 1)
                    {
                        offsetidx = activeitem - MENU_LOAD_ENTRIES + 1;
                    }

                    draw_menu_load (offsetidx, activeitem);
                }
                break;
            }
            case SCANCODE_U_ARROW:
            case SCANCODE_U_ARROW_EXT:
            {
                if (activeitem > 0)
                {
                    activeitem--;

                    if (activeitem < offsetidx)
                    {
                        offsetidx = activeitem;
                    }

                    draw_menu_load (offsetidx, activeitem);
                }
                break;
            }
            case SCANCODE_ENTER:
            case SCANCODE_SPACE:
            {
                // printf ("activeitem = %d\r\n", activeitem);
                fname = files[activeitem];
                do_break = 1;
                break;
            }
        }
    }

    fill_rectangle (MENU_START_X, MENU_START_Y, MENU_END_X, MENU_END_Y, COLOR_BLACK);                       // erase file menu
    return fname;
}

static void
draw_input_fname_field (uint32_t x, uint32_t y, char * fname_buf)
{
    draw_string ((unsigned char *) fname_buf, y, x, COLOR_WHITE, COLOR_BLACK);
    x += 8 * strlen (fname_buf);
    draw_string ((unsigned char *) " ", y, x, COLOR_WHITE, COLOR_RED);
    x += 8;
    draw_string ((unsigned char *) " ", y, x, COLOR_WHITE, COLOR_BLACK);
}

#define MAX_FNAME_INPUT_LEN         16

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_save () - handle save to file menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static char *
menu_save (uint8_t is_snapshot)
{
    static char     fname_buf[MAX_FNAME_INPUT_LEN + 1];                                                         // static, is return value!
    uint_fast8_t    do_break    = 0;
    uint32_t        scancode;
    uint_fast8_t    len;
    uint_fast8_t    ch;
    char *          fname       = (char *) 0;

    fill_rectangle (MENU_START_X, MENU_START_Y, MENU_END_X, MENU_END_Y, COLOR_BLACK);                        // erase menu

    if (is_snapshot)
    {
        draw_string ((unsigned char *) "Snapshot:", MENU_START_Y + MENU_START_Y_OFFSET, MENU_START_X, COLOR_WHITE, COLOR_BLACK);
    }
    else
    {
        draw_string ((unsigned char *) "Save to file:", MENU_START_Y + MENU_START_Y_OFFSET, MENU_START_X, COLOR_WHITE, COLOR_BLACK);
    }

    fname_buf[0] = '\0';
    len = 0;

    draw_input_fname_field (MENU_START_X, MENU_START_Y + MENU_START_Y_OFFSET + MENU_SAVE_STEP_Y, fname_buf);

    while (! do_break && ! steccy_exit)
    {
        ch = '\0';

        scancode = menu_getscancode();

        if (scancode == SCANCODE_ESC || scancode == SCANCODE_REDRAW)
        {
            break;
        }

        switch (scancode)
        {
            case SCANCODE_A:        ch = 'a';   break;
            case SCANCODE_B:        ch = 'b';   break;
            case SCANCODE_C:        ch = 'c';   break;
            case SCANCODE_D:        ch = 'd';   break;
            case SCANCODE_E:        ch = 'e';   break;
            case SCANCODE_F:        ch = 'f';   break;
            case SCANCODE_G:        ch = 'g';   break;
            case SCANCODE_H:        ch = 'h';   break;
            case SCANCODE_I:        ch = 'i';   break;
            case SCANCODE_J:        ch = 'j';   break;
            case SCANCODE_K:        ch = 'k';   break;
            case SCANCODE_L:        ch = 'l';   break;
            case SCANCODE_M:        ch = 'm';   break;
            case SCANCODE_N:        ch = 'n';   break;
            case SCANCODE_O:        ch = 'o';   break;
            case SCANCODE_P:        ch = 'p';   break;
            case SCANCODE_Q:        ch = 'q';   break;
            case SCANCODE_R:        ch = 'r';   break;
            case SCANCODE_S:        ch = 's';   break;
            case SCANCODE_T:        ch = 't';   break;
            case SCANCODE_U:        ch = 'u';   break;
            case SCANCODE_V:        ch = 'v';   break;
            case SCANCODE_W:        ch = 'w';   break;
            case SCANCODE_X:        ch = 'x';   break;
            case SCANCODE_Y:        ch = 'z';   break;
            case SCANCODE_Z:        ch = 'y';   break;
            case SCANCODE_0:        ch = '0';   break;
            case SCANCODE_1:        ch = '1';   break;
            case SCANCODE_2:        ch = '2';   break;
            case SCANCODE_3:        ch = '3';   break;
            case SCANCODE_4:        ch = '4';   break;
            case SCANCODE_5:        ch = '5';   break;
            case SCANCODE_6:        ch = '6';   break;
            case SCANCODE_7:        ch = '7';   break;
            case SCANCODE_8:        ch = '8';   break;
            case SCANCODE_9:        ch = '9';   break;
            case SCANCODE_MINUS:    ch = '-';   break;
            case SCANCODE_BACKSPACE:
            {
                if (len > 0)
                {
                    len--;
                    fname_buf[len] = '\0';
                    draw_input_fname_field (MENU_START_X, MENU_START_Y + MENU_START_Y_OFFSET + MENU_SAVE_STEP_Y, fname_buf);
                }
                break;
            }
            case SCANCODE_ENTER:
            {
                if (len > 0)
                {
                    if (is_snapshot)
                    {
                        strcat (fname_buf, ".z80");
                    }
                    else
                    {
                        strcat (fname_buf, ".tzx");
                    }

                    fname = fname_buf;
                    do_break = 1;
                }
                break;
            }
        }

        if (ch)
        {
            if (len < MAX_FNAME_INPUT_LEN)
            {
                fname_buf[len] = ch;
                len++;
                fname_buf[len] = '\0';
                draw_input_fname_field (MENU_START_X, MENU_START_Y + MENU_START_Y_OFFSET + MENU_SAVE_STEP_Y, fname_buf);
            }
        }
    }

    fill_rectangle (MENU_START_X, MENU_START_Y, MENU_END_X, MENU_END_Y, COLOR_BLACK);                      // erase file menu
    return fname;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_menu () - draw setup menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_menu (uint_fast8_t activeidx, uint_fast8_t menu_stop_active)
{
    draw_menu_entry (MENU_ENTRY_JOYSTICK, joystick_names[joystick_type], activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    draw_menu_entry (MENU_ENTRY_RESET, "RESET", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    draw_menu_entry (MENU_ENTRY_ROM, "ROM", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    draw_menu_entry (MENU_ENTRY_LOAD, "LOAD", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);

    if (menu_stop_active)
    {
        draw_menu_entry (MENU_ENTRY_SAVE, "Stop Record", activeidx, ENTRY_TYPE_ALERT, MENU_STEP_Y);
    }
    else
    {
        draw_menu_entry (MENU_ENTRY_SAVE, "SAVE", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    }

    draw_menu_entry (MENU_ENTRY_SNAPSHOT, "SNAPSHOT", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    draw_menu_entry (MENU_ENTRY_AUTOSTART, autostart_entries[z80_get_autostart()], activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);

    if (z80_romsize == 0x4000)
    {
        draw_string ((unsigned char *) " 48K", MENU_START_Y + MENU_START_Y_OFFSET + 13 * MENU_STEP_Y, MENU_END_X - 5 * 8, COLOR_RED, COLOR_BLACK);
    }
    else
    {
        draw_string ((unsigned char *) "128K", MENU_START_Y + MENU_START_Y_OFFSET + 13 * MENU_STEP_Y, MENU_END_X - 5 * 8, COLOR_RED, COLOR_BLACK);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu () - handle setup menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu (char * path)
{
    static uint_fast8_t     menu_stop_active = 0;
    char                    buf[256];
    uint_fast8_t            activeitem = 0;
    uint32_t                scancode;
    uint_fast8_t            do_break = 0;

    draw_menu (activeitem, menu_stop_active);

    while (! do_break && ! steccy_exit)
    {
        scancode = menu_getscancode ();

        if (scancode == SCANCODE_ESC)
        {
            break;
        }

        switch (scancode)
        {
            case SCANCODE_REDRAW:
            {
                draw_menu (activeitem, menu_stop_active);
                break;
            }
            case SCANCODE_D_ARROW:
            case SCANCODE_D_ARROW_EXT:
            {
                if (activeitem < N_MENUS - 1)
                {
                    activeitem++;
                    draw_menu (activeitem, menu_stop_active);
                }
                break;
            }
            case SCANCODE_U_ARROW:
            case SCANCODE_U_ARROW_EXT:
            {
                if (activeitem > 0)
                {
                    activeitem--;
                    draw_menu (activeitem, menu_stop_active);
                }
                break;
            }
            case SCANCODE_SPACE:
            case SCANCODE_ENTER:
            {
                // printf ("activeitem = %d\r\n", activeitem);

                switch (activeitem)
                {
                    case MENU_ENTRY_JOYSTICK:
                    {
                        joystick_type++;

                        if (joystick_type == N_JOYSTICKS)
                        {
                            joystick_type = 0;
                        }
                        draw_menu (activeitem, menu_stop_active);
                        break;
                    }
                    case MENU_ENTRY_AUTOSTART:
                    {
                        if (z80_get_autostart())
                        {
                            z80_set_autostart (0);
                        }
                        else
                        {
                            z80_set_autostart (1);
                        }
                        draw_menu (activeitem, menu_stop_active);
                        break;
                    }
                    case MENU_ENTRY_RESET:
                    {
                        z80_reset ();
                        do_break = 1;
                        break;
                    }
                    case MENU_ENTRY_ROM:
                    {
                        char * fname = menu_load (path, 1);

                        if (fname)
                        {
                            snprintf (buf, 255, "%s/%s", path, fname);
                            z80_set_fname_rom (buf);
                            do_break = 1;
                        }
                        else
                        {
                            draw_menu (activeitem, menu_stop_active);
                        }
                        break;
                    }
                    case MENU_ENTRY_LOAD:
                    {
                        char * fname = menu_load (path, 0);

                        if (fname)
                        {
                            snprintf (buf, 255, "%s/%s", path, fname);
                            z80_set_fname_load (buf);
                            do_break = 1;
                        }
                        else
                        {
                            draw_menu (activeitem, menu_stop_active);
                        }
                        break;
                    }
                    case MENU_ENTRY_SAVE:
                    {
                        if (menu_stop_active)
                        {
                            z80_close_fname_save ();
                            menu_stop_active = 0;
                            do_break = 1;
                        }
                        else
                        {
                            char * fname = menu_save (0);

                            if (fname)
                            {
                                snprintf (buf, 255, "%s/%s", path, fname);
                                z80_set_fname_save (buf);
                                menu_stop_active = 1;
                                do_break = 1;
                            }
                            else
                            {
                                draw_menu (activeitem, menu_stop_active);
                            }
                        }
                        break;
                    }
                    case MENU_ENTRY_SNAPSHOT:
                    {
                        char * fname = menu_save (1);

                        if (fname)
                        {
                            snprintf (buf, 255, "%s/%s", path, fname);
                            z80_set_fname_save_snapshot (buf);
                            do_break = 1;
                        }
                        else
                        {
                            draw_menu (activeitem, menu_stop_active);
                        }
                        break;
                    }
                }
                break;
            }
        }
    }

    draw_menu (0xFF, menu_stop_active);

    lxmapkey_disable_menu ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_redraw () - redraw menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu_redraw (void)
{
    draw_menu (0xFF, 0);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_init () - initialize menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu_init (void)
{
    set_font (FONT_08x12);
    draw_menu (0xFF, 0);
}
