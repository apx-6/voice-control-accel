# Voice Model（声控模型）

放置声控模型相关配置与推理代码。

## 建议内容

- 标签映射、采样率、特征说明
- 推理入口脚本
- 与 Arduino / 上位机联调方式

## 模型文件

较小的模型 / 配置可以直接放进本目录并提交。  
若单文件过大（约 50–100MB 以上）推送失败，可改用 Git LFS，或外链下载。

**大文件下载链接（如有）：** （待补充）
- ei-chiojynquee-project-1-arduino-1.0.18-eon-3 为训练的语音识别模型库
- resoeaker_tx 包含实现声控部分的ino主代码, 以及头灯/led灯组控制代码
- record_arduino 为使用I2S捕捉音频并保存的主代码
- record.py 实现接收与保存音频

代码基于seed官方wiki, 详见:
- 配合EI实现语音识别 https://wiki.seeedstudio.com/cn/respeaker_xvf3800_xiao_edge_impulse/
- 使用I2S捕捉音频 https://wiki.seeedstudio.com/cn/respeaker_xvf3800_xiao_udp_audio_stream/
