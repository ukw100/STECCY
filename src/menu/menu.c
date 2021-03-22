/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu.c - handle menus
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2019-2021 Frank Meyer - frank(at)uclock.de
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
#include <string.h>
#include "ps2key.h"
#include "usb_hid_host.h"
#include "keypress.h"
#include "wii-gamepad.h"
#include "joystick.h"
#include "tft.h"
#include "font.h"
#include "fs-stdio.h"
#include "z80.h"
#include "zxio.h"

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

#if TFT_WIDTH == 320
#define MENU_START_Y_OFFSET     0                                                       // y position of first menu entry
#define MENU_STEP_Y             16                                                      // y step for next line in menu
#define MENU_LOAD_STEP_Y        16                                                      // y step for next line in load menu
#define MENU_SAVE_STEP_Y        16                                                      // y step for next line in save menu
#define MENU_HEIGHT             (N_MENUS * MENU_STEP_Y)
#define MENU_START_X            80                                                      // rectangle window for menu
#define MENU_START_Y            ((TFT_HEIGHT - MENU_HEIGHT) / 2)
#define MENU_END_X              (TFT_WIDTH - MENU_START_X - 1)
#define MENU_END_Y              (MENU_START_Y + MENU_HEIGHT - 1)
#define MENU_LOAD_ENTRIES       N_MENUS                                                 // max number of visible file entries in list
#else
#define MENU_START_X            600                                                     // rectangle window for menu
#define MENU_START_Y            0
#define MENU_END_X              (TFT_WIDTH - 1)
#define MENU_END_Y              (TFT_HEIGHT - 1)
#define MENU_LOAD_ENTRIES       20                                                      // max number of visible file entries in list
#define MENU_START_Y_OFFSET     32                                                      // y position of first menu entry
#define MENU_STEP_Y             32                                                      // y step for next line in menu
#define MENU_LOAD_STEP_Y        20                                                      // y step for next line in load menu
#define MENU_SAVE_STEP_Y        20                                                      // y step for next line in save menu
#endif

#define BLACK565                0x0000
#define RED565                  0xF800
#define GREEN565                0x07E0
#define BLUE565                 0x001F
#define YELLOW565               (RED565   | GREEN565)
#define MAGENTA565              (RED565   | BLUE565)
#define CYAN565                 (GREEN565 | BLUE565)
#define WHITE565                0xFFFF

#define MAX_FILENAMESIZE        16

static char *                   autostart_entries[2] = { "Autostart: No", "Autostart: Yes" };
static char                     files[MAXFILES][MAX_FILENAMESIZE];
static uint_fast8_t             nfiles = 0;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_menu_entry () - draw menu entry
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_menu_entry (uint_fast8_t menuidx, const char * str, uint_fast8_t activeidx, uint_fast8_t entry_type, uint8_t step_y)
{
    static char             buf[MAXENTRYLEN + 1];
    uint_fast16_t           fcolor565;
    uint_fast16_t           bcolor565;

    if (activeidx == menuidx)
    {
        fcolor565 = RED565;
    }
    else if (entry_type == ENTRY_TYPE_ALERT)
    {
        fcolor565 = YELLOW565;
    }
    else
    {
        fcolor565 = WHITE565;
    }

    bcolor565 = BLACK565;
    snprintf (buf, MAXENTRYLEN + 1, "%-" MAXENTRYNAMELEN "s", str);
    draw_string ((unsigned char *) buf, MENU_START_Y + MENU_START_Y_OFFSET + menuidx * step_y, MENU_START_X, fcolor565, bcolor565);
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
    return strcmp ((const char *) p1, (const char *) p2);
}

