# 中文本地化

本分支在原生客户端上加入了两层中文支持：**客户端界面**（启动器、F1 菜单、提示）
和**字幕**（语音/影片对话）。游戏自身的文本层另见文末说明。

## 1. 客户端界面

- 语言选项：启动器「语言 → 界面语言」，或游戏内 F1 菜单的「语言 → 界面语言」。
- 可选「自动（跟随系统）」「简体中文」「English」。系统语言无法识别时默认中文。
- 设置保存在 `settings.ini` 的 `ui_lang = auto|zh|en`。
- 界面字体：程序按以下顺序寻找含中文字形的 TTF：
  1. 环境变量 `LAIN_UI_FONT`
  2. 数据目录下的 `fonts/ui.ttf`、`fonts/ui.otf`、`fonts/ui.ttc`、`font.ttf`
  3. 程序所在目录的 `fonts/ui.ttf`
  4. 系统字体（Windows：微软雅黑/黑体/宋体；macOS：苹方/冬青黑体；Linux：Noto CJK/文泉驿）
  5. ImGui 内置字体（仅拉丁字母）

  Windows/macOS/Linux 通常能直接找到系统中文字体。Switch 上没有系统字体，
  请把一个中文字体复制为 `sdmc:/switch/lain/fonts/ui.ttf`。

## 2. 中文字幕（`lang/zh/`）

游戏语音是日语；字幕由客户端叠加绘制，支持任意 Unicode，因此中文可以直接显示。
一个语言包就是一个文件夹：

```
<数据目录>/lang/zh/pack.txt           name = 简体中文
<数据目录>/lang/zh/font.ttf           可选；界面字体已含中文时可不放
<数据目录>/lang/zh/subtitles/index.txt
<数据目录>/lang/zh/subtitles/LAIN01.XA.ch00.srt
<数据目录>/lang/zh/subtitles/F001.STR.srt
<数据目录>/lang/zh/subtitles/Cou001.srt   （也可按节点命名）
```

数据目录：

| 平台 | 路径 |
|---|---|
| Windows | `%APPDATA%\LainNative\lain-native\` |
| macOS | `~/Library/Application Support/LainNative/lain-native/` |
| Linux | `~/.local/share/LainNative/lain-native/` |
| Switch | `sdmc:/switch/lain/` |

### 生成模板

导入光盘后运行：

```bash
lain --make-subtitle-pack zh 简体中文
```

会生成全部 754 条轨道的占位 SRT（672 个语音声道 + 82 部影片）。占位字幕不会显示，
所以可以只翻译完成的轨道。然后在启动器「文本／字幕」中选择「简体中文」。

也可以用**文件夹模式**：不写 `index.txt`，把任意层级的 `.srt` / `.ass` 放进去，
文件名（去扩展名）就是轨道名，例如 `Cou001.ass`。

### 完整中文翻译包

`lain-zh-subtitles.zip` 是一个完整的中文语言包：667 个节点、5250 条对白，时间轴沿用
laingame.net 的英文粉丝字幕（该字幕属于其作者，不在本仓库内）。解压到数据目录的 `lang/`
下，得到 `lang/zh/`，然后在启动器或 F1 菜单里选择「简体中文」即可。语言包的 SRT/ASS
文件可以继续用 Subtitle Edit、Aegisub 等工具修改；名字保持不变就会按节点匹配。

本地构建 Switch 版时，把语言包放到 `port/packs/zh/`（该目录被 `.gitignore` 忽略，不会提交），
再运行 `port/tools/package_switch.sh`，生成的 `Lain-switch.zip` 会带上 `Lain/lang/zh/`。

### 双语字幕

一个字幕条目里放两行即可：

```
1
00:00:01,000 --> 00:00:04,000
原文
译文
```

### 注意事项

- 文件必须是 UTF-8（可带 BOM）；GBK 会显示为乱码。
- 字幕按轨道时间计时；时间轴请对照音频。
- 每条字幕由 `(sector - 文件起点) / 150` 秒定位，和模拟器/原机一致。

## 3. 游戏内文本

游戏本体的界面文字（`Authorize User`、`Load Data`、节点名、关键词等）是**英文
ASCII**，由游戏自己的 4bpp 位图字体绘制：

- 字体来自 `BIN.BIN` 第 1 项，是 256×136 的 4bpp 图集；
- 字形表 `g_font_glyphs_large/small` 只有 128 项，对应单字节 `0x00`–`0x7F`；
- `font_copy_glyph` / `font_render_string` 对 `>= 0x80` 的字节直接返回 0。

因此**只改文本内容无法显示中文**（会变成空白），要游戏内汉化必须改字体引擎：
替换/扩充字模、把单字节字形表改成 Unicode 映射、并在渲染函数里加入多字节解码。
这是一项独立的、较大的改动，本分支暂未包含。

当前可行的折中：

| 内容 | 方案 |
|---|---|
| 语音/影片对话 | 中文字幕（本文第 2 节），已完成支持 |
| 节点文本窗口的消息 | 可用模组/插件把翻译画在客户端覆盖层上（F1 菜单 → 模组） |
| 静态菜单图（标题菜单等） | 用图片模组替换为中文版（`MODDING.md` 的图片替换） |
| 节点名、关键词、信息行 | 需要上述字体引擎改造；数据本身可先用 `data/strings.json`、`data/nodes.json` 改成中文内容 |

## 4. Nintendo Switch

- 构建产物：GitHub Actions 的 `switch client` 工作流生成 `Lain-switch.zip`。
- 安装：把压缩包里的 `Lain/` 内容整体复制到 `sdmc:/switch/lain/`，从 Homebrew Menu 启动。
- 手柄：Joy-Con / Pro Controller 直接可用；启动器和 F1 菜单可用方向键 + A/B 导航。
- 触摸：启动器和 F1 菜单支持 Switch 触摸屏点按。
- 光盘：把 `.cue`/`.bin`/`.img` 放到 `sdmc:/switch/lain/discs/`。首次启动会校验并**原地读取**，
  不复制镜像、也不需要手动导入；首次在启动器按一次「开始游戏」后，以后直接进入游戏。
- 中文界面字体：压缩包自带 `fonts/ui.ttf`（文泉驿微米黑，Apache-2.0/GPLv3+字体例外），
  解压后位于 `sdmc:/switch/lain/fonts/ui.ttf`，中文不会再显示为问号。
- 字幕包：`lang/zh/` 已包含在整合包中，共 667 个节点。
