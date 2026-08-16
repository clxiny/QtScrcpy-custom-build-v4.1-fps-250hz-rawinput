# QtScrcpy 分层按键映射编辑器

这是 [QtScrcpy Custom v4.1 · Input Recovery Edition](https://github.com/clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput) 的独立本地 KeyMap 编辑器。它为该定制版的分层 JSON 扩展而做：旧版单层 KeyMap 也能打开，但 `layers`、`switchLayer` 和 `switchMap` 需要使用本编辑器才能完整保留。

`index.html` 可直接用浏览器打开。使用本地预览服务器时：

```powershell
node server.js
```

随后访问 `http://127.0.0.1:4173`。

编辑器可导入旧版单层 KeyMap 与新分层 KeyMap。所有导入、编辑和导出均在本机进行，不会上传配置文件。

基本流程：导入投屏程序仓库 `keymap/` 下的模板或自己的 JSON，在画布上调整点位，导出 JSON，再在 QtScrcpy 中载入。根目录的 `无畏契约幽影.json` 是分层、FPS 起点、方向轮盘和鼠标键映射的参考模板。

点击“加载背景图”或直接将图片拖入页面，即可将游戏截图显示在画布下方，用于核对按键位置。背景图不会写入或影响导出的 JSON；应选择与 KeyMap 屏幕分辨率相同或比例一致的截图。

选中方向轮盘后，右侧可分别编辑上、左、右、下方向的按键与偏移量；也可以直接拖动画布上的 W、A、D、S 方向点。它们会分别写入 `upOffset`、`leftOffset`、`rightOffset`、`downOffset`。

普通“点击”节点还可勾选“松开后切换平板虚拟鼠标”。这就是旧配置的 `switchMap: true`：该按键仍会发送到游戏，松开后在 FPS 触摸映射和 Android UHID 平板虚拟鼠标之间切换，适合 `Tab` 背包或 `M` 地图。它不会合成一个 `~` 键事件，也和 `switchLayer` 相互独立，不能把它当成图层切换使用。

## 对应项目

- 投屏程序、完整源码和 Windows 构建包：[clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput](https://github.com/clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput)
- 本编辑器只处理本地 JSON，不包含投屏程序、ADB 或 Android 服务端。

## 许可证与致谢

本项目以 MIT License 发布，详见 [LICENSE](LICENSE)。界面布局和部分编辑交互参考了 [w4po/ScrcpyKeyMapper](https://github.com/w4po/ScrcpyKeyMapper)，其版权与 MIT 许可证文本保留在 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
