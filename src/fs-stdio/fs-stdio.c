/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * fs-stdio.c - file system stdio function implementation
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2018-2021 Frank Meyer - frank(at)uclock.de
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
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "console.h"
#include "fs-stdio.h"

#define FS_BUFSIZE              512
#define FS_MAX_OPEN_FILES       8
#define FS_FDNO_FLAG_IS_OPEN    0x01

typedef struct
{
    FIL         fil;
    uint32_t    flags;
} FS_FDNO_SLOT;

static int              fs_errno = FR_OK;
static FS_FDNO_SLOT     fs_fdno[FS_MAX_OPEN_FILES];

int                     fs_stdout_fd = -1;
int                     fs_stderr_fd = -1;

void
fs_perror (const char * path, FRESULT res)
{
    if (path && *path)
    {
        fprintf (stderr, "%s: ", path);
    }

    switch (res)
    {
        case FR_OK:                     fputs ("succeeded\n", stderr); break;
        case FR_DISK_ERR:               fputs ("a hard error occurred in the low level disk I/O layer\n", stderr); break;
        case FR_INT_ERR:                fputs ("assertion failed\n", stderr); break;
        case FR_NOT_READY:              fputs ("physical drive cannot work\n", stderr); break;
        case FR_NO_FILE:                fputs ("no such file\n", stderr); break;
        case FR_NO_PATH:                fputs ("no such file or directory\n", stderr); break;
        case FR_INVALID_NAME:           fputs ("path name format is invalid\n", stderr); break;
        case FR_DENIED:                 fputs ("access denied due to prohibited access or directory full\n", stderr); break;
        case FR_EXIST:                  fputs ("access denied due to prohibited access\n", stderr); break;
        case FR_INVALID_OBJECT:         fputs ("file/directory object is invalid\n", stderr); break;
        case FR_WRITE_PROTECTED:        fputs ("physical drive is write protected\n", stderr); break;
        case FR_INVALID_DRIVE:          fputs ("logical drive number is invalid\n", stderr); break;
        case FR_NOT_ENABLED:            fputs ("volume has no work area\n", stderr); break;
        case FR_NO_FILESYSTEM:          fputs ("there is no valid FAT volume\n", stderr); break;
        case FR_MKFS_ABORTED:           fputs ("f_mkfs() aborted due to any problem\n", stderr); break;
        case FR_TIMEOUT:                fputs ("could not get a grant to access the volume within defined period\n", stderr); break;
        case FR_LOCKED:                 fputs ("operation is rejected according to the file sharing policy\n", stderr); break;
        case FR_NOT_ENOUGH_CORE:        fputs ("lFN working buffer could not be allocated\n", stderr); break;
        case FR_TOO_MANY_OPEN_FILES:    fputs ("number of open files > FF_FS_LOCK\n", stderr); break;
        case FR_INVALID_PARAMETER:      fputs ("given parameter is invalid\n", stderr); break;
        default:                        fputs ("unknown error\n", stderr); break;
    }
}

static void
fs_set_errno (int res)
{
    fs_errno = res;
    errno = __ELASTERROR;                                                       // range beginning with user defined errors
}

