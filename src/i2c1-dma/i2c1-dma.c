/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1-dma.c - I2C routines
 *-------------------------------------------------------------------------------------------------------------------------------------------
 *
 * Supported Ports/Pins:
 *
 *  +------+---------------+
 *  |      | STM32F407VE   |
 *  +------+---------------+
 *  | SCL  | I2C1  PB8     |
 *  | SDA  | I2C1  PB9     |
 *  +------+---------------+
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2021 Frank Meyer - frank(at)fli4l.de
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
#include "console.h"
#include "i2c1-dma.h"
#include "delay.h"
#include "io.h"

#define I2C_TIMEOUT_CNT             100                                    // timeout counter: 100
#define I2C_TIMEOUT_USEC             50                                    // timeout: 100 * 50 usec = 5 msec

#if defined (STM32F407VE)

#define PERIPH_SCL_PORT             RCC_AHB1Periph_GPIOB
#define SCL_PORT                    GPIOB                                   // SCL PB8
#define SCL_PIN                     GPIO_Pin_8
#define SCL_PIN_SOURCE              GPIO_PinSource8

#define PERIPH_SDA_PORT             RCC_AHB1Periph_GPIOB
#define SDA_PORT                    GPIOB                                   // SDA PB9
#define SDA_PIN                     GPIO_Pin_9
#define SDA_PIN_SOURCE              GPIO_PinSource9

#define I2C_CHANNEL                 I2C1
#define I2C_CHANNEL_EV_IRQn         I2C1_EV_IRQn
#define RCC_PERIPH_I2C              RCC_APB1Periph_I2C1
#define GPIO_AF_I2C                 GPIO_AF_I2C1

#else
#error STM32 undefined
#endif

#define I2C_DMA_CLOCK_CMD           RCC_AHB1PeriphClockCmd
#define I2C_DMA_CLOCK               RCC_AHB1Periph_DMA1

#define I2C_TX_DMA_CHANNEL          DMA_Channel_1                   // DMA channel
#define I2C_TX_DMA_STREAM           DMA1_Stream6                    // transmitter stream 6
#define I2C_TX_DMA_STREAM_IRQn      DMA1_Stream6_IRQn               // IRQ stream 6
#define I2C_TX_DMA_STREAM_ISR       DMA1_Stream6_IRQHandler         // IRQ handler stream 6
#define I2C_TX_DMA_STREAM_IRQ_TC    DMA_IT_TCIF6                    // transfer complete interrupt DMA1, Stream 6

#define I2C_RX_DMA_CHANNEL          DMA_Channel_1                   // DMA channel
#define I2C_RX_DMA_STREAM           DMA1_Stream0                    // receiver stream 0
#define I2C_RX_DMA_STREAM_IRQn      DMA1_Stream0_IRQn               // IRQ stream 0
#define I2C_RX_DMA_STREAM_ISR       DMA1_Stream0_IRQHandler         // IRQ handler stram 0
#define I2C_RX_DMA_STREAM_IRQ_TC    DMA_IT_TCIF0                    // transfer complete interrupt DMA1, Stream 0

#define I2C_CHANNEL_EV_ISR          I2C1_EV_IRQHandler

static volatile uint_fast8_t        i2c1_slave_addr;
static volatile uint8_t             i2c1_dma_direction_write;
static volatile uint8_t             i2c1_dma_only_write;
volatile uint_fast8_t               i2c1_dma_state = I2C1_DMA_STATE_IDLE;

