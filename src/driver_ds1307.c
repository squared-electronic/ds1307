/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * 
 * The MIT License (MIT)
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
 *
 * @file      driver_ds1307.c
 * @brief     driver ds1307 source file
 * @version   1.0.0
 * @author    Shifeng Li
 * @date      2023-05-31
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2023/05/31  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */

#include "driver_ds1307.h"

/**
 * @brief chip information definition
 */
#define CHIP_NAME                 "Maxim Integrated DS1307"        /**< chip name */
#define MANUFACTURER_NAME         "Maxim Integrated"               /**< manufacturer name */
#define SUPPLY_VOLTAGE_MIN        4.5f                             /**< chip min supply voltage */
#define SUPPLY_VOLTAGE_MAX        5.5f                             /**< chip max supply voltage */
#define MAX_CURRENT               1.50f                            /**< chip max current */
#define TEMPERATURE_MIN           -40.0f                           /**< chip min operating temperature */
#define TEMPERATURE_MAX           85.0f                            /**< chip max operating temperature */
#define DRIVER_VERSION            1000                             /**< driver version */

/**
 * @brief chip register definition
 */
#define DS1307_REG_SECOND               0x00        /**< second register */
#define DS1307_REG_MINUTE               0x01        /**< minute register */
#define DS1307_REG_HOUR                 0x02        /**< hour register */
#define DS1307_REG_WEEK                 0x03        /**< week register */
#define DS1307_REG_DATE                 0x04        /**< date register */
#define DS1307_REG_MONTH                0x05        /**< month register */
#define DS1307_REG_YEAR                 0x06        /**< year register */
#define DS1307_REG_CONTROL              0x07        /**< control register */
#define DS1307_REG_RAM                  0x08        /**< ram start register */

/**
 * @brief chip address definition
 */
#define DS1307_ADDRESS        0xD0        /**< iic device address */ 

/**
 * @brief     write one byte
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] reg iic register address
 * @param[in] data written data
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
static uint8_t a_ds1307_iic_write(ds1307_handle_t *handle, uint8_t reg, uint8_t data)
{
    if (handle->iic_write(DS1307_ADDRESS, reg, &data, 1) != 0)        /* write data */
    {
        return 1;                                                     /* return error */
    }
    else
    {
        return 0;                                                     /* success return 0 */
    }
}

/**
 * @brief     write multiple bytes
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] reg iic register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len data buffer length
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
static uint8_t a_ds1307_iic_multiple_write(ds1307_handle_t *handle, uint8_t reg, uint8_t *buf, uint8_t len)
{
    if (handle->iic_write(DS1307_ADDRESS, reg, buf, len) != 0)       /* write data */
    {
        return 1;                                                    /* return error */
    }
    else
    {                                                                /* success return 0 */
        return 0;
    }
}

/**
 * @brief      read multiple bytes
 * @param[in]  *handle pointer to a ds1307 handle structure
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len data buffer length
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
static uint8_t a_ds1307_iic_multiple_read(ds1307_handle_t *handle, uint8_t reg, uint8_t *buf, uint8_t len)
{
    if (handle->iic_read(DS1307_ADDRESS, reg, buf, len) != 0)        /* read data */
    {
        return 1;                                                    /* return error */
    }
    else
    {                                                                /* success return 0 */
        return 0;
    }
}

/**
 * @brief     hex to bcd
 * @param[in] val hex data
 * @return    bcd data
 * @note      none
 */
static uint8_t a_ds1307_hex2bcd(uint8_t val)
{
    uint8_t i, j, k;
    
    i = val / 10;            /* get tens place */
    j = val % 10;            /* get ones place */
    k = j + (i << 4);        /* set bcd */
    
    return k;                /* return bcd */
}

/**
 * @brief     bcd to hex
 * @param[in] val bcd data
 * @return    hex data
 * @note      none
 */
