# SPI LCD 迁移方案

## 1. 目的

本文用于把当前工程中的 `FSMC 并口 LCD` 迁移为 `ATK-MD0240 2.4寸 SPI LCD`，并梳理：

- 按 `ATK-MD0240模块SPI接口使用说明.pdf` 的探索者 STM32F407 开发板接线方式
- 与当前项目、后续扩展硬件的冲突情况
- 代码工程的增删改方案

## 2. 依据

本方案综合以下信息整理：

- `ATK-MD0240模块SPI接口使用说明.pdf`
  重点参考 `1.4 正点原子探索者 STM32F407 开发板` 部分
- SPI 示例工程：
  - `D:\BaiduNetdiskDownload\【正点原子】2.4寸TFT LCD液晶屏模块320240\2，程序源码\ATK-MD0240模块测试实验（SPI接口）\探索者STM32F407开发板`
- 当前工程：
  - [项目总结.md](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/项目总结.md)
  - [硬件连接总表与扩展规划.md](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/硬件连接总表与扩展规划.md)
  - [gpio.c](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Core/Src/gpio.c)
  - [fsmc.c](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Core/Src/fsmc.c)
  - [lcd.h](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Drivers/HARDWARE/LCD/lcd.h)
  - [lcd.c](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Drivers/HARDWARE/LCD/lcd.c)

说明：

- 当前环境无法直接提取 PDF 页面文本，但 SPI 示例代码中的引脚定义与说明文档命名完全一致，可作为 1.4 节接线的直接落地依据。

## 3. ATK-MD0240 模块硬件连接方式

### 3.1 模块到探索者 STM32F407 的接线

根据 SPI 示例驱动，ATK-MD0240 在探索者 F407 开发板上的连接如下：

| 模块信号 | MCU 引脚 | 说明 |
|---|---|---|
| SCK | PB3 | `SPI1_SCK` |
| SDA | PB5 | `SPI1_MOSI` |
| WR / DC | PB4 | 数据/命令选择脚 |
| PWR | PG6 | 模块电源/背光控制脚 |
| CS | PG7 | SPI 片选 |
| RST | PG8 | 模块复位 |
| VCC | 3.3V | 电源输入 |
| GND | GND | 公共地 |

对应代码依据：

- [atk_md0240_spi.h](D:/BaiduNetdiskDownload/【正点原子】2.4寸TFT LCD液晶屏模块320240/2，程序源码/ATK-MD0240模块测试实验（SPI接口）/探索者STM32F407开发板/Drivers/BSP/ATK_MD0240/atk_md0240_spi.h)
- [atk_md0240.h](D:/BaiduNetdiskDownload/【正点原子】2.4寸TFT LCD液晶屏模块320240/2，程序源码/ATK-MD0240模块测试实验（SPI接口）/探索者STM32F407开发板/Drivers/BSP/ATK_MD0240/atk_md0240.h)

具体定义：

- `SPI1`：`PB3 / PB5`
- `WR/DC`：`PB4`
- `PWR`：`PG6`
- `CS`：`PG7`
- `RST`：`PG8`

### 3.2 这个 SPI 屏的接口特点

- 它不是当前工程这种 `FSMC 16bit 并口屏`
- 它使用 `SPI1 + 3 根普通 GPIO 控制脚`
- 数据刷屏性能会低于并口 FSMC LCD
- 但会释放大量 FSMC 引脚，后续扩展更灵活

### 3.3 需要额外注意的板级点

- `PB3/PB4` 在 STM32F407 上默认与 JTAG 相关功能复用
- 工程把这两个引脚配置为普通 GPIO / SPI 复用后，通常仍可保留 `SWD` 下载调试
- 但不应再依赖完整 `JTAG` 五线调试模式

## 4. 与当前工程的硬件冲突分析

### 4.1 当前工程已占用资源

当前真实占用如下：

| 模块 | 引脚 |
|---|---|
| MPU6050 | `PB6 / PB7` |
| PMS7003 | `PA9 / PA10` |
| ESP8266 | `PB10 / PB11` |
| 蜂鸣器 | `PF8` |
| KEY_UP | `PA0` |
| KEY0/1/2 | `PE4 / PE3 / PE2` |
| LED1/LED2 | `PF9 / PF10` |
| 当前 LCD | `FSMC + PB15` |

当前 LCD 的 FSMC 占用：

