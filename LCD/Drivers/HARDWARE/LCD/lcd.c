/**
 * @file        lcd.c
 * @author      陆文豪
 * @version     V2.0
 * @date        2026-04-12
 * @brief       ATK-MD0240 2.4寸 SPI LCD驱动 (替换原FSMC LCD)
 */

#include "lcd.h"
#include "lcdfont.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>

/* SPI句柄 */
static SPI_HandleTypeDef g_spi_handle = {0};

/* LCD参数 */
_lcd_dev lcddev;

/* LCD的画笔颜色和背景色 */
uint32_t g_point_color = 0xF800;    /* 画笔颜色 */
uint32_t g_back_color  = 0xFFFF;    /* 背景色 */

/* LCD缓冲区 */
#define LCD_BUF_SIZE (LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t) / 10)
static uint8_t g_lcd_buf[LCD_BUF_SIZE] = {0};

/**
 * @brief       SPI初始化
 */
static void lcd_spi_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* 使能时钟 */
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 初始化SCK引脚 */
    gpio_init_struct.Pin        = LCD_SPI_SCK_GPIO_PIN;
    gpio_init_struct.Mode       = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull       = GPIO_PULLUP;
    gpio_init_struct.Speed      = GPIO_SPEED_FREQ_HIGH;
    gpio_init_struct.Alternate  = LCD_SPI_SCK_GPIO_AF;
    HAL_GPIO_Init(LCD_SPI_SCK_GPIO_PORT, &gpio_init_struct);

    /* 初始化MOSI引脚 */
    gpio_init_struct.Pin        = LCD_SPI_MOSI_GPIO_PIN;
    gpio_init_struct.Mode       = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull       = GPIO_PULLUP;
    gpio_init_struct.Speed      = GPIO_SPEED_FREQ_HIGH;
    gpio_init_struct.Alternate  = LCD_SPI_MOSI_GPIO_AF;
    HAL_GPIO_Init(LCD_SPI_MOSI_GPIO_PORT, &gpio_init_struct);

    /* 初始化SPI */
    g_spi_handle.Instance               = LCD_SPI_INTERFACE;
    g_spi_handle.Init.Mode              = SPI_MODE_MASTER;
    g_spi_handle.Init.Direction         = SPI_DIRECTION_2LINES;
    g_spi_handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    g_spi_handle.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    g_spi_handle.Init.CLKPhase          = SPI_PHASE_2EDGE;
    g_spi_handle.Init.NSS               = SPI_NSS_SOFT;
    g_spi_handle.Init.BaudRatePrescaler = LCD_SPI_PRESCALER;
    g_spi_handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    g_spi_handle.Init.TIMode            = SPI_TIMODE_DISABLE;
    g_spi_handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    g_spi_handle.Init.CRCPolynomial     = 10;
    HAL_SPI_Init(&g_spi_handle);
}

/**
 * @brief       SPI发送数据
 */
static void lcd_spi_send(uint8_t *buf, uint16_t len)
{
    HAL_SPI_Transmit(&g_spi_handle, buf, len, HAL_MAX_DELAY);
}

/**
 * @brief       硬件初始化
 */
static void lcd_hw_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* 使能时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* 初始化DC引脚 */
    gpio_init_struct.Pin    = LCD_DC_GPIO_PIN;
    gpio_init_struct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull   = GPIO_PULLUP;
    gpio_init_struct.Speed  = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_DC_GPIO_PORT, &gpio_init_struct);

    /* 初始化PWR引脚 */
    gpio_init_struct.Pin    = LCD_PWR_GPIO_PIN;
    gpio_init_struct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull   = GPIO_PULLUP;
    gpio_init_struct.Speed  = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_PWR_GPIO_PORT, &gpio_init_struct);

    /* 初始化CS引脚 */
    gpio_init_struct.Pin    = LCD_CS_GPIO_PIN;
    gpio_init_struct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull   = GPIO_PULLUP;
    gpio_init_struct.Speed  = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);

    /* 初始化RST引脚 */
    gpio_init_struct.Pin    = LCD_RST_GPIO_PIN;
    gpio_init_struct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull   = GPIO_PULLUP;
    gpio_init_struct.Speed  = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_RST_GPIO_PORT, &gpio_init_struct);

    HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_RESET);
}

/**
 * @brief       硬件复位
 */
static void lcd_hw_reset(void)
{
    HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_RESET);
    delay_ms(10);
    HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_SET);
    delay_ms(120);
}

/**
 * @brief       写命令
 */
static void lcd_write_cmd(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_RESET);
    lcd_spi_send(&cmd, sizeof(cmd));
}

/**
 * @brief       写数据
 */
static void lcd_write_dat(uint8_t dat)
{
    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET);
    lcd_spi_send(&dat, sizeof(dat));
}

/**
 * @brief       写16位数据
 */
static void lcd_write_dat_16b(uint16_t dat)
{
    uint8_t buf[2];

    buf[0] = (uint8_t)(dat >> 8) & 0xFF;
    buf[1] = (uint8_t)dat & 0xFF;

    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET);
    lcd_spi_send(buf, sizeof(buf));
}