static uint8_t a_ds1307_bcd2hex(uint8_t val)
{
    uint8_t temp;
    
    temp = val & 0x0F;              /* get ones place */
    val = (val >> 4) & 0x0F;        /* get tens place */
    val = val * 10;                 /* set tens place */
    temp = temp + val;              /* get hex */
    
    return temp;                    /* return hex */
}

/**
 * @brief     set the current time
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] *t pointer to a time structure
 * @return    status code
 *            - 0 success
 *            - 1 set time failed
 *            - 2 handle or time is NULL
 *            - 3 handle is not initialized
 *            - 4 time is invalid
 * @note      none
 */
uint8_t ds1307_set_time(ds1307_handle_t *handle, ds1307_time_t *t)
{
    uint8_t res;
    uint8_t reg;
    uint8_t buf[7];
    uint16_t year;
  
    /* read current second register to preserve oscillator enable bit */
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_SECOND, &reg,
                                     1); /* read second */
    if (res != 0)                        /* check result */
    {
      return 1; /* return error */
    }
  
    /* prepare buffer with all time register values */
    buf[0] = a_ds1307_hex2bcd(t->second) | (reg & (1 << 7)); /* second with preserved oscillator bit */
    buf[1] = a_ds1307_hex2bcd(t->minute); /* minute */
    buf[2] = (uint8_t)((1 << 6) | (t->am_pm << 5) | a_ds1307_hex2bcd(t->hour)); /* hour in 12H format */
    buf[3] = a_ds1307_hex2bcd(t->week);            /* week */
    buf[4] = a_ds1307_hex2bcd(t->date);            /* date */
    buf[5] = a_ds1307_hex2bcd(t->month);           /* month */
  
    year = t->year - 2000;                    /* year - 2000 */
    buf[6] = a_ds1307_hex2bcd((uint8_t)year); /* year */
  
    /* write all time registers at once */
    res = a_ds1307_iic_multiple_write(handle, DS1307_REG_SECOND, buf,
                                      7); /* write all time registers */
    if (res != 0)                         /* check result */
    {
      return 1; /* return error */
    }
  
    return 0; /* success return 0 */
}

/**
 * @brief      get the current time
 * @param[in]  *handle pointer to a ds1307 handle structure
 * @param[out] *t pointer to a time structure
 * @return     status code
 *             - 0 success
 *             - 1 get time failed
 *             - 2 handle or time is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t ds1307_get_time(ds1307_handle_t *handle, ds1307_time_t *t)
{
    uint8_t res;
    uint8_t buf[7];
    
    memset(buf, 0, sizeof(uint8_t) * 7);                                                  /* clear the buffer */
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_SECOND, (uint8_t *)buf, 7);       /* multiple_read */
    if (res != 0)                                                                         /* check result */
    {
        handle->debug_print("");                           /* multiple read failed */
        
        return 1;                                                                         /* return error */
    }
    t->year = a_ds1307_bcd2hex(buf[6]) + 2000;                                            /* get year */
    t->month = a_ds1307_bcd2hex(buf[5] & 0x1F);                                           /* get month */
    t->week = a_ds1307_bcd2hex(buf[3] & 0x7);                                             /* get week */
    t->date = a_ds1307_bcd2hex(buf[4] & 0x3F);                                            /* get date */
    t->am_pm = (ds1307_am_pm_t)((buf[2] >> 5) & 0x01);                                    /* get am pm */
    t->hour = a_ds1307_bcd2hex(buf[2] & 0x1F);   
    t->minute = a_ds1307_bcd2hex(buf[1]);                                                 /* get minute */
    t->second = a_ds1307_bcd2hex(buf[0] & (~(1 << 7)));                                   /* get second */
    
    return 0;                                                                             /* success return 0 */
}

