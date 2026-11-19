# 00 环境搭建

> **本篇目标**：两套工具链都装好，能编译 MSPM0 工程，能让 K230 打印一句话。
> **验收标准**：CCS 编译出 `Debug/aim_car.out`；CanMV IDE 终端输出 `CanMV v1.2.2 ...`。

## 你需要准备

### 硬件

| 物品 | 说明 |
| --- | --- |
| 电脑 | Windows 10/11，至少 2 个可用 USB 口 |
| LCKFB-TMX-MSPM0G3507 开发板 | 立创扩展底板 + U8 核心模块 |
| XDS110 调试器 | 烧录 MSPM0 用；若你的底板已板载调试器可跳过 |
| CanMV-K230-LP4 V3.0 | 视觉模块 |
| USB 数据线 ×2 | 必须是数据线，只能充电的线会导致「找不到串口」 |

### 软件

| 软件 | 版本 | 用途 |
| --- | --- | --- |
| CCS Theia | 21.0.1 | MSPM0 开发、编译、烧录 |
| MSPM0 SDK | 2.11.00.07 | 外设驱动库 |
| CanMV IDE | K230 版 | K230 脚本编辑与运行 |

> 本项目所有引脚、外设都在 SysConfig 里图形化配置，代码只引用生成的宏。所以 CCS 和 SDK 的版本尽量与本项目一致，避免生成文件差异。

## 步骤 1：安装 CCS Theia

1. 打开 TI 官网 CCS 下载页，选择 **CCS Theia**（不是老版 Eclipse CCS）。
2. 安装时勾选 **MSPM0 SDK 2.11.00.07**，其余保持默认。
3. 安装完成后打开，`Help → About` 确认版本号。

## 步骤 2：导入并编译工程

1. `File → Import → CCS Projects`
2. `Select search-directory` 选择本仓库根目录（`aim_car/`）
3. 勾选 `aim_car`，点击 `Finish`
4. `Project → Build Project`

**验收**：终端最后出现 `Build Finished`，`Debug/` 下生成 `aim_car.out`。

看不到工程？确认选的是仓库根目录，而不是 `Debug/` 或 `vision/`。

## 步骤 3：烧录验证

1. 用 USB 连接 XDS110 与开发板（SWDIO / SWCLK / GND）
2. `Run → Debug`，程序下载后会自动停在 `main`

**验收**：OLED 上电后先**全白 1 秒**，再**全黑**。这说明程序真的跑起来了，不只是烧进去。

烧录报 `-1001` / `-2064`？先检查 SWD 四根线序和目标板供电，再看专门的烧录排查篇。

## 步骤 4：连接 K230

1. 安装 **CanMV IDE**（K230 版）。
2. K230 用 USB 连接电脑，等待系统识别出串口。
3. 打开 CanMV IDE，左下角选择对应串口，点击连接。

**验收**：终端打印类似：

```
CanMV v1.2.2(based on Micropython e00a144) on 2025-06-16; k230_canmv_v3p0 with K230
```

## 步骤 5：跑通第一段 K230 脚本

新建文件，输入：

```python
import os
print("hello k230")
os.exitpoint()
```

点击绿色三角运行。

**验收**：终端输出 `hello k230`。

## 验收清单

| # | 检查项 | 通过标准 |
| --- | --- | --- |
| 1 | CCS 版本 | `Help → About` 显示 21.0.1 |
| 2 | SDK | Products 里勾选了 MSPM0 SDK 2.11.00.07 |
| 3 | 编译 | `Build Finished`，生成 `Debug/aim_car.out` |
| 4 | 烧录 | OLED 全白 1 秒后全黑 |
| 5 | K230 连接 | CanMV IDE 终端显示版本号 |
| 6 | K230 脚本 | 输出 `hello k230` |

六项全部通过，再进入下一篇。

## 常见问题

| 现象 | 处理 |
| --- | --- |
| Import 里看不到工程 | 目录选的是仓库根目录，不是子目录 |
| Build 报找不到 `ti_msp_dl_config.h` | SysConfig 还没生成，重新保存 `empty_mspm0g3507.syscfg` 后重编 |
| Debug 报 -1001 / -2064 | SWD 接线或供电问题，逐根检查 SWDIO / SWCLK / GND |
| CanMV IDE 找不到串口 | 换数据线；在设备管理器看是否出现 COM 口，缺驱动则装 CH340 / CP210x |
| K230 终端无输出 | 串口选错，或串口被其他软件占用 |

## 下一步

01 点灯 —— 让板载 LED 闪起来，验证 GPIO 通路。
