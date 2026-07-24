# Voice Control + Acceleration

物理课程项目：声控与加速度相关软硬件代码仓库。  
报告附录可直接引用本仓库链接，避免把全部源码塞进 PDF。

## 仓库结构

```text
.
├── arduino/          # Arduino / 硬件控制代码
├── voice_model/      # 声控模型相关文件（配置、说明；大权重见下方）
├── scripts/          # 生图 / 数据处理脚本
├── docs/             # 接线说明、实验说明等文档
├── .gitignore
└── README.md
```

## 各目录说明

| 目录 | 用途 |
|------|------|
| `arduino/` | 单片机程序、引脚定义、串口通信等 |
| `voice_model/` | 模型配置、推理入口、标签映射；大文件勿直接塞进 Git |
| `scripts/` | 报告用图生成、数据可视化、后处理脚本 |
| `docs/` | 实验步骤、硬件接线、使用说明 |

## 大文件说明（模型权重）

体积较大的模型文件（如 `.pt` / `.onnx` / `.h5`）默认已被 `.gitignore` 忽略。推荐任选其一：

1. 使用 [Git LFS](https://git-lfs.com) 跟踪大文件  
2. 放到网盘 / 课程平台，并在 `voice_model/README.md` 中写下载链接

## 快速开始

> 代码仍在整理中；下列步骤待源码放入后补全。

### Arduino

1. 用 Arduino IDE 或 PlatformIO 打开 `arduino/`
2. 选择对应开发板与端口
3. 编译并烧录

### 声控模型

1. 安装依赖（待补充 `requirements.txt`）
2. 按 `voice_model/README.md` 放置权重文件
3. 运行推理 / 联调脚本

### 生图脚本

1. 进入 `scripts/`
2. 按脚本注释准备输入数据
3. 运行后输出图片到约定目录

## 报告附录引用示例

> 完整代码、模型说明与生图脚本见：  
> `https://github.com/<your-username>/voice-control-accel`

## 贡献者

- （待补充）

## License

课程作业用途；如无特殊说明，仅供本课程提交与评阅使用。