/**
 * @brief     enable or disable the oscillator
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] enable bool value
 * @return    status code
 *            - 0 success
 *            - 1 set oscillator failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t ds1307_set_oscillator(ds1307_handle_t *handle, ds1307_bool_t enable)
{
    uint8_t res;
    uint8_t prev;
    
    if (handle == NULL)                                                           /* check handle */
    {
        return 2;                                                                 /* return error */
    }
    if (handle->inited != 1)                                                      /* check handle initialization */
    {
        return 3;                                                                 /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_SECOND, &prev, 1);        /* read second */
    if (res != 0)                                                                 /* check result */
    {
        handle->debug_print("");                     /* read second failed */
        
        return 1;                                                                 /* return error */
    }
    prev &= ~(1 << 7);                                                            /* clear config */
    prev |= (!enable) << 7;                                                       /* set enable */
    res = a_ds1307_iic_write(handle, DS1307_REG_SECOND, prev);                    /* write second */
    if (res != 0)                                                                 /* check result */
    {
        handle->debug_print("");                    /* write second failed */
        
        return 1;                                                                 /* return error */
    }
    
    return 0;                                                                     /* success return 0 */
}

/**
 * @brief      get the chip oscillator status
 * @param[in]  *handle pointer to a ds1307 handle structure
 * @param[out] *enable pointer to a bool value buffer
 * @return     status code
 *             - 0 success
 *             - 1 get oscillator failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t ds1307_get_oscillator(ds1307_handle_t *handle, ds1307_bool_t *enable)
{
    uint8_t res;
    uint8_t prev;
    
    if (handle == NULL)                                                                      /* check handle */
    {
        return 2;                                                                            /* return error */
    }
    if (handle->inited != 1)                                                                 /* check handle initialization */
    {
        return 3;                                                                            /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_SECOND, (uint8_t *)&prev, 1);        /* multiple read */
    if (res != 0)                                                                            /* check result */
    {
        handle->debug_print("");                                /* read second failed */
        
        return 1;                                                                            /* return error */
    }
    *enable = (ds1307_bool_t)(!((prev >> 7) & 0x01));                                        /* get enable */
    
    return 0;                                                                                /* success return 0 */
}

/**
 * @brief     set the output level
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] level output level
 * @return    status code
 *            - 0 success
 *            - 1 set output level failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t ds1307_set_output_level(ds1307_handle_t *handle, ds1307_output_level_t level)
{
    uint8_t res;
    uint8_t prev;
    
    if (handle == NULL)                                                            /* check handle */
    {
        return 2;                                                                  /* return error */
    }
    if (handle->inited != 1)                                                       /* check handle initialization */
    {
        return 3;                                                                  /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_CONTROL, &prev, 1);        /* read control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                     /* read control failed */
        
        return 1;                                                                  /* return error */
    }
    prev &= ~(1 << 7);                                                             /* clear config */
    prev |= level << 7;                                                            /* set setting */
    res = a_ds1307_iic_write(handle, DS1307_REG_CONTROL, prev);                    /* write control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                    /* write control failed */
        
        return 1;                                                                  /* return error */
    }
    
    return 0;                                                                      /* success return 0 */
}

/**
 * @brief      get the output level
 * @param[in]  *handle pointer to a ds1307 handle structure
 * @param[out] *level pointer to an output level buffer
 * @return     status code
 *             - 0 success
 *             - 1 get output level failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t ds1307_get_output_level(ds1307_handle_t *handle, ds1307_output_level_t *level)
{
    uint8_t res;
    uint8_t prev;
    
    if (handle == NULL)                                                            /* check handle */
    {
        return 2;                                                                  /* return error */
    }
    if (handle->inited != 1)                                                       /* check handle initialization */
    {
        return 3;                                                                  /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_CONTROL, &prev, 1);        /* read control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                     /* read control failed */
        
        return 1;                                                                  /* return error */
    }
    *level = (ds1307_output_level_t)((prev >> 7) & 0x01);                          /* set level */
    
    return 0;                                                                      /* success return 0 */
}