/**
 * @brief       寄存器初始化
 */
static void lcd_reg_init(void)
{
    /* Sleep Out */
    lcd_write_cmd(0x11);
    delay_ms(120);

    /* Memory Data Access Control */
    lcd_write_cmd(0x36);
    lcd_write_dat(0x00);

    /* RGB 5-6-5-bit */
    lcd_write_cmd(0x3A);
    lcd_write_dat(0x65);

    /* Porch Setting */
    lcd_write_cmd(0xB2);
    lcd_write_dat(0x0C);
    lcd_write_dat(0x0C);
    lcd_write_dat(0x00);
    lcd_write_dat(0x33);
    lcd_write_dat(0x33);

    /* Gate Control */
    lcd_write_cmd(0xB7);
    lcd_write_dat(0x72);

    /* VCOM Setting */
    lcd_write_cmd(0xBB);
    lcd_write_dat(0x3D);

    /* LCM Control */
    lcd_write_cmd(0xC0);
    lcd_write_dat(0x2C);

    /* VDV and VRH Command Enable */
    lcd_write_cmd(0xC2);
    lcd_write_dat(0x01);

    /* VRH Set */
    lcd_write_cmd(0xC3);
    lcd_write_dat(0x19);

    /* VDV Set */
    lcd_write_cmd(0xC4);
    lcd_write_dat(0x20);

    /* Frame Rate Control in Normal Mode */
    lcd_write_cmd(0xC6);
    lcd_write_dat(0x0F);

    /* Power Control 1 */
    lcd_write_cmd(0xD0);
    lcd_write_dat(0xA4);
    lcd_write_dat(0xA1);

    /* Positive Voltage Gamma Control */
    lcd_write_cmd(0xE0);
    lcd_write_dat(0xD0);
    lcd_write_dat(0x04);
    lcd_write_dat(0x0D);
    lcd_write_dat(0x11);
    lcd_write_dat(0x13);
    lcd_write_dat(0x2B);
    lcd_write_dat(0x3F);
    lcd_write_dat(0x54);
    lcd_write_dat(0x4C);
    lcd_write_dat(0x18);
    lcd_write_dat(0x0D);
    lcd_write_dat(0x0B);
    lcd_write_dat(0x1F);
    lcd_write_dat(0x23);

    /* Negative Voltage Gamma Control */
    lcd_write_cmd(0xE1);
    lcd_write_dat(0xD0);
    lcd_write_dat(0x04);
    lcd_write_dat(0x0C);
    lcd_write_dat(0x11);
    lcd_write_dat(0x13);
    lcd_write_dat(0x2C);
    lcd_write_dat(0x3F);
    lcd_write_dat(0x44);
    lcd_write_dat(0x51);
    lcd_write_dat(0x2F);
    lcd_write_dat(0x1F);
    lcd_write_dat(0x1F);
    lcd_write_dat(0x20);
    lcd_write_dat(0x23);

    /* Display Inversion On */
    lcd_write_cmd(0x21);

    /* Display On */
    lcd_write_cmd(0x29);
}

/**
 * @brief       设置显示区域
 */
static void lcd_set_address(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    /* Column Address Set */
    lcd_write_cmd(0x2A);
    lcd_write_dat((uint8_t)(xs >> 8) & 0xFF);
    lcd_write_dat((uint8_t)xs & 0xFF);
    lcd_write_dat((uint8_t)(xe >> 8) & 0xFF);
    lcd_write_dat((uint8_t)xe & 0xFF);

    /* Row Address Set */
    lcd_write_cmd(0x2B);
    lcd_write_dat((uint8_t)(ys >> 8) & 0xFF);
    lcd_write_dat((uint8_t)ys & 0xFF);
    lcd_write_dat((uint8_t)(ye >> 8) & 0xFF);
    lcd_write_dat((uint8_t)ye & 0xFF);

    /* Memory Write */
    lcd_write_cmd(0x2C);
}

/**
 * @brief       LCD初始化
 */
void lcd_init(void)
{
    lcd_spi_init();
    lcd_hw_init();
    lcd_hw_reset();
    lcd_reg_init();

    /* 打开背光 */
    HAL_GPIO_WritePin(LCD_PWR_GPIO_PORT, LCD_PWR_GPIO_PIN, GPIO_PIN_SET);

    /* 设置LCD参数 */
    lcddev.width = LCD_WIDTH;
    lcddev.height = LCD_HEIGHT;
    lcddev.id = 0x7789;  /* ST7789 */
    lcddev.dir = 0;      /* 竖屏 */
    lcddev.wramcmd = 0x2C;
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B;

    lcd_display_dir(0);  /* 默认竖屏 */
    lcd_clear(WHITE);
}

/**
 * @brief       开启显示
 */
void lcd_display_on(void)
{
    lcd_write_cmd(0x29);
}

/**
 * @brief       关闭显示
 */
void lcd_display_off(void)
{
    lcd_write_cmd(0x28);
}

/**
 * @brief       清屏
 */
