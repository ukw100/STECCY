/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxmain - main function
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#if defined FRAMEBUFFER
#include "lxkbd.h"
#include "lxfb.h"
#elif defined X11
#include "lxx11.h"
#endif

#include "z80.h"

#if defined FRAMEBUFFER
#include <pthread.h>
static pthread_t    tid[1];

static void
clear_terminal (void)
{
    printf ("%c[2J", 0x1B);
    fflush (stdout);
    usleep (100000);
}

#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * sigcatch - signal catcher
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
sigcatch (int sig)
{
    (void) sig;

#if defined FRAMEBUFFER
    lxkbd_deinit ();
    fb_deinit ();
    clear_terminal ();

#elif defined X11
    x11_deinit ();
#endif
    exit (0);
}

#if defined X11

static int
x11_main (int argc, char ** argv)
{
    char *  geometry = (char *) "800x480";

    if (argc == 3)
    {
        if (! strcmp (argv[1], "-g"))
        {
            geometry = argv[2];
            argc -= 2;
            argv += 2;
        }
    }

    if (x11_init (geometry) < 0)
    {
        return 1;
    }

    signal (SIGTERM, sigcatch);
    zx_spectrum ();

    x11_deinit ();
    return 0;
}

#elif defined FRAMEBUFFER

static int
fb_main (int argc, char ** argv)
{
    char *  geometry = (char *) 0;
    int     err;

    if (argc == 3)
    {
        if (! strcmp (argv[1], "-g"))
        {
            geometry = argv[2];
            argc -= 2;
            argv += 2;
        }
    }

    if (lxkbd_init () < 0)
    {
        return 1;
    }

    if (fb_init (geometry) < 0)
    {
        return 1;
    }

    err = pthread_create (&(tid[0]), NULL, &lxkbd_read, NULL);

    if (err == 0)
    {
        signal (SIGTERM, sigcatch);
        zx_spectrum ();
    }
    else
    {
        fprintf (stderr, "can't create thread :[%s]", strerror(err));
        return 1;
    }

    lxkbd_deinit ();
    fb_deinit ();

    clear_terminal ();
    return 0;
}

#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * main - main function
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
main (int argc, char ** argv)
{
    int     rtc;

#if defined X11
    rtc = x11_main (argc, argv);
#elif defined FRAMEBUFFER
    rtc = fb_main (argc, argv);
#endif
    return rtc;
}
