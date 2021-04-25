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
#include "zxscr.h"
#include "zxram.h"
#include "zxio.h"
#include "menu.h"

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

#define MAX_MAIN_ENTRY_LEN      16                                                      // max number of characters per main entry
#define MAX_SUB_ENTRIES         128                                                     // max number of entries in list
#define MENU_MAX_FILENAME_LEN   Z80_MAX_FILENAME_LEN                                    // only 8.3 possible with fatfs
#define MAX_FNAME_INPUT_LEN     8


#if TFT_WIDTH == 320

#define MAIN_MENU_HEIGHT        (N_MENUS * MENU_STEP_Y)
#define MAX_SUBENTRY_LEN        29                                                      // max number of characters per sub entry

#define MAIN_MENU_START_X       80                                                      // rectangle window for main menu
#define MAIN_MENU_START_Y       ((TFT_HEIGHT - MAIN_MENU_HEIGHT) / 2)
#define MAIN_MENU_END_X         (TFT_WIDTH - MAIN_MENU_START_X - 1)
#define MAIN_MENU_END_Y         (MAIN_MENU_START_Y + MAIN_MENU_HEIGHT - 1)

#define SUB_MENU_START_X        (ZXSCR_LEFT_OFFSET + ZX_SPECTRUM_LEFT_RIGHT_BORDER_SIZE)    // rectangle window for sub menu
#define SUB_MENU_START_Y        (ZXSCR_TOP_OFFSET + ZX_SPECTRUM_TOP_BOTTOM_BORDER_SIZE)
#define SUB_MENU_END_X          (SUB_MENU_START_X + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS)
#define SUB_MENU_END_Y          (SUB_MENU_START_Y + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS)
#define SUB_MENU_X_OFFSET       4                                                       // x position of first menu entry col

#define MENU_START_Y_OFFSET     4                                                       // y position of first menu entry
#define MENU_STEP_Y             16                                                      // y step for next line in menu
#define MENU_LOAD_STEP_Y        16                                                      // y step for next line in load menu

#define SUB_MENU_ENTRIES        12                                                      // max number of visible entries in list

#else // TFT_WIDTH == 800

#define MAX_SUBENTRY_LEN        59                                                      // max number of characters per sub entry

#define MAIN_MENU_START_X       600                                                     // rectangle window for menu
#define MAIN_MENU_START_Y       12
#define MAIN_MENU_END_X         (TFT_WIDTH - 1)                                         // MAIN_MENU_END_Y not needed here

#define SUB_MENU_START_X        (ZXSCR_LEFT_OFFSET + ZX_SPECTRUM_BORDER_SIZE)           // rectangle window for sub menu
#define SUB_MENU_START_Y        (ZXSCR_TOP_OFFSET  + ZX_SPECTRUM_BORDER_SIZE)
#define SUB_MENU_END_X          (SUB_MENU_START_X + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_COLUMNS)
#define SUB_MENU_END_Y          (SUB_MENU_START_Y + ZXSCR_ZOOM * ZX_SPECTRUM_DISPLAY_ROWS)
#define SUB_MENU_X_OFFSET       16                                                      // x position of first menu entry col