void lcd_clear(uint16_t color)
{
    uint32_t i;
    uint32_t total_point = LCD_WIDTH * LCD_HEIGHT;

    lcd_set_address(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET);

    for (i = 0; i < total_point; i++)
    {
        lcd_write_dat_16b(color);
    }
}

/**
 * @brief       填充矩形区域
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint16_t i, j;
    uint16_t width = ex - sx + 1;
    uint16_t height = ey - sy + 1;

    lcd_set_address(sx, sy, ex, ey);

    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET);

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            lcd_write_dat_16b(color);
        }
    }
}

/**
 * @brief       画点
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    lcd_set_address(x, y, x, y);
    lcd_write_dat_16b(color);
}

/**
 * @brief       读点 (SPI屏不支持读取，返回0)
 */
uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    return 0;
}

/**
 * @brief       画线
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;

    if (delta_x > 0)incx = 1;
    else if (delta_x == 0)incx = 0;
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)incy = 1;
    else if (delta_y == 0)incy = 0;
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if (delta_x > delta_y)distance = delta_x;
    else distance = delta_y;

    for (t = 0; t <= distance + 1; t++)
    {
        lcd_draw_point(uRow, uCol, color);
        xerr += delta_x ;
        yerr += delta_y ;

        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }

        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}

/**
 * @brief       画矩形
 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

/**
 * @brief       画圆
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1);

    while (a <= b)
    {
        lcd_draw_point(x0 + a, y0 - b, color);
        lcd_draw_point(x0 + b, y0 - a, color);
        lcd_draw_point(x0 + b, y0 + a, color);
        lcd_draw_point(x0 + a, y0 + b, color);
        lcd_draw_point(x0 - a, y0 + b, color);
        lcd_draw_point(x0 - b, y0 + a, color);
        lcd_draw_point(x0 - b, y0 - a, color);
        lcd_draw_point(x0 - a, y0 - b, color);
        a++;

        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

/**
 * @brief       显示一个字符
 */
void lcd_show_char(uint16_t x, uint16_t y, uint8_t chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);
    chr = chr - ' ';

    for (t = 0; t < csize; t++)
    {
        if (size == 12)temp = asc2_1206[chr][t];
        else if (size == 16)temp = asc2_1608[chr][t];
        else if (size == 24)temp = asc2_2412[chr][t];
        else return;

        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)lcd_draw_point(x, y, color);
            else if (mode == 0)lcd_draw_point(x, y, g_back_color);

            temp <<= 1;
            y++;

            if (y >= lcddev.height)return;

            if ((y - y0) == size)
            {
                y = y0;
                x++;

                if (x >= lcddev.width)return;

                break;
            }
        }
    }
}

/**
 * @brief       显示字符串
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;
    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)break;

        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}

/**
 * @brief       计算m^n
 */
static uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n--)result *= m;

    return result;
}

/**
 * @brief       显示数字
 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;

        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2)*t, y, ' ', size, 0, color);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }

        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, 0, color);
    }
}

/**
 * @brief       扩展显示数字
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t, temp;

    for (t = 0; t < len; t++)
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;

        if (mode & 0x80)
        {
            lcd_show_char(x + (size / 2)*t, y, temp + '0', size, mode & 0x01, color);
        }
        else
        {
            if (temp == 0 && t < (len - 1))
            {
                lcd_show_char(x + (size / 2)*t, y, ' ', size, mode & 0x01, color);
            }
            else
            {
                lcd_show_char(x + (size / 2)*t, y, temp + '0', size, mode & 0x01, color);
            }
        }
    }
}

/**
 * @brief       设置LCD显示方向
 */
void lcd_display_dir(uint8_t dir)
{
    lcddev.dir = dir;

    if (dir == 0)
    {
        lcddev.width = LCD_WIDTH;
        lcddev.height = LCD_HEIGHT;
    }
    else
    {
        lcddev.width = LCD_HEIGHT;
        lcddev.height = LCD_WIDTH;
    }

    lcddev.wramcmd = 0x2C;
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B;
}

/**
 * @brief       设置窗口 (兼容接口)
 */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    lcd_set_address(sx, sy, sx + width - 1, sy + height - 1);
}

/**
 * @brief       准备写GRAM (兼容接口)
 */
void lcd_write_ram_prepare(void)
{
    lcd_write_cmd(lcddev.wramcmd);
}

/**
 * @brief       设置光标 (兼容接口)
 */
void lcd_set_cursor(uint16_t x, uint16_t y)
{
    lcd_set_address(x, y, x, y);
}

/**
 * @brief       设置扫描方向 (兼容接口)
 */
void lcd_scan_dir(uint8_t dir)
{
    /* SPI LCD扫描方向通过0x36寄存器设置 */
    /* 这里简化处理，保持默认 */
}

/**
 * @brief       彩色填充 (兼容接口)
 */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint16_t height, width;
    uint16_t i, j;
    width = ex - sx + 1;
    height = ey - sy + 1;

    lcd_set_address(sx, sy, ex, ey);

    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET);

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            lcd_write_dat_16b(color[i * width + j]);
        }
    }
}