int
_open (char * path, int flags, ...)
{
    FRESULT res;
    BYTE    mode;
    int     fd;

    // printf ("open: path=%s, flags=0x%02x\n", path, flags);

    switch (flags & (O_RDONLY | O_WRONLY | O_RDWR))
    {
        case O_RDONLY:
        {
            mode = FA_READ;
            break;
        }
        case O_WRONLY:
        {
            mode = FA_WRITE;

            if (flags & O_CREAT)
            {
                if (flags & O_TRUNC)
                {
                    mode = FA_WRITE | FA_CREATE_ALWAYS;
                }
                else
                {
                    mode = FA_WRITE | FA_OPEN_ALWAYS;
                }
            }
            else if (flags & O_TRUNC)
            {
                mode = FA_WRITE | FA_CREATE_ALWAYS;                                         // that's not correct
            }
            if (flags & O_APPEND)
            {
                mode = FA_WRITE | FA_OPEN_APPEND;
            }
            break;
        }
        case O_RDWR:
        {
            mode = FA_READ | FA_WRITE;

            if (flags & O_CREAT)
            {
                if (flags & O_TRUNC)
                {
                    mode = FA_READ | FA_WRITE | FA_CREATE_ALWAYS;
                }
                else
                {
                    mode = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
                }
            }
            else if (flags & O_TRUNC)
            {
                mode = FA_READ | FA_WRITE | FA_CREATE_ALWAYS;                                       // that's not correct
            }
            if (flags & O_APPEND)
            {
                mode = FA_READ | FA_WRITE | FA_OPEN_APPEND;
            }
            break;
        }
        default:
        {
            return -1;
        }
    }

    for (fd = 0; fd < FS_MAX_OPEN_FILES; fd++)
    {
        if (! (fs_fdno[fd].flags & FS_FDNO_FLAG_IS_OPEN))
        {
            break;
        }
    }

    if (fd < FS_MAX_OPEN_FILES)
    {
        // printf ("f_open: mode=0x%02x\n", mode);

        res = f_open (&(fs_fdno[fd].fil), path, mode);

        if (res == FR_OK)
        {
            fs_fdno[fd].flags |= FS_FDNO_FLAG_IS_OPEN;
            // printf ("open: rtc=%d\n", fd + 3);
            return fd + 3;
        }

        fs_set_errno (res);
        return -1;
    }

    // fputs ("too many open files\n", stdout);
    errno = ENFILE;
    return -1;
}

int
_close (int fd)
{
    int rtc = -1;

    // printf ("close: fd=%d\n", fd);

    if (fd >= 3)
    {
        fd -= 3;

        if (fd < FS_MAX_OPEN_FILES)
        {
            if ((fs_fdno[fd].flags & FS_FDNO_FLAG_IS_OPEN))
            {
                f_close (&(fs_fdno[fd].fil));
                fs_fdno[fd].flags &= ~FS_FDNO_FLAG_IS_OPEN;
                // fputs ("closed\n", stdout);
                rtc = 0;
            }
            else
            {
                errno = EBADF;
            }
        }
        else
        {
            errno = EBADF;
        }
    }
    else
    {
        errno = EBADF;
    }

    // printf ("close: rtc=%d\n", rtc);
    return rtc;
}

int
_read (int fd, char * ptr, int len)
{
    int         rtc = -1;

    // printf ("read: fd=%d, len=%d\n", fd, len);

    if (fd >= 3)
    {
        fd -= 3;

        if (fd < FS_MAX_OPEN_FILES && (fs_fdno[fd].flags & FS_FDNO_FLAG_IS_OPEN))
        {
            FIL *       fp = &(fs_fdno[fd].fil);
            FRESULT     res;
            UINT        br;

            rtc = 0;

            while (len > FS_BUFSIZE)
            {
                res = f_read (fp, ptr, FS_BUFSIZE, &br);
                // printf ("read1: res=%d len=%d br=%d\n", res, len, br);

                if (res != FR_OK)
                {
                    fs_set_errno (res);
                    return -1;
                }

                len -= br;
                ptr += br;
                rtc += br;

                if (br < FS_BUFSIZE)                                        // EOF
                {                                                           // stop reading
                    len = 0;
                }
            }

            if (len > 0)
            {
                res = f_read (fp, ptr, len, &br);
                // printf ("read2: res=%d len=%d br=%d\n", res, len, br);

                if (res != FR_OK)
                {
                    fs_set_errno (res);
                    return -1;
                }

                len -= br;
                ptr += br;
                rtc += br;
            }
        }
        else
        {
            errno = EBADF;
        }
    }

    // printf ("read: rtc=%d\n", rtc);
    return rtc;
}

