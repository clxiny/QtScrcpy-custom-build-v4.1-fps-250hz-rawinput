# QtScrcpy Custom v4.1 — FPS 250 Hz Raw Input

一个面向 Windows x64 的 QtScrcpy 定制版本，针对手游 FPS 键鼠操作、鼠标透传和高轮询率鼠标输入进行了优化。

## 上游项目与致谢

- 客户端基于 [barry-ran/QtScrcpy](https://github.com/barry-ran/QtScrcpy) 的 QtScrcpy v4.1.0 源码修改。
- Android 服务端基于 [Genymobile/scrcpy](https://github.com/Genymobile/scrcpy) v4.1 构建并应用本仓库的服务端补丁。

本仓库不是上述项目的官方发布渠道，也不隶属于上游作者。

## 本版本的功能与优化

- 服务端视频暂停：暂停投屏时停止 Android 端采集与编码，控制通道仍保持可用，恢复时重新建立采集和编码会话。
- UHID 绝对系统指针：通过 scrcpy 控制通道创建 Android HID 绝对绘图板设备，无需 Root 或额外 APK；系统可保持可见鼠标指针。
- 游戏触摸桥接：左键、拖动通过投屏窗口坐标转换成 scrcpy 触摸控制消息，适配只接收触摸事件的游戏界面。
- 坐标对齐：UHID X/Y 轴按当前视频帧宽高比动态生成；旋转或分辨率变化后重建设备，减少矩形屏幕上的鼠标和点击偏移。
- FPS 低延迟路径：FPS 模式下的移动与映射点击直接发送触摸控制消息，不经过普通鼠标转换或 UHID 点击桥。
- Windows Raw Input：FPS 模式使用 `WM_INPUT` 读取物理鼠标相对位移，避免将系统加速后的 `WM_MOUSEMOVE` 副本重复计算。
- 250 Hz 触控发送：首个 Raw Input 报告立即发送，后续报告按 4 ms 合并，最高约 250 Hz，降低 500/1000 Hz 鼠标造成的 Qt/TCP 队列积压。
- FPS 边界和拖拽优化：高频拖拽保持触摸压力；到达安全边界时立即重建触点并继续处理剩余位移，降低快速转身时的固定空档。
- 控制通道低延迟：为高频小型触控包启用低延迟选项；Raw Input 注册失败时自动回退到原 Qt 鼠标事件路径。

进入 FPS 模式后，日志应显示：

```text
FPS raw mouse input enabled (250 Hz touch coalescing)
```

退出 FPS 模式后，日志会显示 `FPS raw mouse input disabled`。

## 发布包

已构建的 Windows x64 便携包在 [Releases](../../releases) 页面提供。解压后运行 `QtScrcpy.exe`；压缩包已包含 Qt 运行库、ADB 和定制 `scrcpy-server`。

## 源码与构建

当前定制版的可构建源码已解压并以目录结构直接保存在本仓库；不提供源码 ZIP 作为发布附件。本源码包面向 Windows x64 构建，未包含上游的 Linux/macOS 实现与非 Windows 构建资源。

GitHub Actions 工作流会：构建定制 scrcpy 服务端、使用 Qt 5.15.2 / MSVC 2019 x64 编译 QtScrcpy，并通过 `windeployqt` 生成便携包。

本地 Windows 构建需要 Visual Studio 2022（C++ 桌面开发）、Qt 5.15.2 MSVC 2019 64-bit、CMake 3.19+、JDK 17、Android SDK Platform 36、Build Tools 36.0.0 和 Git。

## 许可证

本仓库保留上游附带的 Apache License 2.0 文件。使用、分发或继续修改时，请同时遵守 QtScrcpy、scrcpy 及其依赖各自的许可证要求。
