/**
 * @file        lcd.h
 * @author      陆文豪
 * @version     V2.0
 * @date        2026-04-12
 * @brief       ATK-MD0240 2.4寸 SPI LCD驱动 (替换原FSMC LCD)
 */

#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include "stdlib.h"

/* SPI LCD 硬件连接定义 (ATK-MD0240) */
#define LCD_SPI_INTERFACE                SPI1
#define LCD_SPI_PRESCALER                SPI_BAUDRATEPRESCALER_2

/* SPI引脚定义 */
#define LCD_SPI_SCK_GPIO_PORT            GPIOB
#define LCD_SPI_SCK_GPIO_PIN             GPIO_PIN_3
#define LCD_SPI_SCK_GPIO_AF              GPIO_AF5_SPI1

#define LCD_SPI_MOSI_GPIO_PORT           GPIOB
#define LCD_SPI_MOSI_GPIO_PIN            GPIO_PIN_5
#define LCD_SPI_MOSI_GPIO_AF             GPIO_AF5_SPI1

/* 控制引脚定义 */
#define LCD_DC_GPIO_PORT                 GPIOB
#define LCD_DC_GPIO_PIN                  GPIO_PIN_4

#define LCD_PWR_GPIO_PORT                GPIOG
#define LCD_PWR_GPIO_PIN                 GPIO_PIN_6

#define LCD_CS_GPIO_PORT                 GPIOG
#define LCD_CS_GPIO_PIN                  GPIO_PIN_7

#define LCD_RST_GPIO_PORT                GPIOG
#define LCD_RST_GPIO_PIN                 GPIO_PIN_8

/* LCD尺寸定义 */
#define LCD_WIDTH                        240
#define LCD_HEIGHT                       320

/* LCD重要参数集 */
typedef struct
{
    uint16_t width;     /* LCD 宽度 */
    uint16_t height;    /* LCD 高度 */
    uint16_t id;        /* LCD ID */
    uint8_t dir;        /* 横屏还是竖屏控制：0，竖屏；1，横屏。 */
    uint16_t wramcmd;   /* 开始写gram指令 */
    uint16_t setxcmd;   /* 设置x坐标指令 */
    uint16_t setycmd;   /* 设置y坐标指令 */
} _lcd_dev;

/* LCD参数 */
extern _lcd_dev lcddev;

/* LCD的画笔颜色和背景色 */
extern uint32_t g_point_color;
extern uint32_t g_back_color;

/* 画笔颜色 */
#define WHITE            0xFFFF     /* 白色 */
#define BLACK            0x0000     /* 黑色 */
#define RED              0xF800     /* 红色 */
#define GREEN            0x07E0     /* 绿色 */
#define BLUE             0x001F     /* 蓝色 */
#define MAGENTA          0xF81F     /* 品红色/紫红色 = BLUE + RED */
#define YELLOW           0xFFE0     /* 黄色 = GREEN + RED */
#define CYAN             0x07FF     /* 青色 = GREEN + BLUE */

#define BROWN            0xBC40     /* 棕色 */
#define BRRED            0xFC07     /* 棕红色 */
#define GRAY             0x8430     /* 灰色 */
#define DARKBLUE         0x01CF     /* 深蓝色 */
#define LIGHTBLUE        0x7D7C     /* 浅蓝色 */
#define GRAYBLUE         0x5458     /* 灰蓝色 */
#define LIGHTGREEN       0x841F     /* 浅绿色 */
#define LGRAY            0xC618     /* 浅灰色(PANNEL),窗体背景色 */
#define LGRAYBLUE        0xA651     /* 浅灰蓝色(中间层颜色) */
#define LBBLUE           0x2B12     /* 浅棕蓝色(选择条目的反色) */

/* 扫描方向定义 */
#define L2R_U2D  0          /* 从左到右,从上到下 */
#define L2R_D2U  1          /* 从左到右,从下到上 */
#define R2L_U2D  2          /* 从右到左,从上到下 */
#define R2L_D2U  3          /* 从右到左,从下到上 */

#define U2D_L2R  4          /* 从上到下,从左到右 */
#define U2D_R2L  5          /* 从上到下,从右到左 */
#define D2U_L2R  6          /* 从下到上,从左到右 */
#define D2U_R2L  7          /* 从下到上,从右到左 */

#define DFT_SCAN_DIR  L2R_U2D   /* 默认的扫描方向 */

/* 函数声明 */
void lcd_init(void);
void lcd_display_on(void);
void lcd_display_off(void);
void lcd_scan_dir(uint8_t dir);
void lcd_display_dir(uint8_t dir);
void lcd_set_cursor(uint16_t x, uint16_t y);

void lcd_write_ram_prepare(void);
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);
uint32_t lcd_read_point(uint16_t x, uint16_t y);

void lcd_clear(uint16_t color);
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);

void lcd_show_char(uint16_t x, uint16_t y, uint8_t chr, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);

#endif