int
_write (int fd, char * ptr, int len)
{
    int         rtc = -1;

    if (fd == STDOUT_FILENO && fs_stdout_fd >= 0)                                           // redirection of stdout
    {
        fd = fs_stdout_fd;
    }

    if (fd == STDERR_FILENO && fs_stderr_fd >= 0)                                           // redirection of stderr
    {
        fd = fs_stderr_fd;
    }

    // printf ("write: fd=%d, len=%d\n", fd, len);

    if (fd == STDIN_FILENO)
    {
        return rtc;
    }
    else if (fd == STDOUT_FILENO)
    {
        static int  last_ch;
        int idx;

        for (idx = 0; idx < len; idx++)
        {
            if (*ptr == '\n' && last_ch != '\r')
            {
                console_putc ('\r');
            }

            console_putc (*ptr);
            last_ch = *ptr;
            ptr++;
        }

        rtc = len;
    }
    else if (fd == STDERR_FILENO)
    {
        static int  last_ch;
        int idx;

        for (idx = 0; idx < len; idx++)
        {
            if (*ptr == '\n' && last_ch != '\r')
            {
                console_putc ('\r');
            }

            console_putc (*ptr);
            last_ch = *ptr;
            ptr++;
        }

        rtc = len;
    }
    else
    {
        fd -= 3;

        if (fd < FS_MAX_OPEN_FILES && (fs_fdno[fd].flags & FS_FDNO_FLAG_IS_OPEN))
        {
            FIL *       fp = &(fs_fdno[fd].fil);
            FRESULT     res;
            UINT        bw;

            rtc = 0;

            while (len > FS_BUFSIZE)
            {
                res = f_write (fp, ptr, FS_BUFSIZE, &bw);
                // printf ("write1: res=%d bw=%d\n", res, bw);

                if (res != FR_OK)
                {
                    fs_set_errno (res);
                    return -1;
                }

                len -= bw;
                ptr += bw;
                rtc += bw;

                if (bw < FS_BUFSIZE)                                        // EOF or FS full
                {                                                           // stop writing
                    len = 0;
                }
            }

            if (len > 0)
            {
                res = f_write (fp, ptr, len, &bw);
                // printf ("write2: res=%d bw=%d\n", res, bw);

                if (res != FR_OK)
                {
                    fs_set_errno (res);
                    return -1;
                }

                len -= bw;
                ptr += bw;
                rtc += bw;
            }
        }
        else
        {
            errno = EBADF;
        }
    }

    // printf ("write: rtc=%d\n", rtc);
    return rtc;
}

int
_lseek(int fd, int ptr, int whence)
{
    int     rtc;

    // printf ("lseek: fd=%d, ptr=%d whence=%d\n", fd, ptr, whence);

    if (fd >= 3)
    {
        fd -= 3;

        if (fd < FS_MAX_OPEN_FILES && (fs_fdno[fd].flags & FS_FDNO_FLAG_IS_OPEN))
        {
            int     res = FR_OK;
            FIL *   fp  = &(fs_fdno[fd].fil);

            switch (whence)
            {
                case SEEK_SET:  res = f_lseek(fp, ptr);                rtc = 0;    break;
                case SEEK_CUR:  res = f_lseek(fp, f_tell(fp) + ptr);   rtc = 0;    break;
                case SEEK_END:  res = f_lseek(fp, f_size(fp) + ptr);   rtc = 0;    break;
            }

            if (res != FR_OK)
            {
                fs_set_errno (res);
            }
        }
        else
        {
            errno = EBADF;
        }
    }
    else
    {
        errno = EBADF;
    }

    // printf ("lseek: rtc=%d\n", rtc);
    return rtc;
}

int _isatty(int fd)
{
    if (fd == STDOUT_FILENO && fs_stdout_fd >= 0)                                           // redirection of stdout
    {
        fd = fs_stdout_fd;
    }

    if (fd == STDERR_FILENO && fs_stderr_fd >= 0)                                           // redirection of stderr
    {
        fd = fs_stderr_fd;
    }

    if ((fd == STDOUT_FILENO) || (fd == STDIN_FILENO) || (fd == STDERR_FILENO))
    {
        return 1;
    }

    return 0;
}

void
fs_close_all_open_files (void)
{
    int fd;

    for (fd = 0; fd < FS_MAX_OPEN_FILES; fd++)
    {
        if ((fs_fdno[fd].flags & FS_FDNO_FLAG_IS_OPEN))
        {
            fprintf (stderr, "error: fd %d not closed\n", fd + 3);
            _close (fd + 3);
        }
    }
}
