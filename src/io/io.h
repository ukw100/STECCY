/*-------------------------------------------------------------------------------------------------------------------------------------------
 * io.h - I/O macros
 *
 * In older versions of stm32f4xx.h the following struct is defined:
 *
 * typedef struct
 * {
 *   __IO uint32_t MODER;
 *   __IO uint32_t OTYPER;
 *   __IO uint32_t OSPEEDR;
 *   __IO uint32_t PUPDR;
 *   __IO uint32_t IDR;
 *   __IO uint32_t ODR;
 *   __IO uint16_t BSRRL;
 *   __IO uint16_t BSRRH;
 *   __IO uint32_t LCKR;
 *   __IO uint32_t AFR[2];
 * } GPIO_TypeDef;
 *
 * The definitions of BSRRL and BSRRH are wrong. There should be a 32 bit register named BSRR.
 * The definition above prevents access to the actual 32-bit register BSRR.
 *
 * There are now two ways to correct this.
 * Instead of
 *
 *   __IO uint16_t BSRRL;
 *   __IO uint16_t BSRRH;
 *
 *  we write:
 *
 *  union
 *  {
 *     __IO uint32_t BSRR;
 *     struct
 *     {
 *       __IO uint16_t BSRRL;
 *       __IO uint16_t BSRRH;
 *     };
 *  };
 *
 *  The second possibility is a 32-bit access to the register pair BSRRL/BSRRH:
 *
 *   (__IO uint32_t *) &(PORTx->BSRRL)
 *
 *  But this hack runs into a type punning error if using -O2.
 *
 *  Solution, if we don't want to modify stm32f4xx.h:
 *
 *    1. We use a struct named GPIO_TypedefExt with the above union.
 *    2. We cast every PORTx into pointer to GPIO_TypedefExt.
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------
 * MIT License
 *
 * Copyright (c) 2015-2021 Frank Meyer - frank(at)fli4l.de
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
#if defined (STM32F4XX)
#include "stm32f4xx_gpio.h"

typedef struct
{
  __IO uint32_t MODER;          /*!< GPIO port mode register,               Address offset: 0x00      */
  __IO uint32_t OTYPER;         /*!< GPIO port output type register,        Address offset: 0x04      */
  __IO uint32_t OSPEEDR;        /*!< GPIO port output speed register,       Address offset: 0x08      */
  __IO uint32_t PUPDR;          /*!< GPIO port pull-up/pull-down register,  Address offset: 0x0C      */
  __IO uint32_t IDR;            /*!< GPIO port input data register,         Address offset: 0x10      */
  __IO uint32_t ODR;            /*!< GPIO port output data register,        Address offset: 0x14      */
  union
  {
    __IO uint32_t BSRR;         /*!< GPIO port bit set/reset register,      Address offset: 0x18      */
    struct
    {
      __IO uint16_t BSRRL;      /*!< GPIO port bit set/reset low register,  Address offset: 0x18      */
      __IO uint16_t BSRRH;      /*!< GPIO port bit set/reset high register, Address offset: 0x1A      */
    };
  };
  __IO uint32_t LCKR;           /*!< GPIO port configuration lock register, Address offset: 0x1C      */
  __IO uint32_t AFR[2];         /*!< GPIO alternate function registers,     Address offset: 0x20-0x24 */
} GPIO_TypeDefExt;

#define GPIO_SET_MODE_OUT_OD(gp, pins, speed)       \
do                                                  \
{                                                   \
    gp.GPIO_Pin     = (pins);                       \
    gp.GPIO_Speed   = (speed);                      \
    gp.GPIO_Mode    = GPIO_Mode_OUT;                \
    gp.GPIO_OType   = GPIO_OType_OD;                \
    gp.GPIO_PuPd    = GPIO_PuPd_NOPULL;             \
} while (0)

#define GPIO_SET_MODE_OUT_PP(gp, pins, speed)       \
do                                                  \
{                                                   \
    gp.GPIO_Pin     = (pins);                       \
    gp.GPIO_Speed   = (speed);                      \
    gp.GPIO_Mode    = GPIO_Mode_OUT;                \
    gp.GPIO_OType   = GPIO_OType_PP;                \
    gp.GPIO_PuPd    = GPIO_PuPd_NOPULL;             \
} while (0)

#define GPIO_SET_MODE_IN_UP(gp, pins, speed)        \
do                                                  \
{                                                   \
    gp.GPIO_Pin     = (pins);                       \
    gp.GPIO_Speed   = (speed);                      \
    gp.GPIO_Mode    = GPIO_Mode_IN;                 \
    gp.GPIO_PuPd    = GPIO_PuPd_UP;                 \
} while (0)

#define GPIO_SET_MODE_IN_NOPULL(gp, pins, speed)    \
do                                                  \
{                                                   \
    gp.GPIO_Pin     = (pins);                       \
    gp.GPIO_Speed   = (speed);                      \
    gp.GPIO_Mode    = GPIO_Mode_IN;                 \
    gp.GPIO_PuPd    = GPIO_PuPd_NOPULL;             \
} while (0)

#define GPIO_RESET_BIT(port,pinmask)        do { (port)->BSRRH = (pinmask); } while (0)
#define GPIO_SET_BIT(port,pinmask)          do { (port)->BSRRL = (pinmask); } while (0)
#define GPIO_SET_VALUE(port,mask,value)     do { ((GPIO_TypeDefExt *) (port))->BSRR = ((mask) << 16) | (value); } while (0)

#elif defined (STM32F10X)

#define GPIO_SET_MODE_OUT_OD(gp, pins, speed)   \
do                                              \
{                                               \
    gp.GPIO_Pin     = (pins);                   \
    gp.GPIO_Speed   = (speed);                  \
    gp.GPIO_Mode    = GPIO_Mode_Out_OD;         \
} while (0)

#define GPIO_SET_MODE_OUT_PP(gp, pins, speed)   \
do                                              \
{                                               \
    gp.GPIO_Pin     = (pins);                   \
    gp.GPIO_Speed   = (speed);                  \
    gp.GPIO_Mode  = GPIO_Mode_Out_PP;           \
} while (0)

#define GPIO_SET_MODE_IN_UP(gp, pins, speed)    \
do                                              \
{                                               \
    gp.GPIO_Pin     = (pins);                   \
    gp.GPIO_Speed   = (speed);                  \
    gp.GPIO_Mode    = GPIO_Mode_IPU;            \
} while (0)

#define GPIO_SET_MODE_IN_NOPULL(gp, pins, speed)    \
do                                                  \
{                                                   \
    gp.GPIO_Pin     = (pins);                       \
    gp.GPIO_Speed   = (speed);                      \
    gp.GPIO_Mode    = GPIO_Mode_IN_FLOATING;        \
} while (0)

#define GPIO_RESET_BIT(port,pinmask)        do { (port)->BRR  = (pinmask); } while (0)
#define GPIO_SET_BIT(port,pinmask)          do { (port)->BSRR = (pinmask); } while (0)
#define GPIO_SET_VALUE(port,mask,value)     do { (port)->BSRR = ((mask) << 16) | (value); } while (0)

#endif