static uint_fast16_t    last_joy_scancode = SCANCODE_NONE;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_get_scancode () - get scancode from keryboard or joystick
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast16_t
menu_get_scancode ()
{
    uint_fast16_t scancode;

    scancode = ps2key_getscancode ();

    if (scancode == SCANCODE_NONE)
    {
        if (joystick_is_online)
        {
            if (joystick_state == JOYSTICK_STATE_BUSY)
            {
                if (joystick_finished_transmission (FALSE))
                {
                    uint_fast16_t   joy_scancode = joystick_to_scancode ();

                    if (last_joy_scancode != joy_scancode)
                    {
                        last_joy_scancode = joy_scancode;
                        scancode = joy_scancode;
                    }
                }
            }
            else if (joystick_state == JOYSTICK_STATE_IDLE)
            {
                joystick_start_transmission ();
            }
        }
    }

    return scancode;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_wait_until_joystick_released () - wait until all joystick buttons have been released
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
menu_wait_until_joystick_released ()
{
    if (joystick_is_online)
    {
        while (1)
        {
            if (joystick_state == JOYSTICK_STATE_IDLE)
            {
                joystick_start_transmission ();
            }

            if (joystick_finished_transmission (FALSE))
            {
                if (joystick_to_scancode() == SCANCODE_NONE)
                {
                    last_joy_scancode = SCANCODE_NONE;
                    break;
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_load () - handle menu for file selection
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static char *
menu_load (uint_fast8_t romfiles)
{
    static FILINFO  fno;
    FRESULT         res;
    DIR             dir;
    uint8_t         len;
    uint_fast8_t    do_break    = 0;
    uint_fast8_t    activeitem  = 0;
    uint_fast16_t   scancode;
    char *          fname       = (char *) 0;
    uint_fast8_t    offsetidx   = 0;

    tft_fill_rectangle (MENU_START_X, MENU_START_Y, MENU_END_X, MENU_END_Y, BLACK565);                      // erase menu

    nfiles = 0;
    res = f_opendir (&dir, "/");

    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir (&dir, &fno);

            if (res != FR_OK || fno.fname[0] == 0)                                      // end of directory entries: fname is empty
            {
                break;
            }

            len = strlen (fno.fname);

            if (len > 4)
            {
                uint_fast8_t    do_display = 0;
                char *          p = fno.fname + len - 4;

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
                        strcpy (files[nfiles], fno.fname);
                        nfiles++;
                    }
                }
            }
        }

        f_closedir (&dir);
    }

    if (nfiles > 1)
    {
        qsort (files, nfiles, MAX_FILENAMESIZE, filescmp);
    }

    draw_menu_load (offsetidx, activeitem);
    menu_wait_until_joystick_released ();

    while (! do_break && (scancode = menu_get_scancode()) != SCANCODE_ESC)
    {
        switch (scancode)
        {
            case SCANCODE_D_ARROW:
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
        usb_hid_host_process (TRUE);
    }

    tft_fill_rectangle (MENU_START_X, MENU_START_Y, MENU_END_X, MENU_END_Y, BLACK565);                      // erase file menu
    return fname;
}

static void
draw_input_fname_field (uint_fast16_t x, uint_fast16_t y, char * fname_buf)
{
    draw_string ((unsigned char *) fname_buf, y, x, WHITE565, BLACK565);
    x += 8 * strlen (fname_buf);
    draw_string ((unsigned char *) " ", y, x, WHITE565, RED565);
    x += 8;
    draw_string ((unsigned char *) " ", y, x, WHITE565, BLACK565);
}

#define MAX_FNAME_INPUT_LEN         8

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_save () - handle save to file menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static char *
menu_save (uint8_t is_snapshot)
{
    static char     fname_buf[16];                                                                          // static, is return value!
    uint_fast8_t    do_break    = 0;
    uint_fast16_t   scancode;
    uint_fast8_t    len;
    uint_fast8_t    ch;
    char *          fname       = (char *) 0;

    tft_fill_rectangle (MENU_START_X, MENU_START_Y, MENU_END_X, MENU_END_Y, BLACK565);                      // erase menu

    if (is_snapshot)
    {
        draw_string ((unsigned char *) "Snapshot:", MENU_START_Y + MENU_START_Y_OFFSET, MENU_START_X, WHITE565, BLACK565);
    }
    else
    {
        draw_string ((unsigned char *) "Save to file:", MENU_START_Y + MENU_START_Y_OFFSET, MENU_START_X, WHITE565, BLACK565);
    }

    fname_buf[0] = '\0';
    len = 0;

    draw_input_fname_field (MENU_START_X, MENU_START_Y + MENU_START_Y_OFFSET + MENU_SAVE_STEP_Y, fname_buf);
    menu_wait_until_joystick_released ();

    while (! do_break && (scancode = menu_get_scancode()) != SCANCODE_ESC)
    {
        ch = '\0';

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
            case SCANCODE_Y:        ch = 'y';   break;
            case SCANCODE_Z:        ch = 'z';   break;
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
            case SCANCODE_BSP:
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
            if (len < MAX_FNAME_INPUT_LEN - 1)
            {
                fname_buf[len] = ch;
                len++;
                fname_buf[len] = '\0';
                draw_input_fname_field (MENU_START_X, MENU_START_Y + MENU_START_Y_OFFSET + MENU_SAVE_STEP_Y, fname_buf);
            }
        }
        usb_hid_host_process (TRUE);
    }

    tft_fill_rectangle (MENU_START_X, MENU_START_Y, MENU_END_X, MENU_END_Y, BLACK565);                      // erase file menu
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
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu () - handle setup menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu (void)
{
    static uint_fast8_t     menu_stop_active = 0;
    uint_fast8_t            activeitem = 0;
    uint_fast16_t           scancode;
    uint_fast8_t            do_break = 0;

#if TFT_WIDTH == 320
    tft_fill_rectangle (MENU_START_X - 8, MENU_START_Y - 8, MENU_END_X + 8, MENU_END_Y + 8, BLACK565);              // erase menu block
    tft_draw_rectangle (MENU_START_X - 8, MENU_START_Y - 8, MENU_END_X + 8, MENU_END_Y + 8, RED565);                // draw thin red border
#endif

    draw_menu (activeitem, menu_stop_active);
    menu_wait_until_joystick_released ();

    while (! do_break && (scancode = menu_get_scancode()) != SCANCODE_ESC)
    {
        switch (scancode)
        {
            case SCANCODE_D_ARROW:
            {
                if (activeitem < N_MENUS - 1)
                {
                    activeitem++;
                    draw_menu (activeitem, menu_stop_active);
                }
                break;
            }
            case SCANCODE_U_ARROW:
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
                        zxio_reset ();
                        do_break = 1;
                        break;
                    }
                    case MENU_ENTRY_ROM:
                    {
                        char * fname = menu_load (1);

                        if (fname)
                        {
                            // printf ("z80_set_fname_rom (%s)\r\n", fname);
                            z80_set_fname_rom (fname);
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
                        char * fname = menu_load (0);

                        if (fname)
                        {
                            // printf ("z80_set_fname_load (%s)\r\n", fname);
                            z80_set_fname_load (fname);
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
                                // printf ("z80_set_fname_save (%s)\r\n", fname);
                                z80_set_fname_save (fname);
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
                            // printf ("z80_set_fname_save_snapshot (%s)\r\n", fname);
                            z80_set_fname_save_snapshot (fname);
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
        usb_hid_host_process (TRUE);
    }

    draw_menu (0xFF, menu_stop_active);
    ps2key_setmode (PS2KEY_CALLBACK_MODE);
    menu_wait_until_joystick_released ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_init () - initialize menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu_init (void)
{
    set_font (FONT_08x12);
#if TFT_WIDTH == 800                                                        // draw menu only if there is enough space on the right side
    draw_menu (0xFF, 0);
#endif
}
