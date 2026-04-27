# SPI LCD 迁移记录

## 1. 文档性质

本文原本用于规划 `FSMC 并口 LCD` 迁移到 `ATK-MD0240 2.4 寸 SPI LCD`。按 2026-04-27 源码核对结果，当前迁移已经落地：

- `Drivers/HARDWARE/LCD/lcd.c` 已是 SPI LCD 驱动。
- `lcd_init()` 内部初始化 `SPI1`、`PB3/PB5` 和 `PB4/PG6/PG7/PG8` 控制脚。
- `main.c` 未调用 `MX_FSMC_Init()`。
- `MDK-ARM/LCD.uvprojx` 当前加入的是 `Drivers/HARDWARE/LCD/lcd.c`，未加入 `Core/Src/fsmc.c`。

因此本文作为历史迁移记录和后续维护说明保留，不再作为“待实施方案”。

## 2. 当前 SPI LCD 接线

| ATK-MD0240 信号 | MCU 引脚 | 说明 |
|---|---|---|
| `SCK` | `PB3` | `SPI1_SCK` |
| `SDA/MOSI` | `PB5` | `SPI1_MOSI` |
| `WR/DC` | `PB4` | 数据/命令选择 |
| `PWR` | `PG6` | 电源/背光控制 |
| `CS` | `PG7` | SPI 片选 |
| `RST` | `PG8` | LCD 复位 |
| `VCC` | `3.3V` | 电源 |
| `GND` | `GND` | 公共地 |

## 3. 当前软件实现

| 项目 | 当前实现 |
|---|---|
| 驱动目录 | `Drivers/HARDWARE/LCD/` |
| 主要文件 | `lcd.h`、`lcd.c`、`lcdfont.h` |
| 上层接口 | 继续提供 `lcd_init()`、`lcd_clear()`、`lcd_show_string()` 等 `lcd_*` 接口 |
| SPI 初始化 | `lcd.c` 内部静态函数 `lcd_spi_init()` |
| GPIO 初始化 | `lcd.c` 内部静态函数 `lcd_hw_init()` |
| 屏幕尺寸 | `240 x 320` |
| LCD 控制器标注 | `lcddev.id = 0x7789` |
| 旧 FSMC 备份 | `lcd_fsmc_backup.c/.h` 文件仍存在，但不是当前主驱动 |

## 4. 与旧 FSMC 方案的差异

| 项目 | 旧 FSMC LCD | 当前 SPI LCD |
|---|---|---|
| 总线 | FSMC 并口 | SPI1 |
| 数据脚数量 | 占用大量 D/E/F/G 口 | 主要占用 `PB3/PB5` |
| 控制脚 | 含 `PB15` 等 | `PB4/PG6/PG7/PG8` |
| 刷屏速度 | 较快 | 较慢 |
| IO 余量 | 少 | 多 |
| 当前状态 | 历史方案 | 已落地 |

当前 SPI 方案释放的旧资源：

- `PD0 / PD1 / PD4 / PD5 / PD8 / PD9 / PD10 / PD14 / PD15`
- `PE7 ~ PE15`
- `PF12`
- `PG12`
- `PB15`

## 5. 当前工程配置核对

| 核对项 | 当前情况 |
|---|---|
| `HAL_SPI_MODULE_ENABLED` | 已在 `Core/Inc/stm32f4xx_hal_conf.h` 中启用 |
| `HAL_UART_MODULE_ENABLED` | 已启用，用于 PMS7003/ESP8266 |
| `HAL_SRAM_MODULE_ENABLED` | 注释状态，符合不再使用 FSMC SRAM/LCD 的现状 |
| `Core/Src/fsmc.c` | 文件存在，但未加入当前 Keil 工程 |
| `Drivers/HARDWARE/LCD/lcd_ex.c` | 文件存在，当前工程未直接加入 |
| `Drivers/HARDWARE/LCD/lcd_fsmc_backup.*` | 历史备份，当前工程未加入 |

## 6. 与其他模块冲突情况

当前 SPI LCD 占用 `PB3/PB4/PB5/PG6/PG7/PG8`，与以下已用或预留模块不冲突：

| 模块 | 引脚 | 冲突情况 |
|---|---|---|
| MPU6050 | `PB6/PB7` | 不冲突 |
| PMS7003 | `PA9/PA10` | 不冲突 |
| ESP8266 | `PB10/PB11` | 不冲突 |
| G5516 | `PC0` | 不冲突 |
| LR7843 | `PC1` | 不冲突 |
| 蜂鸣器 | `PF8` | 不冲突 |
| 按键 | `PA0/PE2/PE3/PE4` | 不冲突 |
| LED | `PF9/PF10` | 不冲突 |
| GPS 预留 | `PC6/PC7` | 不冲突 |
| DS18B20 预留 | `PG9/PG10` | 不冲突 |

注意：`PB3/PB4` 与完整 JTAG 功能复用相关。当前方案通常保留 SWD 下载调试，不应再依赖完整 JTAG 五线模式。

## 7. 后续维护建议

1. 保持上层 `lcd_*` 接口稳定，减少 `main.c` 页面逻辑改动。
2. 若后续清理历史文件，优先确认并处理：
   - `Core/Src/fsmc.c`
   - `Core/Inc/fsmc.h`
   - `Drivers/HARDWARE/LCD/lcd_fsmc_backup.*`
3. 新增 GPS、DS18B20 或更多界面字段前，先重新设计 240x320 页面布局。
4. 若刷新速度不足，可优化 `lcd_clear()` 和大面积填充，减少逐点 SPI 发送。

## 8. 已知不一致标注

| 原说法 | 当前事实 | 处理 |
|---|---|---|
| “新增 SPI LCD 驱动并替换旧 LCD 调用链” | 已完成 | 改为历史记录 |
| “移除 `MX_FSMC_Init()` 调用” | 当前 `main.c` 已无该调用 | 标为已完成 |
| “确认 `HAL_SPI_MODULE_ENABLED`” | 当前已启用 | 标为已完成 |
| “删除 FSMC 文件” | 文件仍存在但未加入工程 | 标为历史备份/待清理 |

## 9. 本次文档更新记录

| 日期 | 更新内容 |
|---|---|
| 2026-04-27 | 仅更新文档；将本文从“迁移方案”改为“迁移记录”，标注 SPI LCD 已落地，并补充当前工程配置、旧 FSMC 资源释放和历史文件待清理说明。 |
