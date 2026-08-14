# QtScrcpy 定制版源码说明

本源码包基于 QtScrcpy v4.1.0，包含以下尚未正式发布的改动：

1. 服务端视频暂停
   - 新增控制消息 `SET_VIDEO_PAUSED = 23`。
   - 暂停后，Android 端停止屏幕采集，向 MediaCodec 发送 EOS，释放编码器并停止发送视频帧。
   - 控制连接保持活动，键盘、鼠标和其他控制指令仍可传输。
   - 恢复后重新创建采集和编码会话。

2. UHID 绝对系统指针与游戏触摸桥接
   - 通过 scrcpy 控制通道在 Android 端创建标准 HID 绝对绘图板设备，不需要额外安装 APK，也不需要 root。
   - 不再使用注入式 `SOURCE_MOUSE/ACTION_HOVER_MOVE`：这种事件只会送到应用，手指触摸把系统光标隐藏后，它无法让 Android 的系统光标重新显示，这就是上一版透传模式下光标消失的原因。
   - 新版描述符使用 `Digitizer + INPUT_PROP_POINTER + Stylus`。绝对 UHID 悬停报告会经过 Android InputReader 和 PointerChoreographer，由系统直接绘制并保持正常鼠标指针；Android 15/16 会把它识别为 `SOURCE_MOUSE | SOURCE_STYLUS` 绘图板。
   - 指针使用与当前视频帧同宽高比的绝对 X/Y 轴，不受 Android 鼠标加速度或初始光标位置影响。旧版把两轴都固定为 `0..32767`，等价于正方形绘图板；Android 16 的 `POINTER` 模式会对两轴使用同一个缩放值，映射到 `2136×3200` 这类矩形屏幕时会造成光标与点击位置逐渐分离。
   - 左键按下、拖动和释放不发送 `SOURCE_MOUSE` 点击，而是使用投屏窗口坐标，按端点对齐方式换算到当前视频帧后，经 scrcpy 控制通道注入为手指触摸，可控制过滤鼠标来源的手游界面。
   - Android 16 存在悬停流与左键按下冲突（上游 scrcpy 的对应现象需用 `--no-mouse-hover` 规避）。本版在左键按下前先发送绘图板 `InRange=0` 结束悬停，等待 24ms 让 InputReader 消费退出事件；按住期间只传手指触摸，松开后再恢复绝对系统指针。这样既保留可见光标，也不会让悬停流吞掉游戏点击。
   - 分辨率无需写死：首次移动鼠标时，客户端使用服务端上报的当前视频帧尺寸动态生成 HID X/Y 最大值，使 Android 的等比绘图板映射与 scrcpy `PositionMapper` 的触摸映射一致；旋转或视频分辨率改变后会自动销毁并按新宽高比重建 UHID 设备。
   - 这里不调用 `adb shell input tap`；最终由项目原有 `CMT_INJECT_TOUCH` 控制消息和服务端触摸注入完成。
   - 支持右/中键、侧键以及横向/纵向滚轮；开启后隐藏并限制电脑端光标在画面内，按 `Ctrl+Shift+M` 可退出透传。
   - 手动关闭透传或断开设备时主动销毁 Android 端的 UHID 设备；使用 `~` 返回 FPS 时只发送 `InRange=0` 并休眠设备，避免 Android InputReader 因反复销毁/创建输入设备而重配置。
   - UHID 模式独立维护按键和触摸状态，退出时会补发触摸释放，避免按键卡住。
   - UHID 系统鼠标与 FPS 射击映射互斥，但由 `~` 自动双向切换：FPS 中按一次显示 Android 鼠标用于购买操作，再按一次关闭鼠标并返回 FPS 瞄准模式。
   - FPS 模式具有独占的低延迟输入分支：鼠标移动和映射点击直接发送项目原有 `CMT_INJECT_TOUCH`，不会落回 UHID、24ms 点击桥或普通鼠标坐标转换。
   - 每次进入 FPS 都会重置旧的鼠标坐标基准、尚未发送的合并位移和未执行的 30/60ms 延迟动作，并将 Windows 光标回到画面中心；购买菜单中的指针位置不会污染下一次瞄准移动。
   - Windows FPS 模式改用 `WM_INPUT` 读取鼠标原始相对位移，不再依赖按屏幕像素生成的 `WM_MOUSEMOVE`；125/500/1000Hz 鼠标都能按硬件实际报告频率进入独立 FPS 路径，系统指针加速副本不会被重复计算。每次处理还会批量排空已经积累的 Raw Input 报告，避免渲染繁忙时旧位移滞留在 Windows 消息队列。
   - 连续原始报告采用“首包立即发送、后续每 4ms 合并一次”的无积压策略，Android 视角触摸最高更新率为 250Hz；不会把 1000Hz 的每个报告都堆进 Qt/TCP 队列形成越来越高的延迟。
   - FPS 拖拽的 DOWN/MOVE 全程保持有效触摸压力，只有 UP 才把压力归零，避免部分游戏把高频 MOVE 识别成不稳定的触点状态。
   - 模拟手指到达 5%/95% 安全边界时，不再像旧版一样丢弃当前位移和后续 5 个鼠标事件，而是在同一批输入内结束旧触点、从键位配置起点建立新触点并继续处理剩余位移，快速 180/360 度转身不会出现固定空档。
   - 控制 TCP 通道启用低延迟选项，减少高频小型触控包被合并等待的机会。Raw Input 注册失败时会打印警告并自动保留原来的 Qt 鼠标事件路径。