- `PD14 / PD15 / PD0 / PD1`
- `PD4 / PD5`
- `PD8 / PD9 / PD10`
- `PE7 ~ PE15`
- `PF12`
- `PG12`
- `PB15`

### 4.2 SPI LCD 是否和当前模块冲突

SPI LCD 新占用：

- `PB3`
- `PB4`
- `PB5`
- `PG6`
- `PG7`
- `PG8`

结论：

- 与 `MPU6050(PB6/PB7)` 不冲突
- 与 `PMS7003(PA9/PA10)` 不冲突
- 与 `ESP8266(PB10/PB11)` 不冲突
- 与 `蜂鸣器(PF8)` 不冲突
- 与 `按键/LED` 不冲突
- 与当前建议扩展的 `DS18B20(PG9 / PG10)` 不冲突
- 与当前建议扩展的 `GPS(PC6 / PC7)` 不冲突
- 与当前建议扩展的 `G5516(PC0)` 不冲突
- 与当前建议扩展的 `LR7843(PC1)` 不冲突

### 4.3 替换后的资源变化

替换为 SPI LCD 后：

新增占用：

- `PB3 / PB4 / PB5 / PG6 / PG7 / PG8`

释放资源：

- `PD0 / PD1 / PD4 / PD5 / PD8 / PD9 / PD10 / PD14 / PD15`
- `PE7 ~ PE15`
- `PF12`
- `PG12`
- `PB15`

结论：

- 从硬件资源角度，迁移到 SPI LCD 是可行的
- 对你当前已接模块和已规划扩展模块没有直接冲突
- 迁移后 IO 余量比现在更大

## 5. 现有工程迁移到 SPI LCD 的代码工程方案

## 5.1 总体策略

不建议在当前 FSMC LCD 驱动文件上直接“硬改成 SPI”。

更稳妥的方式是：

1. 保留现有业务层 `main.c` 的显示调用接口
2. 新增一套 `SPI LCD 驱动层`
3. 逐步替换底层实现
4. 最后移除 FSMC 相关文件和配置

原因：

- 现有 `main.c` 已大量调用 `lcd_init()`、`lcd_show_string()`、`lcd_clear()` 等通用接口
- 如果新 SPI 屏驱动能继续提供同名接口，则上层业务代码改动最小

## 5.2 建议的文件处理方式

### 新增

建议新增目录：

- `Drivers/HARDWARE/LCD_SPI/`

建议新增文件：

- `lcd_spi_port.h`
- `lcd_spi_port.c`
- `lcd_spi_drv.h`
- `lcd_spi_drv.c`

也可以直接复用正点原子示例中的：

- `atk_md0240.h`
- `atk_md0240.c`
- `atk_md0240_spi.h`
- `atk_md0240_spi.c`

但更建议做一次本工程命名统一，避免后续维护时出现两套风格混杂。

### 删除

迁移稳定后再删除：

- [fsmc.c](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Core/Src/fsmc.c)
- [fsmc.h](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Core/Inc/fsmc.h)
- [lcd.c](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Drivers/HARDWARE/LCD/lcd.c)
- [lcd.h](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Drivers/HARDWARE/LCD/lcd.h)
- `lcd_ex.c`
- `lcdfont.h` 是否删除取决于新驱动是否继续复用当前字库

### 修改

需要修改的关键文件：

- [gpio.c](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Core/Src/gpio.c)
- [main.h](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Core/Inc/main.h)
- [main.c](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/Core/Src/main.c)
- [LCD.uvprojx](D:/bysj/SmartHelmet/SmartHelmetByLwh/LCD/MDK-ARM/LCD.uvprojx)
- 可能还包括 `stm32f4xx_hal_conf.h`

## 5.3 GPIO 和外设初始化修改方案

### 当前需要移除的内容

从当前工程移除：

- `MX_FSMC_Init()` 调用
- FSMC 相关 GPIO 复用初始化
- `PB15` LCD 背光输出用途

### 新增的 GPIO / SPI 初始化

需要在 `gpio.c` / 新驱动中新增：

- `PB3` -> SPI1 SCK 复用推挽输出
- `PB5` -> SPI1 MOSI 复用推挽输出
- `PB4` -> 普通推挽输出，作为 `DC/WR`
- `PG6` -> 普通推挽输出，作为 `PWR`
- `PG7` -> 普通推挽输出，作为 `CS`
- `PG8` -> 普通推挽输出，作为 `RST`

同时新增：

- `SPI1` 时钟使能
- `GPIOB`、`GPIOG` 时钟使能

