你现在要接手一个已经基于 QtScrcpy 深度修改过的 Windows/Android 混合语言项目。不要把它当作全新项目，也不要简单套用原版 QtScrcpy 的实现。先完整理解现有仓库、原项目架构以及现有云端编译流程，再开始修改。

# 一、开始前必须先做的事情

首先读取以下交接文档：

`github-cloud-build-handoff.md`

这份文件是 GitHub 云端编译、GitHub 操作、Actions 构建、失败日志检查、产物获取等工作的交接文档。

后续涉及 GitHub 云编译的操作，应优先遵循这份交接文档。

本地电脑没有完整的 Qt、MSVC、Android SDK 等混合编译环境，因此：

**不要把“本地能否完整编译”作为完成任务的前提。**

源码分析、文本检查、静态检查可以本地进行，但最终完整编译验证必须使用 GitHub Actions 云端环境。

---

# 二、仓库关系必须明确

## 原始上游项目

QtScrcpy 主项目：

`https://github.com/barry-ran/QtScrcpy`

QtScrcpyCore：

`https://github.com/barry-ran/QtScrcpyCore`

上游仓库只作为：

* 原始架构参考
* 行为对比参考
* 判断现有定制修改来源的参考

**不要向上游仓库提交任何修改。**

## 当前实际开发仓库

以后所有源码更新、GitHub Actions、编译以及最终产物均以这个仓库为准：

`https://github.com/clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput`

这是当前唯一的开发目标仓库。

要求：

* 直接基于 `main` 工作。
* 不创建开发分支。
* 不创建 Pull Request。
* 修改完成后直接提交并推送到 `main`。
* 后续修复编译问题也继续直接更新 `main`。
* 不要为了“恢复原版结构”而覆盖现有定制代码。
* 不要把当前已经展开并修改过的 `QtScrcpyCore` 随意重新替换成上游 submodule。

在修改前先检查当前 `main` 的实际状态和最近提交，确保操作建立在最新代码之上。

---

# 三、当前项目已有的重要定制功能

这个仓库已经不是简单的 QtScrcpy v4.1.0 原版。

在开始修改之前，请自己阅读：

`CUSTOM_BUILD_README_zh.md`

并检查相关源码和提交历史，确认现有功能实现。

目前至少已经存在以下重要定制：

1. Windows FPS 模式 Raw Input。
2. 鼠标原始相对位移输入。
3. FPS 触控最高约 250Hz 的发送/合并机制。
4. 首个鼠标位移低延迟发送策略。
5. FPS 模式独占直接 KeyMap 触控路径。
6. 模拟视角触点到达边界后的无明显停顿换指机制。
7. UHID Android 系统绝对指针/鼠标透传。
8. FPS 与 UHID 模式切换。
9. 手指直接操作平板后电脑输入失效的自动恢复机制。
10. 自定义 `RESET_INPUT_STATE` 控制机制。
11. 自定义视频暂停控制功能。
12. 定制 scrcpy-server。
13. 输入状态、延迟任务、触点 ID 等一系列恢复与清理逻辑。

这些已有功能不能因为本次开发发生明显功能倒退。

尤其不要轻易重写现有 FPS 鼠标路径。

---

# 四、本次任务的核心目标

为当前项目增加一个：

**通用的、多层 KeyMap / KeyMap Layer 系统。**

它必须是一个通用功能，而不是针对某一款游戏或某一个角色写死的特殊判断。

实际需求来自类似《无畏契约手游》幽影放烟这种情况。

例如存在两个逻辑状态：

**正常战斗层**

鼠标左键执行正常射击映射。

鼠标右键执行正常战斗状态下的映射。

其他键按照普通 FPS KeyMap 工作。

按下某个技能键，例如 E：

除了正常执行 E 所绑定的手机触摸操作以外，在一次完整按键操作结束后，将当前 KeyMap 切换到另一个 Layer。

然后进入：

**幽影烟雾控制层**

此时同一个鼠标左键可以变成控制烟雾向一个方向移动的触摸。

同一个鼠标右键可以变成另一个烟雾控制触摸。

再次按 E：