3. 手指触屏后的输入自恢复
   - 新增客户端/服务端配套控制消息 `RESET_INPUT_STATE = 24`。手指直接触摸平板时，Android 可能取消电脑端正在注入的触摸流，但原版 scrcpy 仍保留旧指针，后续 DOWN/MOVE 就可能与残留触点混在一起，表现为电脑点击和 FPS 操作全部失效。
   - 每一组新的电脑触摸开始前，客户端先发送一次轻量复位，再沿用原来的 `CMT_INJECT_TOUCH` 直接发送 DOWN；FPS 连续 MOVE、Windows Raw Input 和 4ms/250Hz 合并链路不经过第二层坐标或鼠标转换。
   - 服务端优先调用 Android 的全局触摸取消接口；普通 shell 权限无法使用时，会自动退回到 `ACTION_CANCEL + 清空 scrcpy 指针表`，不需要安装 APK，也不要求 root。root/adbd-root 会让全局取消路径可用，但“设备已 root”本身并不会自动把普通 adb shell 进程变成 root。
   - 客户端同时清理尚未执行的连点、拖拽、小眼睛重启、方向盘队列、FPS 合并位移和 UHID 按键状态，防止复位后旧定时器再次制造幽灵触点。
   - 如仍遇到异常，直接按 `Ctrl+Shift+R` 手动复位输入，不会断开投屏、不会重启平板；下一次鼠标移动会自动重建系统指针设备。
   - 服务端只在确实发现 scrcpy 残留触点时输出一条恢复日志，正常点击不会持续刷 adb 日志。

## 目录说明

- `QtScrcpy/`：已修改的 QtScrcpy 客户端源码。
- `QtScrcpy/QtScrcpyCore/`：已修改的核心源码，已展开子模块内容。
- `server-patches/`：客户端补丁、服务端补丁和自动构建脚本。
- `ci/win/`：Windows x64 编译与发布脚本。

## Windows 精简包

- 只保留 Windows x64 编译所需 C++ 源码、FFmpeg x64 库、Windows ADB、配置、键位映射和服务端补丁。
- 已删除 Linux/macOS 实现、对应二进制和构建脚本，也删除截图、说明文档配图、备份图片等非编译内容。
- `QtScrcpy/res/` 中仍保留少量图标、皮肤和字体文件，它们被 `res.qrc` 或 Windows 资源脚本直接引用，删除会导致 Windows 编译失败或界面缺失。

## 当前状态

- 客户端与服务端代码修改已经完成；绝对 UHID 绘图板本身使用 scrcpy v4.1 已有的 UHID 协议，输入复位和视频暂停则要求源码包中配套的定制服务端。
- 两个服务端补丁已验证能够按顺序干净应用到官方 scrcpy v4.1；定制服务端也已使用 Android 36 API 完成 Java 编译、D8 转换和 APK 完整性检查。
- 尚未完成 Windows 实机编译及 Android 设备联调。
- FPS Raw Input/250Hz 合并发送改动已完成静态接口、补丁完整性与空白错误检查；当前环境没有 Qt MSVC 工具链，因此仍需在 Windows 上实际编译并以游戏内 180/360 度转身验证手感。
- 源码包中 `QtScrcpyCore/src/third_party/scrcpy-server` 已替换为与消息 23/24 匹配的定制服务端，可直接随 Windows 客户端编译打包。不要换回官方原版服务端。

## Windows 构建要求

- Visual Studio 2022（Desktop development with C++）
- Qt 5.15.2 MSVC 2019 64-bit
- CMake 3.19 或更高版本
- JDK 17
- Android SDK Platform 36 和 Build Tools 36.0.0
- Git

可参考 `server-patches/build-custom-server.ps1` 编译并替换服务端，然后执行项目原有的：

```bat
ci\win\build_for_win.bat RelWithDebInfo x64
ci\win\publish_for_win.bat x64 ..\..\package\QtScrcpy-custom-win-x64
```

运行后进入 FPS 模式，日志应出现：

```text
FPS raw mouse input enabled (250 Hz touch coalescing)
```

退出 FPS 后应出现 `FPS raw mouse input disabled`。如果注册失败，程序会自动使用旧的 Qt 鼠标移动路径，并在日志中输出 Windows 错误码。

手指触屏后如果系统输入流没有自动恢复，可按：

```text
Ctrl+Shift+R
```

程序会输出 `Input state recovery requested`，无需断开设备或重启平板。