/**
 * @brief     set the output mode
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] mode output mode
 * @return    status code
 *            - 0 success
 *            - 1 set output mode failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t ds1307_set_output_mode(ds1307_handle_t *handle, ds1307_output_mode_t mode)
{
    uint8_t res;
    uint8_t prev;
    
    if (handle == NULL)                                                            /* check handle */
    {
        return 2;                                                                  /* return error */
    }
    if (handle->inited != 1)                                                       /* check handle initialization */
    {
        return 3;                                                                  /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_CONTROL, &prev, 1);        /* read control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                     /* read control failed */
        
        return 1;                                                                  /* return error */
    }
    prev &= ~(1 << 4);                                                             /* clear config */
    prev |= mode << 4;                                                             /* set setting */
    res = a_ds1307_iic_write(handle, DS1307_REG_CONTROL, prev);                    /* write control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                    /* write control failed */
        
        return 1;                                                                  /* return error */
    }
    
    return 0;                                                                      /* success return 0 */
}

/**
 * @brief      get the output mode
 * @param[in]  *handle pointer to a ds1307 handle structure
 * @param[out] *mode pointer to an output mode buffer
 * @return     status code
 *             - 0 success
 *             - 1 get output mode failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t ds1307_get_output_mode(ds1307_handle_t *handle, ds1307_output_mode_t *mode)
{
    uint8_t res;
    uint8_t prev;
    
    if (handle == NULL)                                                            /* check handle */
    {
        return 2;                                                                  /* return error */
    }
    if (handle->inited != 1)                                                       /* check handle initialization */
    {
        return 3;                                                                  /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_CONTROL, &prev, 1);        /* read control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                     /* read control failed */
        
        return 1;                                                                  /* return error */
    }
    *mode = (ds1307_output_mode_t)((prev >> 4) & 0x01);                            /* set mode */
    
    return 0;                                                                      /* success return 0 */
}

/**
 * @brief     set the square wave frequency
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] freq square wave frequency
 * @return    status code
 *            - 0 success
 *            - 1 set square wave frequency failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t ds1307_set_square_wave_frequency(ds1307_handle_t *handle, ds1307_square_wave_frequency_t freq)
{
    uint8_t res;
    uint8_t prev;
    
    if (handle == NULL)                                                            /* check handle */
    {
        return 2;                                                                  /* return error */
    }
    if (handle->inited != 1)                                                       /* check handle initialization */
    {
        return 3;                                                                  /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_CONTROL, &prev, 1);        /* read control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                     /* read control failed */
        
        return 1;                                                                  /* return error */
    }
    prev &= ~(3 << 0);                                                             /* clear config */
    prev |= freq << 0;                                                             /* set setting */
    res = a_ds1307_iic_write(handle, DS1307_REG_CONTROL, prev);                    /* write control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                    /* write control failed */
        
        return 1;                                                                  /* return error */
    }
    
    return 0;                                                                      /* success return 0 */
}

/**
 * @brief      get the square wave frequency
 * @param[in]  *handle pointer to a ds1307 handle structure
 * @param[out] *freq pointer to a square wave frequency buffer
 * @return     status code
 *             - 0 success
 *             - 1 get square wave frequency failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t ds1307_get_square_wave_frequency(ds1307_handle_t *handle, ds1307_square_wave_frequency_t *freq)
{
    uint8_t res;
    uint8_t prev;
    
    if (handle == NULL)                                                            /* check handle */
    {
        return 2;                                                                  /* return error */
    }
    if (handle->inited != 1)                                                       /* check handle initialization */
    {
        return 3;                                                                  /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, DS1307_REG_CONTROL, &prev, 1);        /* read control */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                     /* read control failed */
        
        return 1;                                                                  /* return error */
    }
    *freq = (ds1307_square_wave_frequency_t)(prev & 0x3);                          /* set freq */
    
    return 0;                                                                      /* success return 0 */
}

