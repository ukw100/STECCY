/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxkbd.c - PC keyboard
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2020 Frank Meyer - frank(at)fli4l.de
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
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/ioctl.h>

#include "z80.h"
#include "lxkbd.h"
#include "lxmapkey.h"
#include "scancodes.h"

#undef DEBUG                                                                                    // change here!

static struct termios           old;
static int                      fd;
static int                      oldkbmode;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * is_console - return true if console
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
is_console (int fd)
{
    char arg;
    arg = 0;

    return (ioctl(fd, KDGKBTYPE, &arg) == 0 && ((arg == KB_101) || (arg == KB_84)));
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * open_console - open console
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
open_console (const char * fnam)
{
    int fd;

    fd = open (fnam, O_RDWR);

    if (fd < 0 && errno == EACCES)
    {
        fd = open (fnam, O_WRONLY);
    }

    if (fd < 0 && errno == EACCES)
    {
        fd = open (fnam, O_RDONLY);
    }

    if (fd < 0)
    {
        return -1;
    }

    if (! is_console(fd))
    {
        close (fd);
        return -1;
    }

    return fd;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * getfd - get fd of console
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static int
getfd (void)
{
    int fd;

    fd = open_console ("/dev/tty");

    if (fd >= 0)
    {
        return fd;
    }

    fd = open_console ("/dev/tty0");

    if (fd >= 0)
    {
        return fd;
    }

    fd = open_console ("/dev/vc/0");

    if (fd >= 0)
    {
        return fd;
    }

    fd = open_console ("/dev/console");

    if (fd >= 0)
    {
        return fd;
    }

    fd = 0;

    if (is_console (fd))
    {
        return fd;
    }

    fprintf(stderr, "Couldnt get a file descriptor referring to the console\n");
    return -1;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxkbd_read - read keyboard in a separate thread
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void *
lxkbd_read (void *arg)
{
    unsigned char   buf[1] = { 0 };
    int             n;
    uint32_t        scancode;
    uint32_t        extended = 0;

    (void) arg;

    do
    {
        n = read (fd, buf, 1);

        if (n == 1)
        {
            scancode = buf[0];

            if (scancode == SCANCODE_EXTENDED)
            {
                extended = 1;
            }
            else if (scancode & 0x80)                                                   // key released
            {
                scancode &= 0x007F;                                                     // reset release bit

                if (extended)
                {
                    scancode |= SCANCODE_EXTENDED_FLAG;
                    extended = 0;
                }

#ifdef DEBUG
                printf("\x1B[37;1HRelease 0x%04x\r", scancode);
                fflush (stdout);
#endif

                if (! lxmapkey_menu_enabled)
                {
                    lxkeyrelease (scancode);
                }
            }
            else
            {
                if (extended)
                {
                    scancode |= SCANCODE_EXTENDED_FLAG;
                    extended = 0;
                }

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

#ifdef DEBUG
                printf("\x1B[37;1HPress   0x%04x\r", scancode);
                fflush (stdout);
#endif

            }
        }
        else
        {
            buf[0] = 0;
        }
    } while (lxmapkey_menu_enabled || buf[0] != SCANCODE_F12);       // F12: exit

    return NULL;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxkbd_deinit - deinit keyboard routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
lxkbd_deinit (void)
{
    if (ioctl (fd, KDSKBMODE, oldkbmode))
    {
        perror("KDSKBMODE");
    }

    if (tcsetattr(fd, 0, &old) == -1)
    {
        perror("tcsetattr");
    }

    close (fd);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxkbd_deinit - init keyboard routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
lxkbd_init (void)
{
    struct termios  new;

    fd = getfd ();

    if (ioctl (fd, KDGKBMODE, &oldkbmode))
    {
        perror("KDGKBMODE");
        return -1;
    }

    if (tcgetattr(fd, &old) == -1)
    {
        perror("tcgetattr");
        return -1;
    }

    if (tcgetattr(fd, &new) == -1)
    {
        perror("tcgetattr");
        return -1;
    }

    new.c_lflag &= ~ (ICANON | ECHO | ISIG);
    new.c_iflag = 0;
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSAFLUSH, &new) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    if (ioctl(fd, KDSKBMODE, K_RAW))
    {
        perror("KDSKBMODE");
        return -1;
    }

    return 0;
}