完成相应的手机触摸操作以后退出这个 Layer，恢复正常战斗 Layer。

也就是逻辑上类似：

正常战斗状态
→ E
→ 烟雾控制状态
→ E
→ 正常战斗状态

但实现不能写死：

* 幽影
* E
* 左键
* 右键
* 某个固定游戏
* 固定两个 Layer

应该把它设计成一个以后能够继续扩展的通用 KeyMap 能力。

---

# 五、Layer 必须运行在 PC 端

Layer 状态机、Layer 判断、按键属于哪个 Layer、当前应该采用哪个映射等逻辑全部应该由 Windows QtScrcpy 客户端处理。

Android 端不需要知道：

* 当前是不是幽影状态
* 当前是第几个 Layer
* 当前是什么英雄
* 当前是什么游戏状态

Android 最终仍然只接收 QtScrcpy 已经生成好的普通触摸、按键等控制消息。

原则上本功能不应该要求增加新的 Android server 控制协议。

如果分析源码后发现确实必须修改 server，必须先确认为什么 PC 端无法完成；不要为了 Layer 功能无意义地增加 Android 端逻辑。

---

# 六、不要把 Layer 切换和现有 FPS/UHID 模式切换混为一谈

当前项目已经存在类似：

`~`

控制 FPS 自定义映射与鼠标/UHID操作模式切换。

这个属于：

**Input Mode / 输入模式切换。**

本次增加的：

正常战斗 Layer
烟雾 Layer
其他技能 Layer

属于：

**KeyMap Layer / 键位层切换。**

这是两个完全不同的概念。

本次 Layer 切换应该尽可能是一个非常轻量的 PC 端状态变化。

正常情况下不能因为切换 KeyMap Layer 就：

* 退出 FPS 模式
* 重建 UHID
* 重置 Raw Input
* 清空正常的 FPS 视角状态
* 重新捕获鼠标
* 执行完整的 FPS/Normal 模式切换
* 调用现有重型 mode switch 流程
* 无理由向 Android 发全局 RESET_INPUT_STATE

特别检查当前：

`switchGameMap()`

及其相关 reset 流程。

它是现有输入模式切换的一部分，不应该简单拿来充当 Layer 切换函数。

---

# 七、在设计实现前重点检查这些问题

这里不给你指定具体代码写法。

请根据源码自行分析最佳架构，但以下问题必须逐项考虑。

### 1. KeyPress 与 KeyRelease 跨 Layer

这是最重要的问题之一。

假设：

鼠标左键在 Layer A 按下。

然后用户在鼠标左键尚未释放的情况下切换到了 Layer B。

之后鼠标左键释放。

不能让：

DOWN 使用 Layer A 的坐标，

而 UP 根据 Layer B 再次查表后跑到 Layer B 的坐标。

否则很容易留下：

* 卡死触点
* 一直射击
* 按键释放失败
* Android 多点触控状态异常

需要保证一次完整输入从 DOWN 到 UP 的生命周期具有一致性。

请自行判断最合理的数据和状态管理方式。

---

### 2. 当前 Touch ID 与物理按键的关系

检查当前：

* `attachTouchID`
* `getTouchID`
* `detachTouchID`
* `m_multiTouchID`

以及鼠标和键盘点击处理。

Layer 引入以后，同一个物理键在不同 Layer 可以对应完全不同的触摸位置。

需要确认现有 Touch ID 生命周期是否仍安全。

---

### 3. 当前 KeyMap 的反向映射结构

重点研究：

* `KeyMap`
* `m_keyMapNodes`
* `m_rmapKey`
* `m_rmapMouse`
* `makeReverseMap`
* `getKeyMapNodeKey`
* `getKeyMapNodeMouse`

目前 KeyMap 主要还是原版设计。

增加 Layer 后，同一个物理按键在不同 Layer 中可以合法拥有不同映射。

必须避免不同 Layer 的相同键在现有反向映射结构中互相覆盖、随机命中或者产生不确定行为。

---

### 4. E 一类“执行动作同时切换 Layer”的按键

例如：

E 当前映射到手机上的幽影技能按钮。

用户按下 E 时仍然应该完成正常触摸：