SPI 参数建议直接对齐正点原子示例：

- `SPI1`
- 主机模式
- 8bit 数据宽度
- `CPOL = High`
- `CPHA = 2Edge`
- 软 NSS
- 预分频初始可用 `SPI_BAUDRATEPRESCALER_2`

## 5.4 显示驱动接口迁移方案

### 方案 A：保留现有 `lcd_*` 接口名

这是最推荐的方案。

做法：

- 新 SPI 驱动继续导出以下接口：
  - `lcd_init()`
  - `lcd_clear()`
  - `lcd_show_string()`
  - `lcd_show_num()`
  - `lcd_fill()`
  - `lcd_draw_point()`
  - 其他 `main.c` 已实际使用的接口

优点：

- `main.c` 几乎不用改
- 上层业务显示逻辑可直接保留

缺点：

- 需要把正点原子 `ATK_MD0240` 驱动包装成你当前项目的 `lcd_*` 风格

### 方案 B：业务层全部改为 `atk_md0240_*`

做法：

- 把当前 `main.c` 中所有 `lcd_*` 调用改成 `atk_md0240_*`

优点：

- 直接复用官方示例驱动，底层改动更少

缺点：

- `main.c` 改动会比较多
- 以后如果再换屏，业务层耦合更重

结论：

- 推荐采用 `方案 A`

## 5.5 字库和绘图函数建议

现有工程依赖：

- `lcd_show_string()`
- `lcd_show_num()`
- 基础绘图/清屏

建议：

- 如果 SPI LCD 官方驱动已带显示字符串和数字接口，可优先直接复用
- 如果接口不完全匹配，可以保留现有 `lcdfont.h`，只把底层“画点/区域填充”改成 SPI 发送

这样迁移风险最小。

## 5.6 Keil 工程修改方案

需要在 `LCD.uvprojx` 中：

### 删除或移出编译组

- `Core/Src/fsmc.c`
- `Drivers/HARDWARE/LCD/lcd.c`

### 新增到编译组

- `Drivers/HARDWARE/LCD_SPI/*.c`

如果直接复用正点原子文件，则新增：

- `atk_md0240.c`
- `atk_md0240_spi.c`

并补充头文件包含路径。

### HAL 配置

确认 `stm32f4xx_hal_conf.h` 中：

- `HAL_SPI_MODULE_ENABLED` 已开启

当前工程里 SPI 默认是注释掉的，迁移时需要打开。

## 5.7 main.c 改动建议

### 必改项

- 移除 `MX_FSMC_Init()`
- 改为 `MX_SPI1_Init()` 或由 `lcd_spi_init()` 内部完成 SPI 初始化
- 保持 `lcd_init()` 在系统初始化阶段调用

### 可保持不变的项

如果采用“保留 `lcd_*` 接口”的方案，则以下显示逻辑可基本保持不变：

- 标题显示
- 传感器数据显示
- WiFi/Cloud 状态显示
- 报警状态显示

## 6. 推荐实施顺序

建议按下面顺序做，不要一步到位硬切：

1. 先确认 SPI 屏裸接线点亮
2. 在独立最小工程中完成 `lcd_init + clear + show_string`
3. 把 SPI LCD 驱动移植进当前项目
4. 保留原 `main.c` 显示逻辑，先跑通基本界面
5. 再删除 FSMC 相关代码
6. 最后更新项目文档和总连接表

## 7. 最终结论

### 硬件上

ATK-MD0240 SPI 屏在探索者 F407 上建议按以下方式连接：

- `PB3` -> `SCK`
- `PB5` -> `SDA/MOSI`
- `PB4` -> `WR/DC`
- `PG6` -> `PWR`
- `PG7` -> `CS`
- `PG8` -> `RST`
- `VCC` -> `3.3V`
- `GND` -> 地

### 对当前项目的影响

- 不会与当前已使用的 `MPU6050 / PMS7003 / ESP8266 / 蜂鸣器 / 按键 / LED` 冲突
- 不会与计划扩展的 `DS18B20(PG9 / PG10)`、`GPS(PC6 / PC7)`、`G5516(PC0)`、`LR7843(PC1)` 冲突
- 会释放大量 FSMC 引脚资源

### 软件上

- 不是简单改几根线
- 本质上是把 `FSMC内存映射LCD驱动` 换成 `SPI串行LCD驱动`
- 推荐保留现有 `lcd_*` 上层接口，只重写底层驱动实现