static uint32_t                     i2c1_clockspeed;

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c event handler
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void I2C_CHANNEL_EV_ISR (void)
{
    if (i2c1_dma_state == I2C1_DMA_STATE_BUSY)                      // event got by DMA transfer, handle it
    {
        if (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_SB) == SET)
        {
            if (i2c1_dma_direction_write)
            {                                                       // transmitter
                I2C_Send7bitAddress(I2C_CHANNEL, i2c1_slave_addr, I2C_Direction_Transmitter);
            }
            else
            {                                                       // receiver
                I2C_Send7bitAddress(I2C_CHANNEL, i2c1_slave_addr, I2C_Direction_Receiver);
            }
        }
        else if (I2C_CheckEvent(I2C_CHANNEL, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == SUCCESS)
        {
            if (i2c1_dma_direction_write)
            {                                                       // transmitter
                DMA_Cmd (I2C_TX_DMA_STREAM, ENABLE);                // enable TX DMA
            }
        }
        else if (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BTF))
        {
            if (i2c1_dma_direction_write)                           // transmitter
            {
                if (i2c1_dma_only_write)
                {
                    // DMA_Cmd(I2C_TX_DMA_STREAM, DISABLE);
                    // I2C_DMACmd(I2C_CHANNEL, DISABLE);
                    i2c1_dma_state = I2C1_DMA_STATE_IDLE;
                }
                else
                {
                    DMA_Cmd(I2C_RX_DMA_STREAM, ENABLE);             // enable RX DMA
                    I2C_DMALastTransferCmd(I2C_CHANNEL, ENABLE);    // next DMA transfer is the last one, create NACK after reading last byte
                    i2c1_dma_direction_write = 0;                   // set direction to RX
                    I2C_ClearFlag(I2C_CHANNEL, I2C_FLAG_BTF);       // clear BTF flag
                    I2C_GenerateSTART(I2C_CHANNEL, ENABLE);         // generate Start
                }
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * I2C_TX_DMA_STREAM_ISR() - we need to send a STOP before reading, a repeated START does not work
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
I2C_TX_DMA_STREAM_ISR (void)
{
    if (DMA_GetITStatus(I2C_TX_DMA_STREAM, I2C_TX_DMA_STREAM_IRQ_TC))               // check transfer complete interrupt flag
    {
        DMA_ClearFlag (I2C_TX_DMA_STREAM, I2C_TX_DMA_STREAM_IRQ_TC);
        I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * I2C_RX_DMA_STREAM_ISR() - generate stop, disable DMA, set state to idle
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
I2C_RX_DMA_STREAM_ISR (void)
{
    if (DMA_GetITStatus(I2C_RX_DMA_STREAM, I2C_RX_DMA_STREAM_IRQ_TC))               // check transfer complete interrupt flag
    {
        DMA_ClearFlag (I2C_RX_DMA_STREAM, I2C_RX_DMA_STREAM_IRQ_TC);
        I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);
        DMA_Cmd(I2C_RX_DMA_STREAM, DISABLE);
        DMA_Cmd(I2C_TX_DMA_STREAM, DISABLE);
        // I2C_DMACmd(I2C_CHANNEL, DISABLE);
        i2c1_dma_state = I2C1_DMA_STATE_IDLE;
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1_dma_init_tx_dma() - initialize TX DMA
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
i2c1_dma_init_tx_dma (uint8_t * buffer)
{
    DMA_InitTypeDef  dma;

    DMA_StructInit (&dma);

    DMA_Cmd(I2C_TX_DMA_STREAM, DISABLE);
    DMA_DeInit(I2C_TX_DMA_STREAM);

    dma.DMA_Mode                = DMA_Mode_Normal;                  // setting normal mode (non circular)
    dma.DMA_PeripheralBaseAddr  = (uint32_t) (&(I2C_CHANNEL->DR));  // address of data reading register of I2C1
    dma.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;      // peripheral data size = 8bit
    dma.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;          // memory data size = 8bit
    dma.DMA_BufferSize          = 0;                                // number of data to be transfered
    dma.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;        // automatic memory increment disable for peripheral
    dma.DMA_MemoryInc           = DMA_MemoryInc_Enable;             // automatic memory increment enable for memory
    dma.DMA_Priority            = DMA_Priority_Medium;              // or DMA_Priority_High or DMA_Priority_VeryHigh

#if defined(STM32F4XX)
    dma.DMA_DIR                 = DMA_DIR_MemoryToPeripheral;       // copy from memory to I2C
    dma.DMA_Channel             = I2C_TX_DMA_CHANNEL;               // DMA channel
    dma.DMA_Memory0BaseAddr     = (uint32_t) buffer;                // buffer in memory
    dma.DMA_FIFOMode            = DMA_FIFOMode_Disable;             // no FIFO
    dma.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull;       // not affected
    dma.DMA_MemoryBurst         = DMA_MemoryBurst_Single;           // memory single burst
    dma.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;       // peripheral single burst
#elif defined (STM32F10X)
    dma.DMA_DIR                 = DMA_DIR_PeripheralDst;            // copy from memory to I2C
    dma.DMA_M2M                 = DMA_M2M_Disable;                  // disable memory to memory
    dma.DMA_MemoryBaseAddr      = (uint32_t) buffer;                // buffer in memory
#endif

    DMA_Init(I2C_TX_DMA_STREAM, &dma);
    DMA_ITConfig(I2C_TX_DMA_STREAM, DMA_IT_TC, ENABLE);             // interrupt on transmission complete

    NVIC_InitTypeDef        nvic;

    nvic.NVIC_IRQChannel                    = I2C_TX_DMA_STREAM_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority  = 0;
    nvic.NVIC_IRQChannelSubPriority         = 0;
    nvic.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&nvic);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1_dma_init_rx_dma() - initialize RX DMA
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
i2c1_dma_init_rx_dma (uint8_t * buffer)
{
    DMA_InitTypeDef  dma;

    DMA_StructInit (&dma);

    DMA_Cmd(I2C_RX_DMA_STREAM, DISABLE);
    DMA_DeInit(I2C_RX_DMA_STREAM);

    dma.DMA_Mode                = DMA_Mode_Normal;                  // setting normal mode (non circular)
    dma.DMA_PeripheralBaseAddr  = (uint32_t) (&(I2C_CHANNEL->DR));  // address of data reading register of I2C1
    dma.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;      // peripheral data size = 8bit
    dma.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;          // memory data size = 8bit
    dma.DMA_BufferSize          = 0;                                // number of data to be transfered
    dma.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;        // automatic memory increment disable for peripheral
    dma.DMA_MemoryInc           = DMA_MemoryInc_Enable;             // automatic memory increment enable for memory
    dma.DMA_Priority            = DMA_Priority_Medium;              // or DMA_Priority_High or DMA_Priority_VeryHigh

#if defined(STM32F4XX)
    dma.DMA_DIR                 = DMA_DIR_PeripheralToMemory;       // copy from I2C to memory
    dma.DMA_Channel             = I2C_RX_DMA_CHANNEL;               // DMA channel
    dma.DMA_Memory0BaseAddr     = (uint32_t) buffer;                // buffer in memory
    dma.DMA_FIFOMode            = DMA_FIFOMode_Disable;             // no FIFO
    dma.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull;       // not affected
    dma.DMA_MemoryBurst         = DMA_MemoryBurst_Single;           // memory single burst
    dma.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;       // peripheral single burst
#elif defined (STM32F10X)
    dma.DMA_DIR                 = DMA_DIR_PeripheralSRC;            // copy from I2C to memory
    dma.DMA_M2M                 = DMA_M2M_Disable;                  // disable memory to memory
    dma.DMA_MemoryBaseAddr      = (uint32_t) buffer;                // buffer in memory
#endif

    DMA_Init(I2C_RX_DMA_STREAM, &dma);
    DMA_ITConfig(I2C_RX_DMA_STREAM, DMA_IT_TC, ENABLE);             // interrupt on transmission complete

    /*-------------------------------------------------------------------------------------------------------------------------------------------
     * initialize NVIC
     *-------------------------------------------------------------------------------------------------------------------------------------------
     */
    NVIC_InitTypeDef        nvic;

    nvic.NVIC_IRQChannel                    = I2C_RX_DMA_STREAM_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority  = 0;
    nvic.NVIC_IRQChannelSubPriority         = 0;
    nvic.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&nvic);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * init i2c bus
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
i2c1_init_i2c (void)
{
    I2C_InitTypeDef  i2c;

    I2C_StructInit (&i2c);

    I2C_DeInit(I2C_CHANNEL);

    i2c.I2C_Mode                  = I2C_Mode_I2C;
    i2c.I2C_DutyCycle             = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1           = 0x00;
    i2c.I2C_Ack                   = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress   = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_ClockSpeed            = i2c1_clockspeed;

    I2C_Init (I2C_CHANNEL, &i2c);
    I2C_ITConfig(I2C_CHANNEL, I2C_IT_EVT, ENABLE);
    I2C_Cmd (I2C_CHANNEL, ENABLE);
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * reset I2C bus
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
i2c1_reset_bus (void)
{
    GPIO_InitTypeDef    gpio;
    uint_fast8_t        idx;

#if defined (STM32F4XX)
    RCC_AHB1PeriphClockCmd(PERIPH_SCL_PORT, ENABLE);                // for SCL
    RCC_AHB1PeriphClockCmd(PERIPH_SDA_PORT, ENABLE);                // for SDA

    GPIO_StructInit (&gpio);

    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_OD;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Pin   = SCL_PIN;                                      // SCL pin PB8
    GPIO_Init(SCL_PORT, &gpio);

    GPIO_StructInit (&gpio);

    gpio.GPIO_Mode  = GPIO_Mode_IN;
    gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Pin   = SDA_PIN;                                      // SDA pin PB9

    GPIO_Init(SDA_PORT, &gpio);

#elif defined (STM32F10X)

    RCC_APB2PeriphClockCmd(PERIPH_SCL_SDA_PORT, ENABLE);            // for SCL & SDA

    GPIO_StructInit (&gpio);
    gpio.GPIO_Pin   = SCL_PIN;                                      // SCL pin PB6
    gpio.GPIO_Mode  = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(SCL_PORT, &gpio);

    GPIO_StructInit (&gpio);
    gpio.GPIO_Pin   = SDA_PIN;                                      // SDA pin PB7
    gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(SDA_PORT, &gpio);

#else
#error STM32 undefined
#endif

    if (GPIO_ReadInputDataBit(SDA_PORT, SDA_PIN) == Bit_RESET)      // SDA low?
    {                                                               // yes...
        GPIO_RESET_BIT(SCL_PORT, SCL_PIN);                          // try to release it here....
        delay_msec (1);
    }

    for (idx = 0; idx < 9; idx++)
    {
        GPIO_SET_BIT(SCL_PORT, SCL_PIN);
        delay_msec (1);

        if (GPIO_ReadInputDataBit(SDA_PORT, SDA_PIN) == Bit_SET)    // look for SDA = HIGH
        {                                                           // step 2!
            break;
        }

        GPIO_RESET_BIT(SCL_PORT, SCL_PIN);
        delay_msec (1);
    }

    GPIO_SET_BIT(SCL_PORT, SCL_PIN);
    delay_msec (1);

    if (GPIO_ReadInputDataBit(SDA_PORT, SDA_PIN) == Bit_RESET)      // SDA still low
    {                                                               // yes...
        // log_message ("error: cannot reset I2C bus");
    }
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * initialize I2C
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
i2c1_init (uint32_t clockspeed)
{
    static uint32_t     last_clockspeed;
    GPIO_InitTypeDef    gpio;

    if (last_clockspeed == clockspeed)                              // if already called with same clockspeed, return
    {
        return;
    }

    last_clockspeed = clockspeed;

    I2C_DeInit(I2C_CHANNEL);

    i2c1_reset_bus ();                                               // if 2i2c bus hangs, reset it by toggling SCK 9 times

    GPIO_StructInit (&gpio);

#if defined (STM32F4XX)
    RCC_AHB1PeriphClockCmd(PERIPH_SCL_PORT, ENABLE);                // for SCL
    RCC_AHB1PeriphClockCmd(PERIPH_SDA_PORT, ENABLE);                // for SDA

    // I2C reset
    RCC_APB1PeriphResetCmd(RCC_PERIPH_I2C, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_PERIPH_I2C, DISABLE);

    GPIO_PinAFConfig(SCL_PORT, SCL_PIN_SOURCE, GPIO_AF_I2C);        // SCL: PA8 / PB8
    GPIO_PinAFConfig(SDA_PORT, SDA_PIN_SOURCE, GPIO_AF_I2C);        // SDA: PC9 / PB9

    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_OType = GPIO_OType_OD;                                // OpenDrain!
    gpio.GPIO_PuPd  = GPIO_PuPd_UP; // GPIO_PuPd_NOPULL;

    gpio.GPIO_Pin = SCL_PIN;                                        // SCL pin
    GPIO_Init(SCL_PORT, &gpio);

    gpio.GPIO_Pin = SDA_PIN;                                        // SDA pin
    GPIO_Init(SDA_PORT, &gpio);

    RCC_APB1PeriphClockCmd(RCC_PERIPH_I2C, ENABLE);                 // new: enable I2C Peripheral Clock *after* initializing GPIO

#elif defined (STM32F10X)
    RCC_APB2PeriphClockCmd(PERIPH_SCL_SDA_PORT, ENABLE);            // for SCL & SDA

    // I2C reset
    RCC_APB1PeriphResetCmd(RCC_PERIPH_I2C, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_PERIPH_I2C, DISABLE);

    gpio.GPIO_Pin   = SCL_PIN | SDA_PIN;                            // SCL=PB6, SDA=PB7
    gpio.GPIO_Mode  = GPIO_Mode_AF_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SCL_PORT, &gpio);

    RCC_APB1PeriphClockCmd(RCC_PERIPH_I2C, ENABLE);                 // new: enable I2C Peripheral Clock *after* initializing GPIO
#else
#error STM32 undefined
#endif

    i2c1_clockspeed = clockspeed;
    i2c1_init_i2c ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1_dma_init() - initialize I2C DMA
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void
i2c1_dma_init (uint8_t * sndbuf, uint8_t * recbuf)
{
    I2C_DMACmd (I2C_CHANNEL, ENABLE);
    I2C_DMA_CLOCK_CMD (I2C_DMA_CLOCK, ENABLE);
    i2c1_dma_init_tx_dma (sndbuf);                                      // init TX DMA
    i2c1_dma_init_rx_dma (recbuf);                                      // init RX DMA
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1_dma_start() - send buffer via DMA, then receive data via DMA
 *
 * This function does *not wait* until I2C peripheral is ready.
 *
 * Return values:
 * == 1  OK, I2C DMA transfer started
 * == 0  Error, I2C is currently busy
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
i2c1_dma_start (uint_fast8_t slave_addr, uint_fast8_t sndlen, uint_fast8_t reclen)
{
    uint_fast8_t rtc;

    if (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BUSY) || i2c1_dma_state == I2C1_DMA_STATE_BUSY)
    {
        rtc = 0;
    }
    else
    {
        NVIC_InitTypeDef nvic;

        nvic.NVIC_IRQChannel                    = I2C_CHANNEL_EV_IRQn;
        nvic.NVIC_IRQChannelPreemptionPriority  = 0;
        nvic.NVIC_IRQChannelSubPriority         = 0;
        nvic.NVIC_IRQChannelCmd                 = ENABLE;
        NVIC_Init(&nvic);

        // I2C_DMA_CLOCK_CMD (I2C_DMA_CLOCK, ENABLE);
        i2c1_slave_addr = slave_addr;
        i2c1_dma_state  = I2C1_DMA_STATE_BUSY;

        if (sndlen == 0)                                            // only receive
        {
            i2c1_dma_direction_write = 0;                           // set direction to RX
            i2c1_dma_only_write      = 0;                           // only write is false
            I2C_DMALastTransferCmd(I2C_CHANNEL, ENABLE);            // next DMA transfer is the last one, create NACK after reading last byte
            DMA_SetCurrDataCounter (I2C_RX_DMA_STREAM, reclen);     // set RX len
        }
        else if (reclen == 0)                                       // only send
        {
            i2c1_dma_direction_write = 1;                           // set direction to TX
            i2c1_dma_only_write      = 1;                           // only write is true
            DMA_SetCurrDataCounter (I2C_TX_DMA_STREAM, sndlen);     // set TX len
        }
        else                                                        // send, then receive
        {
            i2c1_dma_direction_write = 1;                           // set direction to TX
            i2c1_dma_only_write      = 0;                           // only write is false
            DMA_SetCurrDataCounter (I2C_TX_DMA_STREAM, sndlen);     // set TX len
            DMA_SetCurrDataCounter (I2C_RX_DMA_STREAM, reclen);     // set RX len
        }

        I2C_GenerateSTART (I2C_CHANNEL, ENABLE);

        rtc = 1;
    }

    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1_dma_write() - send buffer via DMA transfer
 *
 * This function waits until I2C peripheral is ready.
 *
 * EXPERIMENTAL - DOES NOT WORK YET
 *
 * Return values:
 * == 1  OK, I2C DMA transfer started
 * == 0  Error, DMA transfer could not be started
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
i2c1_dma_write (uint_fast8_t slave_addr, uint_fast8_t sndlen)
{
    uint_fast8_t    rtc;

    while (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BUSY))
    {
    }

    rtc = i2c1_dma_start (slave_addr, sndlen, 0);
    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1_dma_read() - receive data via DMA transfer
 *
 * This function waits until I2C peripheral is ready.
 *
 * EXPERIMENTAL - DOES NOT WORK YET
 *
 * Return values:
 * == 1  OK, I2C DMA transfer started
 * == 0  Error, DMA transfer could not be started
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
i2c1_dma_read (uint_fast8_t slave_addr, uint_fast8_t reclen)
{
    uint_fast8_t    rtc;

    while (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BUSY))
    {
        rtc = 0;
    }

    rtc = i2c1_dma_start (slave_addr, 0, reclen);
    return rtc;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * handle timeout
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
i2c1_handle_timeout (void)
{
    I2C_GenerateSTOP (I2C_CHANNEL, ENABLE);
    I2C_SoftwareResetCmd (I2C_CHANNEL, ENABLE);
    I2C_SoftwareResetCmd (I2C_CHANNEL, DISABLE);

    I2C_DeInit (I2C_CHANNEL);
    i2c1_init_i2c ();
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * wait for flags
 *
 * Return values:
 * == 1  OK, got flag
 * == 0  timeout
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
static uint_fast8_t
i2c1_wait_for_flags (uint32_t flag1, uint32_t flag2)
{
    uint32_t  timeout = I2C_TIMEOUT_CNT;

    if (flag2)
    {
        while ((!I2C_GetFlagStatus(I2C_CHANNEL, flag1)) || (!I2C_GetFlagStatus(I2C_CHANNEL, flag2)))
        {
            if (timeout > 0)
            {
                delay_usec(I2C_TIMEOUT_USEC);
                timeout--;
            }
            else
            {
                i2c1_handle_timeout ();
                return 0;
            }
        }
    }
    else
    {
        while (! I2C_GetFlagStatus(I2C_CHANNEL, flag1))
        {
            if (timeout > 0)
            {
                delay_usec(I2C_TIMEOUT_USEC);
                timeout--;
            }
            else
            {
                i2c1_handle_timeout ();
                return 0;
            }
        }
    }

    return 1;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1_read - I2C polling read
 *
 * This function waits until I2C peripheral is ready.
 *
 * return values:
 * ==  0 I2C_OK
 *  <  0 Error
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int_fast16_t
i2c1_read (uint_fast8_t slave_addr, uint8_t * data, uint_fast16_t cnt)
{
    uint_fast16_t   n;

    while (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BUSY))
    {
        ;
    }

    I2C_GenerateSTART(I2C_CHANNEL, ENABLE);                                        // start sequence

    if (! i2c1_wait_for_flags (I2C_FLAG_SB, 0))
    {
        return I2C_ERROR_NO_FLAG_SB2;
    }

    I2C_Send7bitAddress(I2C_CHANNEL, slave_addr, I2C_Direction_Receiver);          // send slave address (receiver)

    if (! i2c1_wait_for_flags (I2C_FLAG_ADDR, 0))
    {
        return I2C_ERROR_NO_FLAG_ADDR2;
    }

    I2C_CHANNEL->SR2;                                                               // clear ADDR-Flag

    I2C_AcknowledgeConfig(I2C_CHANNEL, ENABLE);                                     // ACK enable

    for (n = 0; n < cnt; n++)                                                       // read all data
    {
        if (n + 1 == cnt)
        {
            I2C_AcknowledgeConfig(I2C_CHANNEL, DISABLE);                            // ACK disable
            I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);                                  // stop sequence

            while (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BUSY))
            {
                ;
            }
        }

        if (! i2c1_wait_for_flags (I2C_FLAG_RXNE, 0))
        {
            return I2C_ERROR_NO_FLAG_RXNE;
        }

        data[n] = I2C_ReceiveData(I2C_CHANNEL);                                     // read data
    }

    I2C_AcknowledgeConfig(I2C_CHANNEL, ENABLE);                                     // ACK enable

    return I2C_OK;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * i2c1_write - I2C polling write
 *
 * This function waits until I2C peripheral is ready.
 *
 * return values:
 * ==  0 I2C_OK
 *  <  0 Error
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int_fast16_t
i2c1_write (uint_fast8_t slave_addr, uint8_t * data, uint_fast16_t cnt)
{
    uint8_t         value;
    uint_fast16_t   n;

    while (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BUSY))
    {
       ;
    }

    I2C_GenerateSTART(I2C_CHANNEL, ENABLE);

    if (! i2c1_wait_for_flags (I2C_FLAG_SB, 0))
    {
        return I2C_ERROR_NO_FLAG_SB;
    }

    I2C_AcknowledgeConfig(I2C_CHANNEL, ENABLE);                                     // ACK enable

    I2C_Send7bitAddress (I2C_CHANNEL, slave_addr, I2C_Direction_Transmitter);       // send slave address (transmitter)

    if (! i2c1_wait_for_flags (I2C_FLAG_ADDR, 0))
    {
        return I2C_ERROR_NO_FLAG_ADDR;
    }

    I2C_CHANNEL->SR2;                                                               // clear ADDR-Flag

    if (! i2c1_wait_for_flags (I2C_FLAG_TXE, 0))
    {
        return I2C_ERROR_NO_FLAG_TXE;
    }

    for (n = 0; n < cnt; n++)                                                       // send all data
    {
        value = *data++;                                                            // read data from buffer

        I2C_SendData(I2C_CHANNEL, value);                                           // send data

        if (! i2c1_wait_for_flags (I2C_FLAG_TXE, I2C_FLAG_BTF))
        {
            return I2C_ERROR_NO_TXE_OR_BTF;
        }
    }

    I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);                                          // stop sequence

    while (I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_STOPF))                          // fm: necessary?
    {
        ;
    }

    return I2C_OK;
}
