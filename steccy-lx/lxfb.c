/*-------------------------------------------------------------------------------------------------------------------------------------------
 * lxfb.c - linux framebuffer
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "lxfb.h"
#include "lxdisplay.h"

static int                          fb_fd;
static uint_fast8_t                 fb_resolution_changed = 0;
static struct fb_var_screeninfo     fb_vinfo_old;
static struct fb_var_screeninfo     fb_vinfo;
static struct fb_fix_screeninfo     fb_finfo;
static uint32_t *                   fbp;
static uint32_t                     fb_line_length;
static long                         fb_screensize;

#ifndef PAGE_SHIFT
    #define PAGE_SHIFT 12
#endif
#ifndef PAGE_SIZE
    #define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif
#ifndef PAGE_MASK
    #define PAGE_MASK (~(PAGE_SIZE - 1))
#endif

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * fill_rectangle - fill rectangle
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
fill_rectangle (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
    int y;
    int x;

    for (y = y1; y <= y2; y ++)
    {
        for (x = x1; x <= x2; x++)
        {
            fbp[y * fb_line_length + x] = color;
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * fb_deinit - deinit framebuffer routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
fb_deinit (void)
{
    munmap(fbp, fb_screensize);

    if (fb_resolution_changed && ioctl(fb_fd, FBIOPUT_VSCREENINFO, &fb_vinfo_old) == -1)
    {
        perror("Error reading variable information");
    }

    close(fb_fd);
    printf ("\x1B[?25h");                                                                       // enable text cursor
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * fb_init - init framebuffer routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int
fb_init (char * geometry)
{
    uint32_t width  = 0;
    uint32_t height = 0;

    fb_fd = open("/dev/fb0", O_RDWR);

    if (geometry)
    {
        width = atoi (geometry);

        while (*geometry && *geometry != 'x')
        {
            geometry++;
        }

        if (*geometry == 'x')
        {
            geometry++;
            height = atoi (geometry);
        }
    }

    if (fb_fd == -1)
    {
        perror("Error: cannot open framebuffer device /dev/fb0");
        return -1;
    }

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fb_finfo) == -1)
    {
        perror("Error reading fixed information");
        return -1;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &fb_vinfo) == -1)
    {
        perror("Error reading variable information");
        return -1;
    }

    printf ("%dx%d, %dbpp\n", fb_vinfo.xres, fb_vinfo.yres, fb_vinfo.bits_per_pixel);

    if (fb_vinfo.bits_per_pixel != 32)
    {
        fprintf (stderr, "%dbpp not supported, only 32bpp\n", fb_vinfo.bits_per_pixel);
        return -1;
    }


    if (width > 0 && height > 0)
    {
        if (fb_vinfo.xres != width || fb_vinfo.yres != height)
        {
            if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &fb_vinfo_old) == -1)
            {
                perror("Error reading variable information");
                return -1;
            }

            fb_vinfo.xres   = width;
            fb_vinfo.yres   = height;

            if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &fb_vinfo) == -1)
            {
                perror("Error reading variable information");
                return -1;
            }

            fb_resolution_changed = 1;
        }
    }

    lxdisplay_init (fb_vinfo.xres, fb_vinfo.yres);

    int fb_mem_offset = (unsigned long)(fb_finfo.smem_start) & (~PAGE_MASK);
    printf ("mem_offset=%d smem_len=%d\n", fb_mem_offset, fb_finfo.smem_len + fb_mem_offset);

    fb_line_length  = (2 * fb_finfo.line_length) / 8 ;

    printf("fb_vinfo.xoffset = %d\n", fb_vinfo.xoffset);
    printf("fb_vinfo.yoffset = %d\n", fb_vinfo.yoffset);
    printf("fb_finfo.line_length = %d\n", fb_finfo.line_length);
    printf ("line_length=%d\n", fb_line_length);

    fbp = mmap(0, fb_finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);

    if ((long) fbp == -1L)
    {
        perror("Error: failed to map framebuffer device to memory");
        return -1;
    }

    printf ("\x1B[?25l");                                                                   // disable text cursor
    fflush (stdout);

    fill_rectangle (0, 0, fb_vinfo.xres - 1, fb_vinfo.yres - 1, 0);
    return 0;
}