Press → 对应 DOWN

Release → 对应 UP

之后再完成 Layer 状态变化。

需要特别考虑切 Layer 的时机。

不能让同一个 E 的 DOWN 和 UP 被两个不同 Layer 分别处理。

---

### 5. 延迟任务

检查：

* multi click
* drag
* steer wheel
* small eyes
* QTimer 延迟动作
* pending mouse delta

Layer 发生变化时，之前 Layer 已经创建的延迟动作应该怎样处理，需要有明确语义。

不能出现切 Layer 后旧定时器突然在新 Layer 中制造“幽灵触摸”。

也不要为了简单而每次 Layer 切换都粗暴重置整个 FPS 系统。

应根据当前架构找到影响最小且状态一致的处理方式。

---

### 6. Raw Input / 250Hz FPS 视角必须保持原路径

本次任务重点是“决定鼠标键/键盘键映射到什么操作”。

不要在 FPS 相对鼠标位移路径上额外增加不必要的二次坐标转换。

现有：

Raw Input
→ FPS 相对位移
→ 250Hz 控制
→ 直接触摸注入

这条高频低延迟路径应该尽量保持不变。

Layer 判断不能让鼠标视角明显增加延迟。

---

### 7. KeyMap reload

如果运行过程中重新加载 KeyMap：

需要考虑当前 Layer 如何处理。

不能保留一个已经不存在的 Layer 状态。

应该有明确、安全、可预测的默认状态。

---

### 8. 退出和重新进入 FPS

考虑：

FPS → UHID/Normal → FPS

之后应该处于什么 Layer。

请根据使用安全性设计合理行为，并清楚记录。

优先避免用户重新进入 FPS 后还处于某个已经忘记的技能子 Layer 中。

---

# 八、配置格式的目标

不要只在 C++ 中硬编码两层映射。

KeyMap JSON 应当能够表达 Layer。

但必须高度重视**向后兼容**。

目前仓库中的老 KeyMap 配置不应该因为增加 Layer 功能全部失效。

理想结果是：

旧 JSON：

继续按照原来的单层 KeyMap 行为工作。

新 JSON：

可以选择使用 Layer 功能。

具体 JSON schema、Layer 的组织结构、Layer 切换节点如何表达、是否采用默认 Layer + 子 Layer、是否需要继承/回退等，请你在完整阅读现有解析器后自行设计。

不要机械执行某个预先假设的格式。

优先考虑：

* 简洁
* 可读
* 向后兼容
* 将来容易继续增加技能 Layer
* 不重复大量完全相同的 WASD 等基础键位
* 能够处理同一个键在不同 Layer 中有不同含义

如果设计“基础 Layer + 当前 Layer 未定义按键时回退基础 Layer”更合理，可以采用；如果源码结构下有更可靠的设计，也可以选择其他方案。

先分析再决定。

---

# 九、不要把系统写成幽影专用

最终能力至少应该可以扩展为类似：

* Combat
* Smoke
* Teleport
* Ultimate
* Vehicle
* BuyMenu
* Map
* Inventory

这些只是概念示例。

不要真的在源码里写这些游戏名称。

核心系统应该只理解：

**Layer、映射、切换关系、输入生命周期。**

至于某个 Layer 在游戏里代表什么，应由配置决定。

---

# 十、兼容现有 KeyMap 功能

检查并尽量保证现有类型继续工作，包括但不限于：

* 普通点击
* 双击
* 多段点击
* steer wheel
* drag
* mouse move
* Android key
* `switchMap`
* small eyes
* 当前 FPS mouse mapping

不要因为 Layer 重构让旧配置出现行为变化。

特别是现在仓库已有游戏 KeyMap。

必须尽可能保证旧配置无需修改即可继续使用。

---

# 十一、性能目标

Layer 是 PC 端非常轻量的输入判断。

不要引入：

* 高频 JSON 动态解析
* 每个鼠标 MOVE 都进行复杂容器重建
* 不必要的锁
* 高频日志刷屏
* Android 端额外计算
* 每次事件动态创建完整 KeyMap
* Layer 切换时重建整个输入系统

