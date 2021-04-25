/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxmenu.c - menus
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2020-2021 Frank Meyer - frank(at)uclock.de
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
#include "zxram.h"
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
#define ENTRY_TYPE_DISABLED     2

#define MENU_ENTRY_JOYSTICK     0
#define MENU_ENTRY_RESET        1
#define MENU_ENTRY_ROM          2
#define MENU_ENTRY_POKE         3
#define MENU_ENTRY_SAVE         4
#define MENU_ENTRY_SNAPSHOT     5
#define MENU_ENTRY_AUTOSTART    6
#define N_MENUS                 7

#define MAX_MAIN_ENTRY_LEN      16                                              // max number of characters per main entry
#define MAX_SUB_ENTRIES         128                                             // max number of entries in list
#define MAX_SUBENTRY_LEN        59                                              // max number of characters per sub entry
#define MENU_MAX_FILENAME_LEN   MAX_SUBENTRY_LEN                                // we can only show MAX_SUBENTRY_LEN chars

#define ZXSCR_ZOOM              2
#define ZXSCR_TOP_OFFSET        ((zx_display_height - ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS - 2 * ZX_SPECTRUM_BORDER_SIZE) / 2)
#define ZXSCR_LEFT_OFFSET       (((zx_display_width - 800) / 2) + 8)

#define MAIN_MENU_START_X       ((zx_display_width - 800) / 2 + 600)            // rectangle window for main menu
#define MAIN_MENU_START_Y       ((zx_display_height - (ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS + 2 * 32)) / 2)
#define MAIN_MENU_END_X         (zx_display_width - 1)                          // MAIN_MENU_END_Y not needed here

#define SUB_MENU_START_X        (ZXSCR_LEFT_OFFSET + ZX_SPECTRUM_BORDER_SIZE)   // rectangle window for sub menu
#define SUB_MENU_START_Y        (ZXSCR_TOP_OFFSET + ZX_SPECTRUM_BORDER_SIZE)
#define SUB_MENU_END_X          (ZXSCR_LEFT_OFFSET + ZX_SPECTRUM_BORDER_SIZE + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS)
#define SUB_MENU_END_Y          (ZXSCR_TOP_OFFSET + ZX_SPECTRUM_BORDER_SIZE  + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS)
#define SUB_MENU_X_OFFSET       16                                              // x position of first menu entry col

#define STATUS_Y                (zx_display_height - 14)

#define SUB_MENU_ENTRIES        22                                              // max number of visible entries in list
#define MENU_START_Y_OFFSET     16                                              // y position of first menu entry
#define MENU_STEP_Y             32                                              // y step for next line in menu
#define MENU_LOAD_STEP_Y        16                                              // y step for next line in load menu
#define MENU_SAVE_STEP_Y        16                                              // y step for next line in save menu

#define COLOR_BLACK             0x00000000                                      // black
#define COLOR_BLUE              0x000000F0                                      // blue
#define COLOR_RED               0x00F00000                                      // red
#define COLOR_MAGENTA           0x00F000F0                                      // magenta
#define COLOR_GREEN             0x0000F000                                      // green
#define COLOR_CYAN              0x0000F0F0                                      // cyan
#define COLOR_YELLOW            0x00F0F000                                      // yellow
#define COLOR_WHITE             0x00F0F0F0                                      // white
#define COLOR_GRAY              0x00909090                                      // gray

static const char *             autostart_entries[2] =
{
        "Autostart: No",
        "Autostart: Yes"
};

typedef union
{
    char                        files[MAX_SUB_ENTRIES][MENU_MAX_FILENAME_LEN + 1];
    long                        positions[MAX_SUB_ENTRIES];
} SUBENTRIES;

