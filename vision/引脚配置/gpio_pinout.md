# GPIO 引脚定义（文字版）

- 板卡：CoreM V1 R2930-LP4-8E
- 数据来源：40-Pin 排针定义图 + 芯片 GPIO0–63 引脚复用表
- 配套交互版：`gpio_pinout.html`

---

## 一、40-Pin 排针引脚定义（按引脚号排列）

| PIN | 名称 | 功能分类 | GPIO | PWM | 芯片侧其他复用功能 |
|----:|------|----------|------|-----|--------------------|
| 1  | 5V         | 电源 | —      | —    | — |
| 2  | GND        | 地   | —      | —    | — |
| 3  | 3V3        | 电源 | —      | —    | — |
| 4  | GND        | 地   | —      | —    | — |
| 5  | IO20       | GPIO | GPIO20 | —    | OSPI_D4 / QSPI1_CS0 / PULSE_CNTR0 |
| 6  | OSPI_D3    | OSPI | GPIO19 | —    | QSPI1_CS1 / QSPI0_D3 |
| 7  | OSPI_D0    | OSPI | GPIO16 | —    | QSPI1_CS4 / QSPI0_D0 |
| 8  | OSPI_CLK   | OSPI | GPIO15 | —    | QSPI0_CLK |
| 9  | IO42       | GPIO | GPIO42 | PWM0 | UART1_RTS / QSPI1_D2 |
| 10 | IIC3_SCL   | I2C  | GPIO4  | —    | （见注意事项 2） |
| 11 | JTAG_TCK   | JTAG | GPIO2  | —    | PULSE_CNTR0 |
| 12 | IIC3_SDA   | I2C  | GPIO45 | —    | UART2_RXD / SPI2AXI_CS |
| 13 | JTAG_TDO   | JTAG | GPIO4  | —    | PULSE_CNTR2 / UART1_RXD |
| 14 | IIS_D_OUT0 | I2S  | GPIO35 | —    | IIC1_SDA / PDM_IN1 / UART3_CTS |
| 15 | IO27       | GPIO | GPIO27 | —    | MMC1_CMD / PULSE_CNTR5 / PDM_IN0 |
| 16 | M_CLK0     | MCLK | GPIO63 | —    | （芯片表 GPIO63 为 M_CLK3 / UART3_RE） |
| 17 | UART2_TXD  | UART | GPIO5  | —    | JTAG_TMS / PULSE_CNTR3 |
| 18 | JTAG_TDI   | JTAG | GPIO3  | —    | PULSE_CNTR1 / UART1_TXD |
| 19 | PDM_CLK    | PDM  | GPIO26 | —    | MMC1_CLK |
| 20 | UART2_RXD  | UART | GPIO6  | —    | JTAG_RST / PULSE_CNTR4 |
| 21 | IO30       | GPIO | GPIO30 | —    | MMC1_D2 / UART3_RTS / CTRL_O1_3D |
| 22 | IO28       | GPIO | GPIO28 | —    | MMC1_D0 / UART3_TXD / PDM_IN1 |
| 23 | IO31       | GPIO | GPIO31 | —    | MMC1_D3 / UART3_CTS / CTRL_O2_3D |
| 24 | IO29       | GPIO | GPIO29 | —    | MMC1_D1 / UART3_RXD / CTRL_IN_3D |
| 25 | IIS_CLK    | I2S  | GPIO32 | —    | IIC0_SCL / UART3_TXD |
| 26 | IIS_D_IN0  | I2S  | GPIO34 | —    | IIC1_SCL / PDM_IN3 / UART3_RTS |
| 27 | M_CLK2     | MCLK | GPIO62 | —    | UART3_DE |
| 28 | IIS_WS     | I2S  | GPIO33 | —    | IIC0_SDA / UART3_RXD |
| 29 | IO52       | GPIO | GPIO52 | PWM4 | UART3_RTS / IIC3_SCL |
| 30 | IO53       | GPIO | GPIO53 | PWM5 | UART3_CTS / IIC3_SDA |
| 31 | OSPI_D2    | OSPI | GPIO18 | —    | QSPI1_CS2 / QSPI0_D2 |
| 32 | OSPI_D1    | OSPI | GPIO17 | —    | QSPI1_CS3 / QSPI0_D1 |
| 33 | IIC4_SCL   | I2C  | GPIO46 | PWM2 | UART2_RTS |
| 34 | OSPI_CS    | OSPI | GPIO14 | —    | QSPI0_CS0 |
| 35 | IO43       | GPIO | GPIO43 | PWM1 | UART1_CTS / QSPI1_D3 |
| 36 | IIC4_SDA   | I2C  | GPIO47 | PWM3 | UART2_CTS |
| 37 | GND        | 地   | —      | —    | — |
| 38 | ADC1       | ADC  | ADC1   | —    | — |
| 39 | ADC2       | ADC  | ADC2   | —    | — |
| 40 | ADC0       | ADC  | ADC0   | —    | — |