/**
 * @brief      read ram
 * @param[in]  *handle pointer to a ds1307 handle structure
 * @param[in]  addr ram address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len buffer length
 * @return     status code
 *             - 0 success
 *             - 1 read ram failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 *             - 4 addr > 55
 *             - 5 len is invalid
 * @note       none
 */
uint8_t ds1307_read_ram(ds1307_handle_t *handle, uint8_t addr, uint8_t *buf, uint8_t len)
{
    uint8_t res;
    
    if (handle == NULL)                                                            /* check handle */
    {
        return 2;                                                                  /* return error */
    }
    if (handle->inited != 1)                                                       /* check handle initialization */
    {
        return 3;                                                                  /* return error */
    }
    if (addr > 55)                                                                 /* check addr */
    {
        handle->debug_print("");                               /* addr > 55 */
        
        return 4;                                                                  /* return error */
    }
    if (addr + len - 1 > 55)                                                       /* check len */
    {
        handle->debug_print("");                          /* len is invalid */
        
        return 5;                                                                  /* return error */
    }
    
    res = a_ds1307_iic_multiple_read(handle, (uint8_t)(DS1307_REG_RAM + addr),
                                     buf, len);                                    /* read ram */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                         /* read ram failed */
        
        return 1;                                                                  /* return error */
    }
    
    return 0;                                                                      /* success return 0 */
}

/**
 * @brief     write ram
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] addr ram address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len buffer length
 * @return    status code
 *            - 0 success
 *            - 1 write ram failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 *            - 4 addr > 55
 *            - 5 len is invalid
 * @note      none
 */
uint8_t ds1307_write_ram(ds1307_handle_t *handle, uint8_t addr, uint8_t *buf, uint8_t len)
{
    uint8_t res;
    
    if (handle == NULL)                                                            /* check handle */
    {
        return 2;                                                                  /* return error */
    }
    if (handle->inited != 1)                                                       /* check handle initialization */
    {
        return 3;                                                                  /* return error */
    }
    if (addr > 55)                                                                 /* check addr */
    {
        handle->debug_print("");                               /* addr > 55 */
        
        return 4;                                                                  /* return error */
    }
    if (addr + len - 1 > 55)                                                       /* check len */
    {
        handle->debug_print("");                          /* len is invalid */
        
        return 5;                                                                  /* return error */
    }
    
    res = a_ds1307_iic_multiple_write(handle, (uint8_t)(DS1307_REG_RAM + addr),
                                      buf, len);                                   /* write ram */
    if (res != 0)                                                                  /* check result */
    {
        handle->debug_print("");                        /* write ram failed */
        
        return 1;                                                                  /* return error */
    }
    
    return 0;                                                                      /* success return 0 */
}

/**
 * @brief     initialize the chip
 * @param[in] *handle pointer to a ds1307 handle structure
 * @return    status code
 *            - 0 success
 *            - 1 iic initialization failed
 *            - 2 handle is NULL
 *            - 3 linked functions is NULL
 * @note      none
 */
uint8_t ds1307_init(ds1307_handle_t *handle)
{
    if (handle == NULL)                                              /* check handle */
    {
        return 2;                                                    /* return error */
    }
    if (handle->debug_print == NULL)                                 /* check debug_print */
    {
        return 3;                                                    /* return error */
    }
    if (handle->iic_init == NULL)                                    /* check iic_init */
    {
        handle->debug_print("");          /* iic_init is null */
       
        return 3;                                                    /* return error */
    }
    if (handle->iic_deinit == NULL)                                  /* check iic_deinit */
    {
        handle->debug_print("");        /* iic_deinit is null */
       
        return 3;                                                    /* return error */
    }
    if (handle->iic_write == NULL)                                   /* check iic_write */
    {
        handle->debug_print("");         /* iic_write is null */
       
        return 3;                                                    /* return error */
    }
    if (handle->iic_read == NULL)                                    /* check iic_read */
    {
        handle->debug_print("");          /* iic_read is null */
       
        return 3;                                                    /* return error */
    }
    if (handle->delay_ms == NULL)                                    /* check delay_ms */
    {
        handle->debug_print("");          /* delay_ms is null */
       
        return 3;                                                    /* return error */
    }
    
    if (handle->iic_init() != 0)                                     /* iic init */
    {
        handle->debug_print("");           /* iic init failed */
       
        return 1;                                                    /* return error */
    }
    handle->inited = 1;                                              /* flag finish initialization */
    
    return 0;                                                        /* success return 0 */
}