正常按键查询仍应该是非常轻量的操作。

FPS 250Hz 路径的性能和延迟不能明显恶化。

---

# 十二、可调试性

增加适量但不过度的日志，使后续排查能够知道：

* 当前 KeyMap 是否支持 Layer
* 默认 Layer 是什么
* 当前 Layer 是什么
* 发生了什么 Layer 切换
* 非法 Layer 引用是否被拒绝
* KeyMap 配置解析失败的原因

不要在每个 250Hz mouse MOVE 上打印日志。

---

# 十三、源码修改前先进行差异分析

不要直接开始写。

先对比：

上游 QtScrcpy / QtScrcpyCore

和：

`clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput`

确认：

哪些文件仍与上游一致；

哪些文件已经被当前项目修改；

本次最适合在哪一层增加功能；

哪些已有定制绝对不能被上游源码覆盖。

特别注意：

当前 KeyMap parser 基本仍接近上游；

而 `InputConvertGame` 已经因为 Raw Input、250Hz、输入恢复等功能产生了大量定制。

因此不能直接复制一份上游 `InputConvertGame` 覆盖当前版本。

---

# 十四、GitHub Actions 云端编译

本项目最终必须通过 GitHub 云端完整构建。

当前项目包含：

* C/C++
* Qt
* Windows MSVC
* CMake
* Java scrcpy-server
* Android SDK / Build Tools
* PowerShell 构建脚本

属于混合语言编译。

优先沿用现有成熟流程，不要重新发明完全不同的 CI。

当前仓库已有：

`.github/workflows/custom-windows-x64.yml`

先检查它。

当前已知有一个需要处理的问题：

这个 workflow 当前的 `push` 事件带有过窄的 `paths` 条件，普通源码更新到 `main` 时未必会自动触发构建。

请确认实际配置。

本次修改后必须保证：

**main 中真正影响 QtScrcpy 的源码修改能够可靠进入 GitHub Actions 构建。**

可以修正 workflow 的触发条件，也可以根据交接文档采用可靠的手动 workflow dispatch；同时最好让未来正常源码更新也能直接触发构建。

不要出现：

“源码已经 push 成功，因此认为编译成功”

这种错误判断。

必须看到真正的 GitHub Actions build run 完成。

---

# 十五、云编译环境不要随意降级

先读取现有 workflow 和交接文档。

当前项目的构建环境涉及：

* Windows Server runner
* Visual Studio 2022
* Qt 5.15.2
* MSVC x64
* JDK 17
* Android SDK Platform 36
* Build Tools 36.0.0
* CMake

除非编译错误证明必须调整，否则不要随意升级/降级这些关键版本。

尤其不要为了让 C++ 编译通过而破坏定制 scrcpy-server 的 Java/Android 构建。

---

# 十六、定制 scrcpy-server 必须保留

当前仓库不是单纯使用官方 scrcpy-server。

现有客户端和服务端之间已经存在定制功能与消息。

不要：

* 换回官方原版 server
* 删除定制 server
* 删除已有输入恢复协议
* 删除已有视频暂停协议
* 重新拉取上游 server 覆盖当前版本

本次 Layer 功能原则上应属于 PC 客户端能力。

---

# 十七、执行流程

完成分析以后直接继续实施，不需要只给我一份设计报告然后停止。

完整任务应当包含：

1. 阅读云编译交接文档。
2. 检查 GitHub 当前目标仓库。
3. 确认 `main` 最新状态。
4. 阅读项目定制 README。
5. 对比上游与定制版本的关键输入代码。
6. 设计 Layer 架构。
7. 检查上述输入生命周期风险。
8. 修改源码。
9. 更新必要的 KeyMap 文档。
10. 保持旧 KeyMap 向后兼容。
11. 做能够在当前环境完成的静态检查。
12. 检查 Git diff，避免无关改动。
13. 直接提交到目标仓库 `main`。
14. 推送到 GitHub。
15. 触发 GitHub Actions 云端混合编译。
16. 监控实际构建结果。
17. 如果失败，读取具体失败 job/step/log。
18. 找出真正根因。
19. 修复。
20. 再次直接提交 `main`。
21. 再次云编译。
22. 重复直到得到成功构建产物，或者遇到确实无法在当前条件解决的外部阻塞。

