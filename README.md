# Voice Control + Acceleration

物理课程项目：声控与加速度相关软硬件代码仓库。
报告附录可直接引用本仓库链接，避免把全部源码塞进 PDF。

仓库地址：https://github.com/apx-6/voice-control-accel

## 仓库结构

`	ext
.
├── arduino/          # Arduino / 硬件控制代码
├── voice_model/      # 声控模型相关文件
├── scripts/          # 生图 / 数据处理脚本
├── docs/             # 接线说明、实验说明等
├── .gitignore
└── README.md
`

## 各目录说明

| 目录 | 用途 |
|------|------|
| rduino/ | 单片机程序、引脚定义、串口通信等 |
| oice_model/ | 模型配置、推理入口、标签映射 |
| scripts/ | 报告用图生成、数据可视化、后处理 |
| docs/ | 实验步骤、硬件接线、使用说明 |

## 关于大文件

一般代码、配置、较小模型可以直接提交。
若单个文件超过约 50–100MB，GitHub 可能拒绝推送，可改用 [Git LFS](https://git-lfs.com)，或放到网盘并在 oice_model/README.md 写下载链接。

## 快速开始

### Arduino

1. 用 Arduino IDE 打开 rduino/control/control.ino
2. 安装依赖库：Adafruit ADXL345、Adafruit Unified Sensor
3. 选择对应开发板与端口
4. 编译并烧录

### 声控模型

1. 安装依赖
2. 按 oice_model/README.md 放置模型文件
3. 运行推理 / 联调脚本

### 生图脚本

1. 进入 scripts/
2. 按脚本注释准备输入数据
3. 运行后输出图片到约定目录

## 报告附录引用示例

> 完整代码、模型说明与生图脚本见：
> https://github.com/apx-6/voice-control-accel

## 贡献者

- Yueying Jin (Team Leader)
- Haoqi Chen
- Peiran Su
- Chenming Tao
- Pinjia Zeng

## License

课程作业用途；如无特殊说明，仅供本课程提交与评阅使用。