> 排针物理排列：上排 = 偶数引脚（2/4/…/40），下排 = 奇数引脚（1/3/…/39），引脚 1 为方形焊盘。

---

## 二、按功能分类汇总

### 1. 电源与地
- **5V**：PIN 1
- **3V3**：PIN 3
- **GND**：PIN 2、PIN 4、PIN 37

### 2. PWM（6 路全部可从排针使用）
| PWM 通道 | 排针位置 | 排针 GPIO | 其他候选 GPIO（未引出排针） |
|----------|----------|-----------|------------------------------|
| PWM0 | PIN 9  | GPIO42 | GPIO54、GPIO60 |
| PWM1 | PIN 35 | GPIO43 | GPIO55、GPIO61 |
| PWM2 | PIN 33 | GPIO46 | GPIO7、GPIO56  |
| PWM3 | PIN 36 | GPIO47 | GPIO8、GPIO57  |
| PWM4 | PIN 29 | GPIO52 | GPIO9、GPIO58  |
| PWM5 | PIN 30 | GPIO53 | GPIO25、GPIO59 |

### 3. UART 串口
| 接口 | TXD | RXD | RTS | CTS/DE/RE |
|------|-----|-----|-----|-----------|
| UART0 | GPIO38（未引出） | GPIO39（未引出） | — | — |
| UART1 | GPIO3（PIN 18）/ GPIO40 | GPIO4（PIN 13）/ GPIO41 | GPIO42（PIN 9） | GPIO43（PIN 35） |
| UART2 | GPIO5（PIN 17）/ GPIO44 | GPIO6（PIN 20）/ GPIO45 | GPIO46（PIN 33） | GPIO47（PIN 36） |
| UART3 | GPIO28（PIN 22）/ GPIO32（PIN 25）/ GPIO50 | GPIO29（PIN 24）/ GPIO33（PIN 28）/ GPIO51 | GPIO30（PIN 21）/ GPIO52（PIN 29） | GPIO31（PIN 23）/ GPIO53（PIN 30）；DE=GPIO62（PIN 27）、RE=GPIO63（PIN 16） |
| UART4 | GPIO36、GPIO48（均未引出） | GPIO37、GPIO49（均未引出） | — | — |

### 4. I2C 总线
| 控制器 | SCL | SDA |
|--------|-----|-----|
| IIC0 | GPIO32（PIN 25）、GPIO48 | GPIO33（PIN 28）、GPIO49 |
| IIC1 | GPIO34（PIN 26）、GPIO40 | GPIO35（PIN 14）、GPIO41 |
| IIC2 | GPIO11、GPIO50（均未引出） | GPIO12、GPIO51（均未引出） |
| IIC3 | GPIO36、GPIO44、GPIO52（PIN 29）；排针 PIN 10 标注 IO4 | GPIO37、GPIO45（PIN 12）、GPIO53（PIN 30） |
| IIC4 | GPIO7、GPIO46（PIN 33） | GPIO8、GPIO47（PIN 36） |

### 5. OSPI / QSPI 存储总线（排针引出情况）
- **OSPI_CS** = PIN 34（GPIO14）
- **OSPI_CLK** = PIN 8（GPIO15）
- **OSPI_D0** = PIN 7（GPIO16）、**D1** = PIN 32（GPIO17）、**D2** = PIN 31（GPIO18）、**D3** = PIN 6（GPIO19）、**D4** = PIN 5（GPIO20）
- D5–D7 / DQS（GPIO21–24）未引出到排针

### 6. JTAG 调试
- **TCK** = PIN 11（GPIO2）
- **TDI** = PIN 18（GPIO3）
- **TDO** = PIN 13（GPIO4）
- **TMS** = GPIO5（PIN 17，与 UART2_TXD 复用）
- **RST** = GPIO6（PIN 20，与 UART2_RXD 复用）

### 7. I2S / 音频（IIS）
- **IIS_CLK**（位时钟）= PIN 25（GPIO32）
- **IIS_WS**（左右声道）= PIN 28（GPIO33）
- **IIS_D_IN0**（录音）= PIN 26（GPIO34）
- **IIS_D_OUT0**（放音）= PIN 14（GPIO35）
- **M_CLK** 主时钟：PIN 27（GPIO62 / M_CLK2）、PIN 16（GPIO63）
- **PDM**：CLK = PIN 19（GPIO26）；IN0 = PIN 15（GPIO27）、IN1 = PIN 22（GPIO28）/ PIN 14（GPIO35）、IN2 = GPIO36、IN3 = PIN 26（GPIO34）