static SUBENTRIES               subentries;
static uint_fast8_t             n_subentries = 0;
static uint_fast8_t             menu_stop_active = 0;

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
 * draw_main_menu_entry () - draw main menu entry
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_main_menu_entry (uint_fast8_t idx, const char * str, uint_fast8_t actidx, uint_fast8_t entry_type, uint8_t step_y)
{
    char            buf[MAX_MAIN_ENTRY_LEN + 1];
    uint32_t        fcolor;
    uint32_t        bcolor;
    uint32_t        i;

    if (actidx == idx)
    {
        fcolor = COLOR_RED;
    }
    else if (entry_type == ENTRY_TYPE_ALERT)
    {
        fcolor = COLOR_YELLOW;
    }
    else if (entry_type == ENTRY_TYPE_DISABLED)
    {
        fcolor = COLOR_GRAY;
    }
    else
    {
        fcolor = COLOR_WHITE;
    }

    bcolor = COLOR_BLACK;

    for (i = 0; i < MAX_MAIN_ENTRY_LEN; i++)
    {
        if (str[i])
        {
            buf[i] = str[i];
        }
        else
        {
            break;
        }
    }

    while (i < MAX_MAIN_ENTRY_LEN)
    {
        buf[i++] = ' ';
    }
    buf[i] = '\0';

    draw_string ((unsigned char *) buf, MAIN_MENU_START_Y + MENU_START_Y_OFFSET + idx * step_y, MAIN_MENU_START_X, fcolor, bcolor);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_sub_menu_entry () - draw sub menu entry
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_sub_menu_entry (uint_fast8_t idx, const char * str, uint_fast8_t actidx, uint_fast8_t entry_type, uint8_t step_y)
{
    static char     buf[MAX_SUBENTRY_LEN + 1];
    uint32_t        fcolor;
    uint32_t        bcolor;
    uint32_t        i;

    if (actidx == idx)
    {
        fcolor = COLOR_RED;
    }
    else if (entry_type == ENTRY_TYPE_ALERT)
    {
        fcolor = COLOR_YELLOW;
    }
    else if (entry_type == ENTRY_TYPE_DISABLED)
    {
        fcolor = COLOR_GRAY;
    }
    else
    {
        fcolor = COLOR_WHITE;
    }

    bcolor = COLOR_BLACK;

    for (i = 0; i < MAX_SUBENTRY_LEN; i++)
    {
        if (str[i])
        {
            buf[i] = str[i];
        }
        else
        {
            break;
        }
    }

    while (i < MAX_SUBENTRY_LEN)
    {
        buf[i++] = ' ';
    }
    buf[i] = '\0';

    draw_string ((unsigned char *) buf, SUB_MENU_START_Y + MENU_START_Y_OFFSET + idx * step_y, SUB_MENU_START_X + SUB_MENU_X_OFFSET, fcolor, bcolor);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_menu_poke_entry () - draw menu poke entry
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_menu_poke_entry (FILE * fp, uint_fast8_t idx, long pos, uint_fast8_t actidx, uint_fast8_t entry_type, uint8_t step_y)
{
    static char     buf[MAX_SUBENTRY_LEN + 1];
    uint32_t        fcolor;
    uint32_t        bcolor;
    int             ch;
    uint32_t        i;

    if (actidx == idx)
    {
        fcolor = COLOR_RED;
    }
    else if (entry_type == ENTRY_TYPE_ALERT)
    {
        fcolor = COLOR_YELLOW;
    }
    else if (entry_type == ENTRY_TYPE_DISABLED)
    {
        fcolor = COLOR_GRAY;
    }
    else
    {
        fcolor = COLOR_WHITE;
    }

    bcolor = COLOR_BLACK;

    fseek (fp, pos, SEEK_SET);

    for (i = 0; i < MAX_SUBENTRY_LEN; i++)
    {
        ch = getc (fp);

        if (ch == '\r' || ch == '\n' || ch == EOF)
        {
            break;
        }
        buf[i] = ch;
    }

    while (i < MAX_SUBENTRY_LEN)
    {
        buf[i++] = ' ';
    }
    buf[i] = '\0';

    draw_string ((unsigned char *) buf, SUB_MENU_START_Y + MENU_START_Y_OFFSET + idx * step_y, SUB_MENU_START_X + SUB_MENU_X_OFFSET, fcolor, bcolor);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_sub_menu () - list subentries for selection
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_sub_menu (FILE * poke_fp, uint_fast8_t offsetidx, uint_fast8_t activeidx)
{
    uint_fast8_t    widx;
    uint_fast8_t    idx;

    if (poke_fp)
    {
        for (idx = offsetidx, widx = 0; idx < n_subentries && widx < SUB_MENU_ENTRIES; idx++, widx++)
        {
            draw_menu_poke_entry (poke_fp, widx, subentries.positions[idx], activeidx - offsetidx, ENTRY_TYPE_NONE, MENU_LOAD_STEP_Y);
        }
    }
    else
    {
        for (idx = offsetidx, widx = 0; idx < n_subentries && widx < SUB_MENU_ENTRIES; idx++, widx++)
        {
            draw_sub_menu_entry (widx, subentries.files[idx], activeidx - offsetidx, ENTRY_TYPE_NONE, MENU_LOAD_STEP_Y);
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * filescmp () - compare filenames to sort
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
filescmp (const void * p1, const void * p2)
{
    return strcasecmp ((const char *) p1, (const char *) p2);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_draw_rectangle () - draw rectangle for sub menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
menu_draw_rectangle (void)
{
    fill_rectangle (SUB_MENU_START_X, SUB_MENU_START_Y, SUB_MENU_END_X, SUB_MENU_END_Y, COLOR_BLACK);   // erase sub menu
    draw_rectangle (SUB_MENU_START_X, SUB_MENU_START_Y, SUB_MENU_END_X, SUB_MENU_END_Y, COLOR_RED);     // draw rectangle
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_erase_rectangle () - erase rectangle for sub menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
menu_erase_rectangle (void)
{
    z80_display_cached = 0;
    zxscr_update_display ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_handle_sub_menu () - handle user input in sub menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
menu_handle_sub_menu (FILE * poke_fp)
{
    uint_fast8_t    do_break    = 0;
    uint_fast8_t    activeitem  = 0;
    uint_fast8_t    offsetidx   = 0;
    uint_fast16_t   scancode;
    int             entry_idx   = -1;

    draw_sub_menu (poke_fp, 0, 0);

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
                draw_sub_menu (poke_fp, offsetidx, activeitem);
                break;
            }
            case SCANCODE_D_ARROW:
            case SCANCODE_D_ARROW_EXT:
            {
                if (activeitem < n_subentries - 1)
                {
                    activeitem++;

                    if (activeitem - offsetidx > SUB_MENU_ENTRIES - 1)
                    {
                        offsetidx = activeitem - SUB_MENU_ENTRIES + 1;
                    }

                    draw_sub_menu (poke_fp, offsetidx, activeitem);
                }
                break;
            }
            case SCANCODE_R_ARROW:
            case SCANCODE_R_ARROW_EXT:
            case SCANCODE_PG_DN:
            {
                uint_fast8_t newitem = activeitem + SUB_MENU_ENTRIES;

                if (newitem >= n_subentries)
                {
                    newitem = n_subentries - 1;
                }

                if (newitem > activeitem)
                {
                    offsetidx += newitem - activeitem;

                    if (offsetidx > n_subentries - SUB_MENU_ENTRIES)
                    {
                        offsetidx = n_subentries - SUB_MENU_ENTRIES;
                    }

                    activeitem = newitem;
                    draw_sub_menu (poke_fp, offsetidx, activeitem);
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

                    draw_sub_menu (poke_fp, offsetidx, activeitem);
                }
                break;
            }
            case SCANCODE_L_ARROW:
            case SCANCODE_L_ARROW_EXT:
            case SCANCODE_PG_UP:
            {
                uint_fast8_t newitem;

                if (activeitem > SUB_MENU_ENTRIES)
                {
                    newitem = activeitem - SUB_MENU_ENTRIES;
                }
                else
                {
                    newitem = 0;
                }

                if (newitem != activeitem)
                {
                    activeitem = newitem;

                    if (offsetidx > SUB_MENU_ENTRIES)
                    {
                        offsetidx -= SUB_MENU_ENTRIES;
                    }
                    else
                    {
                        offsetidx = 0;
                    }

                    draw_sub_menu (poke_fp, offsetidx, activeitem);
                }
                break;
            }
            case SCANCODE_ENTER:
            case SCANCODE_SPACE:
            {
                // printf ("activeitem = %d\r\n", activeitem);
                entry_idx = activeitem;
                do_break = 1;
                break;
            }
        }
    }

    return entry_idx;
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
    int             entry_idx;
    char *          fname       = (char *) 0;

    menu_draw_rectangle ();

    n_subentries = 0;

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
                    if (n_subentries < MAX_SUB_ENTRIES)
                    {
                        strncpy (subentries.files[n_subentries], dp->d_name, MENU_MAX_FILENAME_LEN);
                        subentries.files[n_subentries][MENU_MAX_FILENAME_LEN] = '\0';
                        n_subentries++;
                    }
                }
            }
        }

        closedir (dir);
    }

    if (n_subentries > 1)
    {
        qsort (subentries.files, n_subentries, MENU_MAX_FILENAME_LEN + 1, filescmp);
    }

    entry_idx = menu_handle_sub_menu ((FILE *) NULL);

    if (entry_idx >= 0)
    {
        fname = subentries.files[entry_idx];
    }

    menu_erase_rectangle ();
    return fname;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_do_poke () - do the poke
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
menu_do_poke (int entry)
{
    char            buf[128];
    char *          fname;
    int             entry_idx = 0;

    fname = z80_get_poke_file ();

    if (fname && *fname)
    {
        FILE * fp = fopen (fname, "r");

        if (fp)
        {
            while (fgets (buf, sizeof (buf), fp))
            {
                if (*buf == 'Y')
                {
                    break;
                }

                if (*buf == 'N')
                {
                    if (entry_idx == entry)
                    {
                        for (;;)
                        {
                            if (fgets (buf, sizeof (buf), fp))
                            {
                                if (buf[0] == 'M' || buf[0] == 'Z')
                                {
                                    int     values[4];
                                    int     cnt = 0;
                                    char *  p;

                                    for (p = buf + 1; *p; p++)
                                    {
                                        if (*p != ' ')
                                        {
                                            values[cnt++] = atoi (p);

                                            if (cnt == 4)
                                            {
                                                break;
                                            }

                                            while (*p && *p != ' ')
                                            {
                                                p++;
                                            }
                                        }
                                    }

                                    if (cnt == 4)
                                    {
                                        if (values[0] == 8)
                                        {
                                            zx_ram_set_8 (values[1], values[2]);
                                            // printf ("addr=%d val=%d\n", values[1], values[2]);
                                        }
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }

                        break;
                    }

                    entry_idx++;
                }
            }

            fclose (fp);
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_poke () - handle menu for poke entry selection
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
menu_poke (void)
{
    char            buf[128];
    char *          fname;
    int             entry_idx   = -1;
    size_t          pos;

    menu_draw_rectangle ();

    n_subentries = 0;

    fname = z80_get_poke_file ();

    if (fname && *fname)
    {
        FILE * fp = fopen (fname, "r");

        if (fp)
        {
            pos = 0;

            while (fgets (buf, sizeof (buf), fp))
            {
                if (*buf == 'Y')
                {
                    break;
                }

                if (*buf == 'N')
                {
                    if (n_subentries < MAX_SUB_ENTRIES)
                    {
                        subentries.positions[n_subentries] = pos + 1;
                        n_subentries++;
                    }
                }
                pos = ftell (fp);
            }

            entry_idx = menu_handle_sub_menu (fp);
            fclose (fp);
        }
    }

    menu_erase_rectangle ();
    return entry_idx;
}

char *
menu_start_load (char * path)
{
    char *          fname;

    z80_leave_focus ();
    lxmapkey_enable_menu ();
    fname = menu_load (path, 0);
    lxmapkey_disable_menu ();
    z80_enter_focus ();

    return fname;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_input_fname_field () - draw input fname field
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
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
    static char     fname_buf[MAX_FNAME_INPUT_LEN + 1];                                     // static, is return value!
    uint_fast8_t    do_break    = 0;
    uint32_t        scancode;
    uint_fast8_t    len;
    uint_fast8_t    ch;
    char *          p;
    char *          fname       = (char *) 0;
    int             xoffset;

    menu_draw_rectangle ();

    if (is_snapshot)
    {
        p = "Snapshot:";
    }
    else
    {
        p = "Save to file:";
    }

    draw_string ((unsigned char *) p, SUB_MENU_START_Y + MENU_START_Y_OFFSET, SUB_MENU_START_X + SUB_MENU_X_OFFSET, COLOR_WHITE, COLOR_BLACK);
    xoffset = SUB_MENU_START_X + SUB_MENU_X_OFFSET + 8 * (strlen (p) + 1);

    fname_buf[0] = '\0';
    len = 0;

    draw_input_fname_field (xoffset, SUB_MENU_START_Y + MENU_START_Y_OFFSET, fname_buf);

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
                    draw_input_fname_field (xoffset, SUB_MENU_START_Y + MENU_START_Y_OFFSET, fname_buf);
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
                draw_input_fname_field (xoffset, SUB_MENU_START_Y + MENU_START_Y_OFFSET, fname_buf);
            }
        }
    }

    menu_erase_rectangle ();
    return fname;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_update_status () - update status
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu_update_status (void)
{
    if (z80_get_turbo_mode ())
    {
        draw_string ((unsigned char *) "TURBO", STATUS_Y, MAIN_MENU_END_X - 19 * 8, COLOR_RED, COLOR_BLACK);
    }
    else
    {
        draw_string ((unsigned char *) "     ", STATUS_Y, MAIN_MENU_END_X - 19 * 8, COLOR_RED, COLOR_BLACK);
    }

    if (z80_get_rom_hooks ())
    {
        draw_string ((unsigned char *) "HOOKS", STATUS_Y, MAIN_MENU_END_X - 12 * 8, COLOR_RED, COLOR_BLACK);
    }
    else
    {
        draw_string ((unsigned char *) "     ", STATUS_Y, MAIN_MENU_END_X - 12 * 8, COLOR_RED, COLOR_BLACK);
    }

    if (z80_romsize == 0x4000)
    {
        draw_string ((unsigned char *) " 48K", STATUS_Y, MAIN_MENU_END_X - 5 * 8, COLOR_RED, COLOR_BLACK);
    }
    else
    {
        draw_string ((unsigned char *) "128K", STATUS_Y, MAIN_MENU_END_X - 5 * 8, COLOR_RED, COLOR_BLACK);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_main_menu () - draw main menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_main_menu (uint_fast8_t activeidx, uint_fast8_t poke_file_active)
{
    draw_main_menu_entry (MENU_ENTRY_JOYSTICK, joystick_names[joystick_type], activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    draw_main_menu_entry (MENU_ENTRY_RESET, "Reset CPU", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    draw_main_menu_entry (MENU_ENTRY_ROM, "Load ROM", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);

    if (poke_file_active)
    {
        draw_main_menu_entry (MENU_ENTRY_POKE, "Poke", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    }
    else
    {
        draw_main_menu_entry (MENU_ENTRY_POKE, "Poke", activeidx, ENTRY_TYPE_DISABLED, MENU_STEP_Y);
    }

    if (menu_stop_active)
    {
        draw_main_menu_entry (MENU_ENTRY_SAVE, "Stop Record", activeidx, ENTRY_TYPE_ALERT, MENU_STEP_Y);
    }
    else
    {
        draw_main_menu_entry (MENU_ENTRY_SAVE, "Record", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    }

    draw_main_menu_entry (MENU_ENTRY_SNAPSHOT, "Snapshot", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    draw_main_menu_entry (MENU_ENTRY_AUTOSTART, autostart_entries[z80_get_autostart()], activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);

    menu_update_status ();  // fm: really?
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu () - handle setup menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu (char * path, uint_fast8_t poke_file_active)
{
    char                    buf[256];
    uint_fast8_t            activeitem = 0;
    uint32_t                scancode;
    uint_fast8_t            do_break = 0;

    draw_main_menu (activeitem, poke_file_active);

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
                draw_main_menu (activeitem, poke_file_active);
                break;
            }
            case SCANCODE_D_ARROW:
            case SCANCODE_D_ARROW_EXT:
            {
                if (activeitem < N_MENUS - 1)
                {
                    activeitem++;

                    if (activeitem == MENU_ENTRY_POKE && ! poke_file_active)
                    {
                        if (activeitem + 1 < N_MENUS - 1)
                        {
                            activeitem++;
                        }
                        else
                        {
                            activeitem--;
                        }
                    }

                    draw_main_menu (activeitem, poke_file_active);
                }
                break;
            }
            case SCANCODE_U_ARROW:
            case SCANCODE_U_ARROW_EXT:
            {
                if (activeitem > 0)
                {
                    activeitem--;

                    if (activeitem == MENU_ENTRY_POKE && ! poke_file_active)
                    {
                        if (activeitem - 1 > 0)
                        {
                            activeitem--;
                        }
                        else
                        {
                            activeitem++;
                        }
                    }

                    draw_main_menu (activeitem, poke_file_active);
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
                        draw_main_menu (activeitem, poke_file_active);
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
                        draw_main_menu (activeitem, poke_file_active);
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
                            z80_load_rom (fname);
                            do_break = 1;
                        }
                        else
                        {
                            draw_main_menu (activeitem, poke_file_active);
                        }
                        break;
                    }

                    case MENU_ENTRY_POKE:
                    {
                        int entry = menu_poke ();

                        if (entry >= 0)
                        {
                            menu_do_poke (entry);
                            do_break = 1;
                        }
                        else
                        {
                            draw_main_menu (activeitem, poke_file_active);
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
                                draw_main_menu (activeitem, poke_file_active);
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
                            draw_main_menu (activeitem, poke_file_active);
                        }
                        break;
                    }
                }
                break;
            }
        }
    }

    draw_main_menu (0xFF, poke_file_active);
    lxmapkey_disable_menu ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_redraw () - redraw menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu_redraw (uint_fast8_t poke_file_active)
{
    static uint_fast8_t last_poke_file_active = 0;

    if (poke_file_active == 0xFF)                                   // if status unknown, take last status (see lxx11.c)
    {
        poke_file_active = last_poke_file_active;
    }
    else
    {
        last_poke_file_active = poke_file_active;
    }

    draw_main_menu (0xFF, poke_file_active);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_init () - initialize menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu_init (void)
{
    set_font (FONT_08x12);
    draw_main_menu (0xFF, 0);
}