不要在第一次 CI 报错后就停止并把日志扔给用户。

能自行修复的编译错误应继续修复。

---

# 十八、GitHub 操作规则

唯一可写仓库：

`clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput`

目标分支：

`main`

禁止：

* 新建 feature branch
* 新建开发分支
* PR 工作流
* force push
* 重写已有 main 历史
* reset 掉之前的用户定制
* 大规模无关格式化

提交应保持主题清晰。

如果 CI 修复需要第二次、第三次提交，可以正常继续追加提交，不需要 squash，也不要重写历史。

---

# 十九、验收标准

不能只以“代码看起来合理”为完成。

至少检查：

### 旧 KeyMap

原有不包含 Layer 的 JSON 仍能正常解析。

行为与修改前保持兼容。

### 新 Layer

能够存在默认 Layer 和至少一个额外 Layer。

同一物理键在不同 Layer 中可以对应不同触摸。

一个普通映射按键可以在完成自身动作的同时触发 Layer 转换。

可以从子 Layer 返回默认 Layer。

### 输入一致性

跨 Layer 的 KeyPress/KeyRelease 不产生错位 UP。

切 Layer 不产生残留触点。

快速操作不容易产生幽灵触点。

已有延迟操作不会无控制地跨 Layer 污染新状态。

### FPS

Raw Input 路径继续存在。

250Hz 合并机制继续存在。

FPS 视角不因为普通 Layer 切换执行完整 reset。

FPS 和 UHID 原有切换仍然工作。

### 输入恢复

现有触屏导致电脑输入失效后的恢复机制不能因为 Layer 功能被破坏。

### Server

现有定制 scrcpy-server 正常参与构建和打包。

### 编译

GitHub Actions Windows x64 完整构建成功。

最终 ZIP artifact 能正常生成。

---

# 二十、测试配置

可以增加一个用于验证 Layer 功能的示例/测试配置或更新键位文档。

但不要虚构我的游戏实际触摸坐标。

真正的游戏坐标后续可以根据我的设备和游戏 UI 再配置。

这一阶段重点是确保：

**框架确实支持这种能力。**

如果增加示例，请让示例清楚表达：

普通层中一个鼠标按钮映射到位置 A；

进入第二层后同一个鼠标按钮映射到位置 B；

某个普通键完成自己的触摸后触发 Layer 切换；

再次操作可以返回默认 Layer。

示例只能作为机制展示，不要把示例游戏逻辑写进核心代码。

---

# 二十一、最终交付信息

任务完成以后给我一份简洁但完整的结果。

必须包含：

* 最终采用的 Layer 架构概述
* 为什么选择这种设计
* 修改的主要文件
* KeyMap 新能力如何使用
* 原有 JSON 如何保持兼容
* 如何避免跨 Layer DOWN/UP 错乱
* Layer 与现有 `~` FPS/UHID 模式切换之间的关系
* 是否影响 Raw Input / 250Hz
* 是否修改 Android server，以及原因
* GitHub 最终 commit SHA
* GitHub Actions run
* Actions 最终状态
* 构建 artifact 名称
* 若已下载产物，给出产物所在位置
* 当前仍需要真机验证的项目

不要只说“编译成功”。

---

# 二十二、决策原则

这不是要求你机械按照某个预先设计的实现方案编码。

你应该充分利用源码分析能力。

如果你发现更合理的架构，可以采用更合理的方案，只要满足这些核心目标：

**通用 Layer，而非游戏硬编码。**

**PC 端状态判断。**

**旧 KeyMap 向后兼容。**

**DOWN/UP 生命周期安全。**

**不破坏现有 FPS Raw Input / 250Hz。**

**不把轻量 Layer 切换错误地变成完整 Input Mode reset。**

**保留当前定制 scrcpy-server 和输入恢复功能。**

**最终必须经过真实 GitHub Actions 云端混合编译验证。**

**只更新 `clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput` 的 `main`。**

先阅读交接文件和两个仓库，再开始执行。