/**
 * @brief     close the chip
 * @param[in] *handle pointer to a ds1307 handle structure
 * @return    status code
 *            - 0 success
 *            - 1 iic deinit failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t ds1307_deinit(ds1307_handle_t *handle)
{
    if (handle == NULL)                                             /* check handle */
    {
        return 2;                                                   /* return error */
    }
    if (handle->inited != 1)                                        /* check handle initialization */
    {
        return 3;                                                   /* return error */
    }
    
    if (handle->iic_deinit() != 0)                                  /* iic deinit */
    {
        handle->debug_print("");        /* iic deinit failed */
       
        return 1;                                                   /* return error */
    }
    handle->inited = 0;                                             /* flag close */
    
    return 0;                                                       /* success return 0 */
}

/**
 * @brief     set the chip register
 * @param[in] *handle pointer to a ds1307 handle structure
 * @param[in] reg iic register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len data buffer length
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t ds1307_set_reg(ds1307_handle_t *handle, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (handle == NULL)                                               /* check handle */
    {
        return 2;                                                     /* return error */
    }
    if (handle->inited != 1)                                          /* check handle initialization */
    {
        return 3;                                                     /* return error */
    }
    
    if (handle->iic_write(DS1307_ADDRESS, reg, buf, len) != 0)        /* write data */
    {
        return 1;                                                     /* return error */
    }
    else
    {
        return 0;                                                     /* success return 0 */
    }
}

/**
 * @brief      get the chip register
 * @param[in]  *handle pointer to a ds1307 handle structure
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len data buffer length
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t ds1307_get_reg(ds1307_handle_t *handle, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (handle == NULL)                                              /* check handle */
    {
        return 2;                                                    /* return error */
    }
    if (handle->inited != 1)                                         /* check handle initialization */
    {
        return 3;                                                    /* return error */
    }
    
    if (handle->iic_read(DS1307_ADDRESS, reg, buf, len) != 0)        /* read data */
    {
        return 1;                                                    /* return error */
    }
    else
    {
        return 0;                                                    /* success return 0 */
    }
}

/**
 * @brief      get chip's information
 * @param[out] *info pointer to a ds1307 info structure
 * @return     status code
 *             - 0 success
 *             - 2 handle is NULL
 * @note       none
 */
uint8_t ds1307_info(ds1307_info_t *info)
{
    if (info == NULL)                                               /* check handle */
    {
        return 2;                                                   /* return error */
    }
    
    memset(info, 0, sizeof(ds1307_info_t));                         /* initialize ds1307 info structure */
    strncpy(info->chip_name, CHIP_NAME, 32);                        /* copy chip name */
    strncpy(info->manufacturer_name, MANUFACTURER_NAME, 32);        /* copy manufacturer name */
    strncpy(info->interface, "IIC", 8);                             /* copy interface name */
    info->supply_voltage_min_v = SUPPLY_VOLTAGE_MIN;                /* set minimal supply voltage */
    info->supply_voltage_max_v = SUPPLY_VOLTAGE_MAX;                /* set maximum supply voltage */
    info->max_current_ma = MAX_CURRENT;                             /* set maximum current */
    info->temperature_max = TEMPERATURE_MAX;                        /* set minimal temperature */
    info->temperature_min = TEMPERATURE_MIN;                        /* set maximum temperature */
    info->driver_version = DRIVER_VERSION;                          /* set driver version */
    
    return 0;                                                       /* success return 0 */
}
