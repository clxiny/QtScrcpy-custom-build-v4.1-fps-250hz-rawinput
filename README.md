# QtScrcpy Custom v4.1 — Input Recovery

Windows x64 定制版本，面向 Android 投屏、游戏键鼠控制、UHID 系统指针和高轮询率 FPS 输入。

## 上游项目

- Qt 客户端基于 [barry-ran/QtScrcpy](https://github.com/barry-ran/QtScrcpy) 的 QtScrcpy v4.1.0 修改。
- Android 服务端基于 [Genymobile/scrcpy](https://github.com/Genymobile/scrcpy) v4.1 构建，并应用本仓库中的服务端补丁。

本仓库不是上游项目的官方发布渠道，也不隶属于上游作者。

## 本版本功能与优化

- 视频暂停：新增匹配服务端的控制消息；暂停时停止 Android 屏幕采集与编码，控制连接保持可用。
- UHID 绝对系统指针：使用 Android HID 绘图板，保持可见系统指针，无需 Root 或额外 APK。
- 游戏触摸桥接：主键、拖拽按投屏窗口坐标转换为 scrcpy 触摸控制消息，适配只接收触摸的游戏界面。
- 动态坐标对齐：UHID 描述符按视频帧宽高比生成，旋转或分辨率改变后重建，减少指针与点击偏移。
- FPS Raw Input：Windows FPS 模式使用 `WM_INPUT` 原始相对鼠标输入，不重复计算系统加速鼠标事件。
- 250 Hz 合并发送：首个报告立即发送，后续按 4 ms 合并，减少 500/1000 Hz 鼠标造成的队列积压。
- FPS 边界与拖拽优化：高频拖拽保持有效压力；在安全边界无空档重建触点，改善快速 180/360 度转身。
- 输入状态自恢复（本版新增）：新增客户端/服务端消息 `RESET_INPUT_STATE = 24`。平板被手指直接触摸导致 Android 取消电脑注入触摸后，下一组电脑触摸会先复位残留输入状态；可按 `Ctrl+Shift+R` 手动恢复，无需断开投屏或重启设备。

进入 FPS 模式后，日志应显示：

```text
FPS raw mouse input enabled (250 Hz touch coalescing)
```

手动恢复时，日志会显示：

```text
Input state recovery requested
```

## 下载与构建

已构建的 Windows x64 便携程序在 [Releases](../../releases) 页面提供。解压后运行 `QtScrcpy.exe`；发布包包含 Qt 运行库、ADB 和匹配消息 23/24 的定制 `scrcpy-server`。

当前定制版可构建源码已解压并以目录结构直接存储在本仓库，不提供源码 ZIP 附件。本源码包面向 Windows x64，未包含上游的 Linux/macOS 实现与非 Windows 构建资源。

GitHub Actions 会构建定制 scrcpy 服务端、编译 QtScrcpy，并用 `windeployqt` 生成便携包。

## 许可证

本仓库保留上游附带的 Apache License 2.0 文件。继续使用或分发时，请同时遵守 QtScrcpy、scrcpy 及其依赖的许可证要求。