#define SUB_MENU_ENTRIES        22                                                      // max number of visible entries in list
#define MENU_START_Y_OFFSET     16                                                      // y position of first menu entry
#define MENU_STEP_Y             32                                                      // y step for next line in menu
#define MENU_LOAD_STEP_Y        16                                                      // y step for next line in load menu
#endif

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

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_main_menu_entry () - draw menu entry
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_main_menu_entry (uint_fast8_t menuidx, const char * str, uint_fast8_t activeidx, uint_fast8_t entry_type, uint8_t step_y)
{
    char            buf[MAX_MAIN_ENTRY_LEN + 1];
    uint_fast16_t   fcolor565;
    uint_fast16_t   bcolor565;
    uint32_t        i;

    if (activeidx == menuidx)
    {
        fcolor565 = RED565;
    }
    else if (entry_type == ENTRY_TYPE_ALERT)
    {
        fcolor565 = YELLOW565;
    }
    else if (entry_type == ENTRY_TYPE_DISABLED)
    {
        fcolor565 = GRAY565;
    }
    else
    {
        fcolor565 = WHITE565;
    }

    bcolor565 = BLACK565;

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

    draw_string ((unsigned char *) buf, MAIN_MENU_START_Y + MENU_START_Y_OFFSET + menuidx * step_y, MAIN_MENU_START_X, fcolor565, bcolor565);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_sub_menu_entry () - draw sub menu entry
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_sub_menu_entry (uint_fast8_t idx, const char * str, uint_fast8_t activeidx, uint_fast8_t entry_type, uint8_t step_y)
{
    static char     buf[MAX_SUBENTRY_LEN + 1];
    uint_fast16_t   fcolor565;
    uint_fast16_t   bcolor565;
    uint32_t        i;

    if (activeidx == idx)
    {
        fcolor565 = RED565;
    }
    else if (entry_type == ENTRY_TYPE_ALERT)
    {
        fcolor565 = YELLOW565;
    }
    else if (entry_type == ENTRY_TYPE_DISABLED)
    {
        fcolor565 = GRAY565;
    }
    else
    {
        fcolor565 = WHITE565;
    }

    bcolor565 = BLACK565;

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

    draw_string ((unsigned char *) buf, SUB_MENU_START_Y + MENU_START_Y_OFFSET + idx * step_y, SUB_MENU_START_X + SUB_MENU_X_OFFSET, fcolor565, bcolor565);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_menu_poke_entry () - draw menu poke entry
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_menu_poke_entry (FILE * fp, uint_fast8_t idx, long pos, uint_fast8_t actidx, uint_fast8_t entry_type, uint8_t step_y)
{
    static char     buf[MAX_SUBENTRY_LEN + 1];
    uint_fast16_t   fcolor565;
    uint_fast16_t   bcolor565;
    int             ch;
    uint32_t        i;

    if (actidx == idx)
    {
        fcolor565 = RED565;
    }
    else if (entry_type == ENTRY_TYPE_ALERT)
    {
        fcolor565 = YELLOW565;
    }
    else if (entry_type == ENTRY_TYPE_DISABLED)
    {
        fcolor565 = GRAY565;
    }
    else
    {
        fcolor565 = WHITE565;
    }

    bcolor565 = BLACK565;

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

    draw_string ((unsigned char *) buf, SUB_MENU_START_Y + MENU_START_Y_OFFSET + idx * step_y, SUB_MENU_START_X + SUB_MENU_X_OFFSET, fcolor565, bcolor565);
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
    return strcmp ((const char *) p1, (const char *) p2);
}

static uint_fast16_t    last_joy_scancode = SCANCODE_NONE;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_get_scancode () - get scancode from keyboard or joystick
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast16_t
menu_get_scancode (void)
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
    else
    {
        static uint8_t  shift = 0;

        if (scancode == SCANCODE_LSHFT)
        {
            shift = 1;
        }
        else if (scancode == (SCANCODE_LSHFT | PS2KEY_RELEASED_FLAG))
        {
            shift = 0;
        }
        else if (shift)
        {
            switch (scancode)
            {
                case SCANCODE_SPACE:        scancode = SCANCODE_ESC;        break;
                case SCANCODE_1:            scancode = SCANCODE_ESC;        break;
                case SCANCODE_5:            scancode = SCANCODE_L_ARROW;    break;
                case SCANCODE_6:            scancode = SCANCODE_D_ARROW;    break;
                case SCANCODE_7:            scancode = SCANCODE_U_ARROW;    break;
                case SCANCODE_8:            scancode = SCANCODE_R_ARROW;    break;
                case SCANCODE_0:            scancode = SCANCODE_BSP;        break;
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
menu_wait_until_joystick_released (void)
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
 * menu_draw_rectangle () - draw rectangle for sub menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
menu_draw_rectangle (void)
{
    tft_fill_rectangle (SUB_MENU_START_X, SUB_MENU_START_Y, SUB_MENU_END_X, SUB_MENU_END_Y, BLACK565);  // erase sub menu
    tft_draw_rectangle (SUB_MENU_START_X, SUB_MENU_START_Y, SUB_MENU_END_X, SUB_MENU_END_Y, RED565);    // draw rectangle
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_erase_rectangle () - erase rectangle for sub menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
menu_erase_rectangle (void)
{
    zxscr_force_update = 1;
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

    while (! do_break && (scancode = menu_get_scancode()) != SCANCODE_ESC)
    {
        switch (scancode)
        {
            case SCANCODE_D_ARROW:
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

        if (z80_settings.keyboard & KEYBOARD_USB)
        {
            usb_hid_host_process (TRUE);
        }
    }
    return entry_idx;
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
    int             entry_idx;
    char *          fname       = (char *) 0;

    menu_draw_rectangle ();

    n_subentries = 0;
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
                    if (n_subentries < MAX_SUB_ENTRIES)
                    {
                        strncpy (subentries.files[n_subentries], fno.fname, MENU_MAX_FILENAME_LEN);
                        subentries.files[n_subentries][MENU_MAX_FILENAME_LEN] = '\0';
                        n_subentries++;
                    }
                }
            }
        }

        f_closedir (&dir);
    }

    if (n_subentries > 1)
    {
        qsort (subentries.files, n_subentries, MENU_MAX_FILENAME_LEN + 1, filescmp);
    }

    menu_wait_until_joystick_released ();
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
    char *          fname;
    int             entry_idx   = -1;
    int             ch;
    uint_fast8_t    bol = 1;                                // flag: at beginning of line
    long            pos;

    menu_draw_rectangle ();

    n_subentries = 0;

    fname = z80_get_poke_file ();

    if (fname && *fname)
    {
        FILE * fp = fopen (fname, "r");

        if (fp)
        {
            for (pos = 0; (ch = getc (fp)) != EOF; pos++)   // ftell doesn't work correctly, so we read char by char instead of calling fgets()
            {
                if (bol)
                {
                    if (bol && ch == 'Y')
                    {
                        break;
                    }

                    if (ch == 'N')
                    {
                        if (n_subentries < MAX_SUB_ENTRIES)
                        {
                            subentries.positions[n_subentries] = pos + 1;
                            n_subentries++;
                        }
                    }
                }

                if (ch == '\n')
                {
                    bol = 1;
                }
                else
                {
                    bol = 0;
                }
            }

            menu_wait_until_joystick_released ();
            entry_idx = menu_handle_sub_menu (fp);
            fclose (fp);
        }
    }

    menu_erase_rectangle ();
    return entry_idx;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_input_fname_field () - draw input fname field
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_input_fname_field (uint_fast16_t x, uint_fast16_t y, char * fname_buf)
{
    draw_string ((unsigned char *) fname_buf, y, x, WHITE565, BLACK565);
    x += 8 * strlen (fname_buf);
    draw_string ((unsigned char *) " ", y, x, WHITE565, RED565);
    x += 8;
    draw_string ((unsigned char *) " ", y, x, WHITE565, BLACK565);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_save () - handle save to file menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static char *
menu_save (uint8_t is_snapshot)
{
    static char     fname_buf[MAX_FNAME_INPUT_LEN + 1];                                     // static, is return value!
    uint_fast8_t    do_break    = 0;
    uint_fast16_t   scancode;
    uint_fast8_t    len;
    uint_fast8_t    ch;
    const char *    p;
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

    draw_string ((unsigned char *) p, SUB_MENU_START_Y + MENU_START_Y_OFFSET, SUB_MENU_START_X + SUB_MENU_X_OFFSET, WHITE565, BLACK565);
    xoffset = SUB_MENU_START_X + SUB_MENU_X_OFFSET + 8 * (strlen (p) + 1);

    fname_buf[0] = '\0';
    len = 0;

    draw_input_fname_field (xoffset, SUB_MENU_START_Y + MENU_START_Y_OFFSET, fname_buf);
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

        if (z80_settings.keyboard & KEYBOARD_USB)
        {
            usb_hid_host_process (TRUE);
        }
    }

    menu_erase_rectangle ();
    return fname;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * draw_main_menu () - draw main menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
draw_main_menu (uint_fast8_t activeidx, uint_fast8_t poke_file_active)
{
#if TFT_WIDTH == 320
    tft_fill_rectangle (MAIN_MENU_START_X - 8, MAIN_MENU_START_Y - 8, MAIN_MENU_END_X + 8, MAIN_MENU_END_Y + 8, BLACK565);  // erase menu block
    tft_draw_rectangle (MAIN_MENU_START_X - 8, MAIN_MENU_START_Y - 8, MAIN_MENU_END_X + 8, MAIN_MENU_END_Y + 8, RED565);    // draw thin red border
#endif

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
        draw_main_menu_entry (MENU_ENTRY_SAVE, "SAVE", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    }

    draw_main_menu_entry (MENU_ENTRY_SNAPSHOT, "SNAPSHOT", activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
    draw_main_menu_entry (MENU_ENTRY_AUTOSTART, autostart_entries[z80_get_autostart()], activeidx, ENTRY_TYPE_NONE, MENU_STEP_Y);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu () - handle setup menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu (uint_fast8_t poke_file_active)
{
    uint_fast8_t            activeitem = 0;
    uint_fast16_t           scancode;
    uint_fast8_t            do_break = 0;

    draw_main_menu (activeitem, poke_file_active);
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
                        zxio_reset ();
                        do_break = 1;
                        break;
                    }
                    case MENU_ENTRY_ROM:
                    {
                        char * fname = menu_load (1);

                        if (fname)
                        {
                            // printf ("z80_load_rom (%s)\r\n", fname);
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
                                // printf ("z80_set_fname_save (%s)\r\n", fname);
                                z80_set_fname_save (fname);
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
                            // printf ("z80_set_fname_save_snapshot (%s)\r\n", fname);
                            z80_set_fname_save_snapshot (fname);
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

        if (z80_settings.keyboard & KEYBOARD_USB)
        {
            usb_hid_host_process (TRUE);
        }
    }

    draw_main_menu (0xFF, poke_file_active);
    ps2key_setmode (PS2KEY_CALLBACK_MODE);
    menu_wait_until_joystick_released ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_redraw () - redraw menu
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
menu_redraw (uint_fast8_t poke_file_active)
{
    draw_main_menu (0xFF, poke_file_active);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * menu_start_load () - directly enter menu "LOAD"
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
char *
menu_start_load (void)
{
    char *  fname;

    ps2key_setmode (PS2KEY_GET_MODE);
    z80_leave_focus ();
    fname = menu_load (0);
    z80_enter_focus ();
    ps2key_setmode (PS2KEY_CALLBACK_MODE);

    return fname;
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
    draw_main_menu (0xFF, 0);
#endif
}
