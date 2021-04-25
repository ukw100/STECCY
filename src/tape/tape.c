/*-------------------------------------------------------------------------------------------------------------------------------------------
 * tape.c - tape functions
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2019-2021 Frank Meyer - frank(at)fli4l.de
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
#include <stdint.h>
#include <string.h>
#include "z80.h"
#include "zxram.h"
#include "zxscr.h"
#include "tape.h"

#ifdef DEBUG
#ifdef unix
static uint8_t debug = 1;
#define debug_printf(...) do { if (debug) { printf(__VA_ARGS__); fflush (stdout); }} while (0)
#define debug_putchar(c)  do { if (debug) { putchar(c); }} while (0)
#else
static uint8_t debug = 0;
#define debug_printf(...) do { if (debug /* && iff1 */) { printf(__VA_ARGS__); fflush (stdout); }} while (0)
#define debug_putchar(c)  do { if (debug /* && iff1 */) { putchar(c); fflush (stdout); }} while (0)
#endif
#else
#define debug_printf(...)
#define debug_putchar(c)
#endif

static FILE *               tape_load_fp;                                   // current fp of (load) tape file
static FILE *               tape_save_fp;                                   // current fp of (save) tape file

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_get_byte() - get a byte from TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_get_byte (FILE * fp, uint8_t * chp)
{
    int     ch;
    uint8_t rtc;

    ch = getc (fp);

    if (ch != EOF)
    {
        *chp = UINT8_T (ch);
        rtc = 1;
    }
    else
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_get_byte: unexpected EOF\n");
        fclose (fp);
#endif
        rtc = 0;
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_get_word() - get a word from TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_get_word (FILE * fp, uint16_t * wp)
{
    int         lo;
    int         hi;

    lo = getc (fp);

    if (lo == EOF)
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_get_word: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    hi = getc (fp);

    if (hi == EOF)
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_get_word: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    *wp = UINT16_T ((UINT16_T (hi) << 8) | (UINT16_T (lo)));

    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_get_triple() - get 3 bytes from TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_get_triple (FILE * fp, uint32_t * dp)
{
    int         lo;
    int         mi;
    int         hi;

    lo = getc (fp);

    if (lo == EOF)
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_get_word: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    mi = getc (fp);

    if (mi == EOF)
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_get_word: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    hi = getc (fp);

    if (hi == EOF)
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_get_word: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    *dp = UINT32_T ((UINT32_T (hi) << 16) | (UINT32_T (mi) << 8) | (UINT32_T (lo)));

    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_header() - read TZX header
 *------------------------------------------------------------------------------------------------------------------------
 */
static int
tzx_read_header (FILE * fp)
{
    uint8_t     tzx_buf[10];
    int         i;

    for (i = 0x00; i <= 0x09; i++)
    {
        if (! tzx_get_byte (fp, tzx_buf + i))
        {
#ifdef DEBUG
            fprintf (stderr, "tzx_read_header: unexpected EOF\n");
            fflush (stderr);
#endif
            return 0;
        }
    }

    if (memcmp (tzx_buf, "ZXTape!", 7))
    {
        fprintf (stderr, "invalid TZX signature\n");
        return 0;
    }

    if (tzx_buf[0x07] != 0x1A)
    {
        fprintf (stderr, "wrong End of text file marker: 0x%02X\n", tzx_buf[0x07]);
        return 0;
    }

    debug_printf ("version: %d.%d\n", tzx_buf[0x08], tzx_buf[0x09]);

    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_write_header() - write TZX header
 *------------------------------------------------------------------------------------------------------------------------
 */
static int
tzx_write_header (FILE * fp)
{
    fputs ("ZXTape!", fp);                                                      // Magic header
    fputc (0x1A, fp);                                                           // End of text marker
    fputc (0x01, fp);                                                           // Version 1.10
    fputc (0x10, fp);
    return 1;
}

#ifdef DEBUG
static void
/*------------------------------------------------------------------------------------------------------------------------
 * tzx_print_header_info() - print header info of TZX block
 *------------------------------------------------------------------------------------------------------------------------
 */
tzx_print_header_info (uint8_t * addr)
{
    uint16_t    idx;
    uint16_t    autostartline;
    uint16_t    data_len;
    uint16_t    program_len;

    debug_printf ("Block type: Program Header\n");
    debug_printf ("Name: ");

    for (idx = 0; idx < 10; idx++)
    {
        debug_putchar (*addr);
        addr++;
    }
    debug_putchar ('\n');

    data_len = UINT16_T (*addr | (*(addr + 1) << 8));
    debug_printf ("data len: %d\n", data_len);
    addr += 2;

    autostartline = UINT16_T (*addr | (*(addr + 1) << 8));

    if (autostartline != 32768)
    {
        debug_printf ("autostart line: %d\n", autostartline);
    }
    else
    {
        debug_printf ("autostart line: [none]\n");
    }
    addr += 2;

    program_len = UINT16_T (*addr | (*(addr + 1) << 8));
    debug_printf ("program len: %d\n", program_len);
    debug_printf ("offset of variables: %d\n", data_len - program_len);
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_print_byte_header_info() - print byte header info of TZX block
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
tzx_print_byte_header_info (uint8_t * addr)
{
    uint16_t    idx;
    uint16_t    data_len;
    uint16_t    start_addr;
    uint16_t    magic;

    debug_printf ("Block type: Byte Header\n");
    debug_printf ("Name: ");

    for (idx = 0; idx < 10; idx++)
    {
        debug_putchar (*addr);
        addr++;
    }
    debug_putchar ('\n');

    data_len = UINT16_T (*addr | (*(addr + 1) << 8));
    debug_printf ("data len: %d\n", data_len);
    addr += 2;
    start_addr = UINT16_T (*addr | (*(addr + 1) << 8));
    debug_printf ("start address: %d\n", start_addr);
    addr += 2;
    magic = UINT16_T (*addr | (*(addr + 1) << 8));
    debug_printf ("magic: %d (should be 32768)\n", magic);
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_print_numeric_data_array_header_info() - print numeric data array header info of TZX block
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
tzx_print_numeric_data_array_header_info (uint8_t * addr)
{
    uint16_t    idx;
    uint16_t    data_len;
    uint16_t    magic;

    debug_printf ("Block type: Numeric Data Array Header\n");
    debug_printf ("Name: ");

    for (idx = 0; idx < 10; idx++)
    {
        debug_putchar (*addr);
        addr++;
    }
    debug_putchar ('\n');

    data_len = UINT16_T (*addr | (*(addr + 1) << 8));
    debug_printf ("data len: %d\n", data_len);
    addr += 2;
    debug_printf ("dummy byte: %d\n", *addr);
    addr += 1;
    debug_printf ("variable name: %c\n", *addr - 128 + 'A');
    addr += 1;
    magic = UINT16_T (*addr | (*(addr + 1) << 8));
    debug_printf ("magic: %d (should be 32768)\n", magic);
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_print_alphanumeric_data_header_info() - print alphanumeric data array header info of TZX block
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
tzx_print_alphanumeric_data_header_info (uint8_t * addr)
{
    uint16_t    idx;
    uint16_t    data_len;
    uint16_t    magic;

    debug_printf ("Block type: Alphanumeric Data Array Header\n");
    debug_printf ("Name: ");

    for (idx = 0; idx < 10; idx++)
    {
        debug_putchar (*addr);
        addr++;
    }
    debug_putchar ('\n');

    data_len = UINT16_T (*addr | (*(addr + 1) << 8));
    addr += 2;
    debug_printf ("data len: %d\n", data_len);

    debug_printf ("dummy byte: %d\n", *addr);
    addr += 1;

    debug_printf ("variable name: %c$\n", *addr - 192 + 'A');
    addr += 1;

    magic = UINT16_T (*addr | (*(addr + 1) << 8));
    debug_printf ("magic: %d (should be 32768)\n", magic);
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_print_data_info() - print data info of TZX block
 *------------------------------------------------------------------------------------------------------------------------
 */
static void
tzx_print_data_info (void)
{
    debug_printf ("Block type: Data\n");
}
#endif // DEBUG

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_tap() - read TAP block of TZX/TAP file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tap_read_block_data (FILE * fp, uint16_t base_addr, uint16_t maxlen, uint8_t load, uint8_t load_data, uint32_t len)
{
    uint8_t         flag;
    uint8_t         dummy;
    uint16_t        addr;
    uint16_t        idx;
    uint8_t         chksum;
    uint8_t         rtc = 1;

    debug_printf ("load = %d, len = %lu, max_len = %u\n", load, len, maxlen);

    if (! tzx_get_byte (fp, &flag))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_tap: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    chksum = flag;
    len -= 2;

    if (flag == 0)                                                  // read header
    {
        addr = base_addr;

        if (load_data)                                              // we want data but we have read a header
        {
            for (idx = 0; idx < len; idx++)                         // skip stuff
            {
                if (! tzx_get_byte (fp, &dummy))
                {
#ifdef DEBUG
                    fprintf (stderr, "tzx_read_block_tap: unexpected EOF\n");
                    fflush (stderr);
#endif
                    return 0;
                }
                chksum ^= dummy;
            }

            rtc = 0;
        }
        else
        {
            uint8_t data;

            addr = base_addr;

            for (idx = 0; idx < len; idx++)
            {
                if (! tzx_get_byte (fp, &data))
                {
#ifdef DEBUG
                    fprintf (stderr, "tzx_read_block_tap: unexpected EOF\n");
                    fflush (stderr);
#endif
                    return 0;
                }

                if (idx < maxlen)
                {
                    if (load)                                                   // load - not verify
                    {
                        if (addr >= ZX_RAM_BEGIN)
                        {
                            zx_ram_set_8(addr, data);
                        }
                    }
                    else
                    {
                        // TODO verify
                    }
                }

                chksum ^= data;
                addr++;
            }

#ifdef DEBUG
            if (ram[base_addr] == 0x00)                                         // program header
            {
                tzx_print_header_info (ram + base_addr + 1);
            }
            else if (ram[base_addr] == 0x01)                                    // data array header
            {
                tzx_print_numeric_data_array_header_info (ram + base_addr + 1);
            }
            else if (ram[base_addr] == 0x02)                                    // string array header
            {
                tzx_print_alphanumeric_data_header_info (ram + base_addr + 1);
            }
            else if (ram[base_addr] == 0x03)                                    // byte heser
            {
                tzx_print_byte_header_info (ram + base_addr + 1);
            }
#endif
            if (zx_ram_get_8(base_addr) == 0x00)                                // program header
            {
                if (! z80_get_autostart ())                                     // disable autostart
                {
                    zx_ram_set_8(base_addr + 13, 0x00);
                    zx_ram_set_8(base_addr + 14, 0x80);
                }
            }
        }

        if (! tzx_get_byte (fp, &dummy))                                        // checksum
        {
#ifdef DEBUG
            fprintf (stderr, "tzx_read_block_tap: unexpected EOF\n");
            fflush (stderr);
#endif
            return 0;
        }

        if (chksum != dummy)
        {
            fprintf (stderr, "tzx_read_block_tap: checksum error\n");
            fflush (stderr);
            return 0;
        }
    }
    else // if (flag == 0xFF)                                                      // read data
    {
        addr = base_addr;

        if (load_data)
        {
            uint8_t     data;

            for (idx = 0; idx < len; idx++)
            {
                if (! tzx_get_byte (fp, &data))
                {
#ifdef DEBUG
                    fprintf (stderr, "tzx_read_block_tap: unexpected EOF\n");
                    fflush (stderr);
#endif
                    return 0;
                }

                if (idx < maxlen)
                {
                    if (load)                                                   // load - not verify
                    {
                        if (addr >= ZX_RAM_BEGIN)
                        {
                            zx_ram_set_8(addr, data);
                        }
                    }
                    else
                    {
                        // TODO verify
                    }
                }

                chksum ^= data;
                addr++;
            }
#ifdef DEBUG
            tzx_print_data_info ();
#endif
        }
        else                                                        // we want a header but we have found a data block
        {
            for (idx = 0; idx < len; idx++)                         // skip stuff
            {
                if (! tzx_get_byte (fp, &dummy))
                {
#ifdef DEBUG
                    fprintf (stderr, "tzx_read_block_tap: unexpected EOF\n");
                    fflush (stderr);
#endif
                    return 0;
                }
                chksum ^= dummy;
            }

            rtc = 0;
        }

        if (! tzx_get_byte (fp, &dummy))                             // checksum
        {
#ifdef DEBUG
            fprintf (stderr, "tzx_read_block_tap: unexpected EOF\n");
            fflush (stderr);
#endif
            return 0;
        }

        if (chksum != dummy)
        {
            fprintf (stderr, "tzx_read_block_tap: checksum error\n");
            fflush (stderr);
            return 0;
        }
    }
    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_10() - read block 10 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tap_read_block (FILE * fp, uint16_t base_addr, uint16_t maxlen, uint8_t load, uint8_t load_data)
{
    uint16_t        len;
    uint8_t         rtc;

    if (! tzx_get_word (fp, &len))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_10: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    rtc = tap_read_block_data (fp, base_addr, maxlen, load, load_data, UINT32_T (len));

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_10() - read block 10 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_10 (FILE * fp, uint16_t base_addr, uint16_t maxlen, uint8_t load, uint8_t load_data)
{
    uint16_t        len;
    uint16_t        pause;
    uint8_t         rtc;

    if (! tzx_get_word (fp, &pause))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_10: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }
    debug_printf ("pause = %d\n", pause);

    if (! tzx_get_word (fp, &len))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_10: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    rtc = tap_read_block_data (fp, base_addr, maxlen, load, load_data, UINT32_T (len));

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_11() - read block 11 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_11 (FILE * fp, uint16_t base_addr, uint16_t maxlen, uint8_t load, uint8_t load_data)
{
    uint32_t    len;
    uint16_t    dummyw;
    uint8_t     dummyb;
    uint8_t     rtc;

    if (! tzx_get_word (fp, &dummyw) ||
        ! tzx_get_word (fp, &dummyw) ||
        ! tzx_get_word (fp, &dummyw) ||
        ! tzx_get_word (fp, &dummyw) ||
        ! tzx_get_word (fp, &dummyw) ||
        ! tzx_get_word (fp, &dummyw) ||
        ! tzx_get_byte (fp, &dummyb) ||
        ! tzx_get_word (fp, &dummyw))
    {
        return 0;
    }

    if (! tzx_get_triple (fp, &len))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_11: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    rtc = tap_read_block_data (fp, base_addr, maxlen, load, load_data, len);

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_12() - read block 12 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_12 (FILE * fp)
{
    uint16_t    dummy;

    if (! tzx_get_word (fp, &dummy))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_12: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    if (! tzx_get_word (fp, &dummy))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_12: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    return 0;                                                   // return failure to force read of next block
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_14() - read block 14 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_14 (FILE * fp)
{
    uint16_t        pause0;
    uint16_t        pause1;
    uint8_t         lenbyte;
    uint32_t        len;
    uint8_t         used_bits;
    uint16_t        pause;
    uint16_t        idx;
    uint8_t         dummy;

    if (! tzx_get_word (fp, &pause0))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_14: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    debug_printf ("pause 0-bit = %d\n", pause0);

    if (! tzx_get_word (fp, &pause1))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_14: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    debug_printf ("pause 1-bit = %d\n", pause1);

    if (! tzx_get_byte (fp, &used_bits))
    {
        return 0;
    }

    debug_printf ("used bits of last byte = %d\n", used_bits);

    if (! tzx_get_word (fp, &pause))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_read_block_14: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    debug_printf ("pause after this block = %d\n", pause);

    if (! tzx_get_byte (fp, &lenbyte))
    {
        return 0;
    }

    len = lenbyte;

    if (! tzx_get_byte (fp, &lenbyte))
    {
        fflush (stdout);
        return 0;
    }

    len |= UINT32_T (lenbyte << 8);

    if (! tzx_get_byte (fp, &lenbyte))
    {
        return 0;
    }

    len |= UINT32_T (lenbyte << 16);

    debug_printf ("len = %d\n", len);

    for (idx = 0; idx < len; idx++)
    {
        if (! tzx_get_byte (fp, &dummy))
        {
            break;
        }
    }

    fflush (stdout);
    return 0;                                                   // return failure to force read of next block
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_20() - read block 20 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_20 (FILE * fp)
{
    uint16_t        pause;

    if (! tzx_get_word (fp, &pause))
    {
        return 0;
    }
    printf ("pause = %d\n", pause);
    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_21() - read block 21 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_21 (FILE * fp)
{
    uint8_t         slen;
    uint8_t         ch;
    uint8_t         idx;

    if (! tzx_get_byte (fp, &slen))
    {
        return 0;
    }

    printf ("String length = %d\n", slen);

    for (idx = 0; idx < slen; idx++)
    {
        if (! tzx_get_byte (fp, &ch))
        {
            return 0;
        }
        putchar (ch);
    }
    putchar ('\n');
    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_22() - read block 22 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_22 (void)
{
    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_30() - read block 30 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_30 (FILE * fp)
{
    uint8_t        len;
    uint8_t         ch;

    debug_printf ("Text: ");

    if (! tzx_get_byte (fp, &len))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_block_30: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    while (len--)
    {
        if (! tzx_get_byte (fp, &ch))
        {
#ifdef DEBUG
            fprintf (stderr, "tzx_block_30: unexpected EOF\n");
            fflush (stderr);
#endif
            return 0;
        }
        debug_putchar (ch);
    }
    debug_putchar ('\n');

    return 0;                                                   // return failure to force read of next block
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_read_block_32() - read block 32 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_read_block_32 (FILE * fp)
{
    uint16_t    archlength;
    uint8_t     NoOfTextStrings;

    debug_printf ("Archive Info: ");

    if (! tzx_get_word (fp, &archlength))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_block_32: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    if (! tzx_get_byte (fp, &NoOfTextStrings))
    {
#ifdef DEBUG
        fprintf (stderr, "tzx_block_32: unexpected EOF\n");
        fflush (stderr);
#endif
        return 0;
    }

    for (int i = 0; NoOfTextStrings > i; ++i)
    {
        uint8_t        EntryLength;
        uint8_t        EntryType;

        if (! tzx_get_byte (fp, &EntryType))
        {
#ifdef DEBUG
            fprintf (stderr, "tzx_block_32: unexpected EOF\n");
            fflush (stderr);
#endif
            return 0;
        }

        if (! tzx_get_byte (fp, &EntryLength))
        {
#ifdef DEBUG
            fprintf (stderr, "tzx_block_32: unexpected EOF\n");
            fflush (stderr);
#endif
            return 0;
        }

        char szBuffer[256];
        fread(szBuffer,1,EntryLength,fp);

        switch(EntryType)
        {
            case 0x00:                                                                      // - Full title
                debug_printf ("Full title");
                break;
            case 0x01:                                                                      // - Software house/publisher
                debug_printf ("Software house/publisher");
                break;
            case 0x02:                                                                      // - Author(s)
                debug_printf ("Author(s)");
                break;
            case 0x03:                                                                      // - Year of publication
                debug_printf ("Year of publication");
                break;
            case 0x04:                                                                      // - Language
                debug_printf ("Language");
                break;
            case 0x05:                                                                      // - Game/utility type
                debug_printf ("Game/utility type");
                break;
            case 0x06:                                                                      // - Price
                debug_printf ("Price");
                break;
            case 0x07:                                                                      // - Protection scheme/loader
                debug_printf ("Protection scheme/loader");
                break;
            case 0x08:                                                                      // - Origin
                debug_printf ("Origin");
                break;
            case 0xFF:                                                                      // - Comment(s)
                debug_printf ("Comment(s)");
                break;
        }

        szBuffer[EntryLength] = 0;

        debug_printf (szBuffer);
        debug_printf ("\n");
    }

    return 1;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tzx_write_block_10() - write block 10 of TZX file
 *------------------------------------------------------------------------------------------------------------------------
 */
static uint8_t
tzx_write_block_10 (FILE * fp, uint16_t base_addr, uint16_t len, uint8_t save_data)
{
    uint8_t         flag;
    uint16_t        addr;
    uint16_t        pause = 1000;
    int             pause_lo;
    int             pause_hi;
    int             total_len;
    int             total_len_lo;
    int             total_len_hi;
    uint16_t        idx;
    uint8_t         chksum;
    uint8_t         rtc = 1;

    pause_lo    = (pause & 0x00FF);
    pause_hi    = (pause >> 8) & 0x00FF;

    total_len       = len + 2;
    total_len_lo    = (total_len & 0x00FF);
    total_len_hi    = (total_len >> 8) & 0x00FF;

    putc (pause_lo, fp);
    putc (pause_hi, fp);
    putc (total_len_lo, fp);
    putc (total_len_hi, fp);

    if (save_data)
    {
        flag = 0xFF;
    }
    else
    {
        flag = 0x00;
    }

    putc (flag, fp);
    chksum = flag;

    addr = base_addr;

    for (idx = 0; idx < len; idx++)
    {
        chksum ^= zx_ram_get_8(addr);
        putc (zx_ram_get_8(addr), fp);
        addr++;
    }

    putc (chksum, fp);

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tape_load() - load data from tape (TZX file)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint8_t
tape_load (const char * fname, uint8_t tape_format, uint16_t base_addr, uint16_t maxlen, uint8_t load, uint8_t load_data)
{
    int             ch;
    uint8_t         rtc = 0;

    if (! tape_load_fp)
    {
        tape_load_fp = fopen (fname, "rb");

        if (! tape_load_fp)
        {
            return rtc;
        }

        if (tape_format == TAPE_FORMAT_TZX)
        {
            if (! tzx_read_header (tape_load_fp))
            {
                return rtc;
            }
        }
    }

    if (tape_format == TAPE_FORMAT_TAP)
    {
        rtc = tap_read_block (tape_load_fp, base_addr, maxlen, load, load_data);
    }
    else if (tape_format == TAPE_FORMAT_TZX)
    {
        do
        {
            ch = getc (tape_load_fp);

            if (ch != EOF)
            {
                switch (ch)
                {
                    case 0x10:      // Standard Speed Data Block
                    {
                        rtc = tzx_read_block_10 (tape_load_fp, base_addr, maxlen, load, load_data);
                        break;
                    }
                    case 0x11:      // Turbo Speed Data Block
                    {
                        rtc = tzx_read_block_11 (tape_load_fp, base_addr, maxlen, load, load_data);
                        break;
                    }
                    case 0x12:      // Text Description
                    {
                        rtc = tzx_read_block_12 (tape_load_fp);
                        break;
                    }
                    case 0x14:      // Text Description
                    {
                        rtc = tzx_read_block_14 (tape_load_fp);
                        break;
                    }
                    case 0x20:
                    {
                        rtc = tzx_read_block_20 (tape_load_fp);
                        break;
                    }
                    case 0x21:
                    {
                        rtc = tzx_read_block_21 (tape_load_fp);
                        break;
                    }
                    case 0x22:
                    {
                        rtc = tzx_read_block_22 ();
                        break;
                    }
                    case 0x30:                                                      // text description
                    {
                        rtc = tzx_read_block_30 (tape_load_fp);
                        break;
                    }
                    case 0x32:                                                      // archive info
                    {
                        if (! tzx_read_block_32 (tape_load_fp))
                        {
                            fclose (tape_load_fp);
                            tape_load_fp = nullptr;
                            return 0;
                        }

                        rtc = 0;                                                    // force continuing of tape read
                        break;
                    }
                    default:
                    {
                        fprintf (stderr, "tape_load: unknown block id: 0x%02X\n", ch);
                        fflush (stderr);
                        fclose (tape_load_fp);
                        tape_load_fp = nullptr;
                        return 0;
                    }
                }

                if (rtc == 1)
                {
                    break;
                }
            }
            else
            {
#ifdef DEBUG
                fprintf (stderr, "tape_load: EOF reached\n");
                fflush (stderr);
#endif
                fclose (tape_load_fp);
                tape_load_fp = nullptr;
            }
        } while (ch != EOF);
    }

    return rtc;
}

/*------------------------------------------------------------------------------------------------------------------------
 * tape_save() - save data on tape (TZX file)
 *------------------------------------------------------------------------------------------------------------------------
 */
uint8_t
tape_save (const char * fname, uint16_t base_addr, uint16_t len, uint8_t save_data)
{
    uint8_t         rtc = 0;

    if (! tape_save_fp)
    {
        tape_save_fp = fopen (fname, "wb");

        if (! tape_save_fp)
        {
            return rtc;
        }

        if (! tzx_write_header (tape_save_fp))
        {
            return rtc;
        }
    }

    putc (0x10, tape_save_fp);                                                   // Standard Speed Data Block
    rtc = tzx_write_block_10 (tape_save_fp, base_addr, len, save_data);
    fflush (tape_save_fp);

    return rtc;
}

void
tape_load_close (void)
{
    if (tape_load_fp)
    {
        fclose (tape_load_fp);
        tape_load_fp = nullptr;
    }
}

void
tape_save_close (void)
{
    if (tape_save_fp)
    {
        fclose (tape_save_fp);
        tape_save_fp = nullptr;
    }
}