### 8. ADC 模拟输入
- **ADC0** = PIN 40、**ADC1** = PIN 38、**ADC2** = PIN 39

---

## 三、芯片 GPIO0–63 完整复用表

| GPIO | 默认功能 | 可配置复用功能 | 40-Pin 排针 |
|------|----------|----------------|-------------|
| GPIO0  | BOOT0     | BOOT0 | 未引出 |
| GPIO1  | BOOT1     | BOOT1 | 未引出 |
| GPIO2  | JTAG_TCK  | JTAG_TCK / PULSE_CNTR0 | PIN 11 |
| GPIO3  | JTAG_TDI  | JTAG_TDI / PULSE_CNTR1 / UART1_TXD | PIN 18 |
| GPIO4  | JTAG_TDO  | JTAG_TDO / PULSE_CNTR2 / UART1_RXD | PIN 10 / 13 |
| GPIO5  | JTAG_TMS  | JTAG_TMS / PULSE_CNTR3 / UART2_TXD | PIN 17 |
| GPIO6  | JTAG_RST  | JTAG_RST / PULSE_CNTR4 / UART2_RXD | PIN 20 |
| GPIO7  | IIC4_SCL  | PWM2 / IIC4_SCL | 未引出 |
| GPIO8  | IIC4_SDA  | PWM3 / IIC4_SDA | 未引出 |
| GPIO9  | GPIO9     | PWM4 / UART1_TXD / IIC1_SCL | 未引出 |
| GPIO10 | GPIO10    | CTRL_IN_3D / UART1_RXD / IIC1_SDA | 未引出 |
| GPIO11 | GPIO11    | CTRL_O1_3D / UART2_TXD / IIC2_SCL | 未引出 |
| GPIO12 | GPIO12    | CTRL_O2_3D / UART2_RXD / IIC2_SDA | 未引出 |
| GPIO13 | M_CLK1    | M_CLK1 | 未引出 |
| GPIO14 | GPIO14    | OSPI_CS / QSPI0_CS0 | PIN 34 |
| GPIO15 | GPIO15    | OSPI_CLK / QSPI0_CLK | PIN 8 |
| GPIO16 | GPIO16    | OSPI_D0 / QSPI1_CS4 / QSPI0_D0 | PIN 7 |
| GPIO17 | GPIO17    | OSPI_D1 / QSPI1_CS3 / QSPI0_D1 | PIN 32 |
| GPIO18 | GPIO18    | OSPI_D2 / QSPI1_CS2 / QSPI0_D2 | PIN 31 |
| GPIO19 | GPIO19    | OSPI_D3 / QSPI1_CS1 / QSPI0_D3 | PIN 6 |
| GPIO20 | GPIO20    | OSPI_D4 / QSPI1_CS0 / PULSE_CNTR0 | PIN 5 |
| GPIO21 | GPIO21    | OSPI_D5 / QSPI1_CLK / PULSE_CNTR1 | 未引出 |
| GPIO22 | GPIO22    | OSPI_D6 / QSPI1_D0 / PULSE_CNTR2 | 未引出 |
| GPIO23 | GPIO23    | OSPI_D7 / QSPI1_D1 / PULSE_CNTR3 | 未引出 |
| GPIO24 | GPIO24    | OSPI_DQS / QSPI1_D2 / PULSE_CNTR4 | 未引出 |
| GPIO25 | GPIO25    | PWM5 / QSPI1_D3 / PULSE_CNTR5 | 未引出 |
| GPIO26 | GPIO26    | MMC1_CLK / PDM_CLK | PIN 19 |
| GPIO27 | GPIO27    | MMC1_CMD / PULSE_CNTR5 / PDM_IN0 | PIN 15 |
| GPIO28 | GPIO28    | MMC1_D0 / UART3_TXD / PDM_IN1 | PIN 22 |
| GPIO29 | GPIO29    | MMC1_D1 / UART3_RXD / CTRL_IN_3D | PIN 24 |
| GPIO30 | GPIO30    | MMC1_D2 / UART3_RTS / CTRL_O1_3D | PIN 21 |
| GPIO31 | GPIO31    | MMC1_D3 / UART3_CTS / CTRL_O2_3D | PIN 23 |
| GPIO32 | GPIO32    | IIC0_SCL / IIS_CLK / UART3_TXD | PIN 25 |
| GPIO33 | GPIO33    | IIC0_SDA / IIS_WS / UART3_RXD | PIN 28 |
| GPIO34 | GPIO34    | IIC1_SCL / IIS_D_IN0 / PDM_IN3 / UART3_RTS | PIN 26 |
| GPIO35 | GPIO35    | IIC1_SDA / IIS_D_OUT0 / PDM_IN1 / UART3_CTS | PIN 14 |
| GPIO36 | IIC3_SCL  | IIC3_SCL / IIS_D_IN1 / PDM_IN2 / UART4_TXD | 未引出 |
| GPIO37 | IIC3_SDA  | IIC3_SDA / IIS_D_OUT1 / PDM_IN0 / UART4_RXD | 未引出 |
| GPIO38 | UART0_TXD | UART0_TXD / QSPI1_CS0 / HSYNC0 | 未引出 |
| GPIO39 | UART0_RXD | UART0_RXD / QSPI1_CLK / VSYNC0 | 未引出 |
| GPIO40 | IIC1_SCL  | UART1_TXD / IIC1_SCL / QSPI1_D0 | 未引出 |
| GPIO41 | IIC1_SDA  | UART1_RXD / IIC1_SDA / QSPI1_D1 | 未引出 |
| GPIO42 | GPIO42    | UART1_RTS / PWM0 / QSPI1_D2 | PIN 9 |
| GPIO43 | GPIO43    | UART1_CTS / PWM1 / QSPI1_D3 | PIN 35 |
| GPIO44 | GPIO44    | UART2_TXD / IIC3_SCL / SPI2AXI_CK | 未引出 |
| GPIO45 | GPIO45    | UART2_RXD / IIC3_SDA / SPI2AXI_CS | PIN 12 |
| GPIO46 | GPIO46    | UART2_RTS / PWM2 / IIC4_SCL | PIN 33 |
| GPIO47 | GPIO47    | UART2_CTS / PWM3 / IIC4_SDA | PIN 36 |
| GPIO48 | IIC0_SCL  | UART4_TXD / IIC0_SCL / SPI2AXI_DI | 未引出 |
| GPIO49 | IIC0_SDA  | UART4_RXD / IIC0_SDA / SPI2AXI_DO | 未引出 |
| GPIO50 | UART3_TXD | UART3_TXD / IIC2_SCL / QSPI0_CS4 | 未引出 |
| GPIO51 | UART3_RXD | UART3_RXD / IIC2_SDA / QSPI0_CS3 | 未引出 |
| GPIO52 | GPIO52    | UART3_RTS / PWM4 / IIC3_SCL | PIN 29 |
| GPIO53 | GPIO53    | UART3_CTS / PWM5 / IIC3_SDA | PIN 30 |
| GPIO54 | MMC1_CMD  | QSPI0_CS0 / MMC1_CMD / PWM0 | 未引出 |
| GPIO55 | MMC1_CLK  | QSPI0_CLK / MMC1_CLK / PWM1 | 未引出 |
| GPIO56 | MMC1_D0   | QSPI0_D0 / MMC1_D0 / PWM2 | 未引出 |
| GPIO57 | MMC1_D1   | QSPI0_D1 / MMC1_D1 / PWM3 | 未引出 |
| GPIO58 | MMC1_D2   | QSPI0_D2 / MMC1_D2 / PWM4 | 未引出 |
| GPIO59 | MMC1_D3   | QSPI0_D3 / MMC1_D3 / PWM5 | 未引出 |
| GPIO60 | GPIO60    | PWM0 / IIC0_SCL / QSPI0_CS2 / HSYNC1 | 未引出 |
| GPIO61 | GPIO61    | PWM1 / IIC0_SDA / QSPI0_CS1 / VSYNC1 | 未引出 |
| GPIO62 | GPIO62    | M_CLK2 / UART3_DE | PIN 27 |
| GPIO63 | GPIO63    | M_CLK3 / UART3_RE | PIN 16 |

---

## 四、注意事项

1. **PWM 使用**：6 路 PWM 全部可从排针引出（PIN 9 / 35 / 33 / 36 / 29 / 30），对应 GPIO42 / 43 / 46 / 47 / 52 / 53；这些 GPIO 同时还复用为 UART / I2C 功能，启用 PWM 时注意关闭同引脚的其他复用。
2. **GPIO4 标注不一致**：排针图将 PIN 10 标注为 IIC3_SCL(IO4)，但芯片复用表中 GPIO4 的复用为 JTAG_TDO / PULSE_CNTR2 / UART1_RXD，**不含 IIC3_SCL**（IIC3_SCL 实际可映射至 GPIO36 / GPIO44 / GPIO52）。使用前请以芯片手册与原理图为准。
3. **电源与 ADC**：电源引脚严禁短路或反接，3V3 负载能力有限；ADC 通道输入电压不可超过参考电压（一般 ≤3.3V）。
