import socket
import wave
import struct
import threading
import os
import time

VOICE_DIR = "voice"  # 保存目录名称

# UDP 参数
UDP_IP = "..."
UDP_PORT = 12345

# 支持的标签列表
VALID_LABELS = ["right", "left", "turn_on", "dim", "off", "noise", "unknown"]

# 全局状态
buffer = []                 # 当前录音的 PCM 数据（int16 字节流）
is_recording = False        # 是否正在录音
current_label = None        # 当前录音的标签
lock = threading.Lock()     # 保护 buffer 和 is_recording
file_counter = {}           # 每个标签的文件计数器 {label: count}

def ensure_voice_dir():
    """确保 voice 文件夹存在"""
    if not os.path.exists(VOICE_DIR):
        os.makedirs(VOICE_DIR)
        print(f"📁 已创建文件夹: {VOICE_DIR}")

def get_next_filename(label):
    counter = file_counter.get(label, 0) + 1
    file_counter[label] = counter
    return f"{label}.{counter:03d}.wav"

def load_existing_counts():
    """扫描 voice 目录，初始化每个标签的计数器为当前最大序号"""
    global file_counter
    if not os.path.exists(VOICE_DIR):
        return
    for filename in os.listdir(VOICE_DIR):
        if not filename.endswith('.wav'):
            continue
        # 解析文件名：label.NNN.wav
        name_without_ext = filename[:-4]  # 去掉 .wav
        parts = name_without_ext.rsplit('.', 1)  # 从右边分割一次
        if len(parts) != 2:
            continue
        label, num_str = parts
        if label not in VALID_LABELS:
            continue
        try:
            num = int(num_str)
        except ValueError:
            continue
        # 更新计数器为当前看到的最大值
        if num > file_counter.get(label, 0):
            file_counter[label] = num

def save_wav(filename, data):
    ensure_voice_dir()
    full_path = os.path.join(VOICE_DIR, filename)
    with wave.open(full_path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(16000)
        wf.writeframes(data)
    print(f"✅ 已保存: {full_path}")

def udp_receiver():
    """后台线程：持续接收 UDP 数据包，如果正在录音则加入缓冲区"""
    global buffer
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    sock.settimeout(1.0)  # 每秒超时一次，以便能检测退出
    print(f"🎧 UDP 监听中: {UDP_IP}:{UDP_PORT}")
    try:
        while True:
            try:
                data, addr = sock.recvfrom(1024)  # 每包 1024 字节
                with lock:
                    if is_recording:
                        buffer.append(data)
            except socket.timeout:
                continue
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()

def show_help():
    print("\n📋 可用命令:")
    print("  start <label>   - 开始录制指定标签的音频（标签：right/left/turn_on/dim/off/noise/unknown）")
    print("  stop            - 停止当前录音并保存")
    print("  list            - 查看当前目录下的录音文件")
    print("  help            - 显示此帮助")
    print("  quit            - 退出程序")

def main():
    global is_recording, current_label, buffer, file_counter
    ensure_voice_dir()
    load_existing_counts()  # 启动时恢复计数器
    receiver_thread = threading.Thread(target=udp_receiver, daemon=True)
    receiver_thread.start()

    show_help()
    while True:
        try:
            cmd = input("\n> ").strip().lower()
        except EOFError:
            break

        if cmd == "quit":
            if is_recording:
                print("⚠️ 正在录音中，强制退出将丢失当前录音。")
            print("👋 再见！")
            break

        elif cmd == "help":
            show_help()

        elif cmd == "list":
            files = [f for f in os.listdir('.') if f.endswith('.wav')]
            if files:
                print("📂 当前录音文件:")
                for f in sorted(files):
                    size = os.path.getsize(f)
                    print(f"  {f} ({size} bytes)")
            else:
                print("📭 没有录音文件。")

        elif cmd.startswith("start "):
            parts = cmd.split(maxsplit=1)
            if len(parts) < 2:
                print("❌ 用法: start <label>")
                continue
            label = parts[1].strip()
            if label not in VALID_LABELS:
                print(f"❌ 无效标签 '{label}'，有效标签: {', '.join(VALID_LABELS)}")
                continue

            with lock:
                if is_recording:
                    print("⚠️ 正在录音中，请先 stop 再开始新的录音。")
                    continue
                # 开始新录音
                is_recording = True
                current_label = label
                buffer = []  # 清空旧数据
            print(f"🔴 开始录制 [{label}] ... 按 stop 结束")

        elif cmd == "stop":
            with lock:
                if not is_recording:
                    print("⚠️ 当前没有正在进行的录音。")
                    continue
                is_recording = False
                label = current_label
                current_label = None
                data_to_save = b''.join(buffer) if buffer else b''
                buffer = []

            if data_to_save:
                filename = get_next_filename(label)
                save_wav(filename, data_to_save)
            else:
                print(f"⚠️ 录音为空，未保存。")

        else:
            print("❓ 未知命令，输入 help 查看帮助。")

if __name__ == "__main__":
    main()