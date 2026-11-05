#include "i18n.h"

#include <SDL.h>
#include <string.h>

typedef struct {
    const char *en;
    const char *zh;
} Entry;

/* UI strings, newest translations first is not needed: lookup is linear over a
 * few hundred entries, once per label per frame. */
static const Entry kEntries[] = {
    /* Launcher */
    {"ready", "已就绪"},
    {"not imported", "未导入"},
    {"Import disc 1 first (Setup tab).", "请先在「设置」标签页导入光盘 1。"},
    {"Reading your discs...", "正在读取光盘…"},
    {"Try again", "重试"},
    {"Things on the Serial Experiments Lain discs that the game never shows, decoded from your own discs. Found while decompiling the game.",
     "《Serial Experiments Lain》光盘中存在、但游戏从未展示的内容，由你自己的光盘镜像解析而来。这些内容是反编译游戏时发现的。"},
    {"Stop", "停止"},
    {"Play sound", "播放"},
    {"(%.1f s)", "（%.1f 秒）"},
    {"(%zu frames)", "（%zu 帧）"},
    {"Serial Experiments Lain - Setup", "Serial Experiments Lain - 设置"},
    {"setup window: %s\n", "设置窗口：%s\n"},
    {"Japanese (original)", "日语（原版）"},
    {"Native client. Game data comes from your own discs.", "原生客户端。游戏数据来自你自己的光盘。"},
    {"Setup", "设置"},
    {"Discs", "光盘"},
    {"Disc 1 (Site A): %s", "光盘 1（Site A）：%s"},
    {"Disc 2 (Site B): %s", "光盘 2（Site B）：%s"},
    {"Disc folder: %sdiscs/", "光盘目录：%sdiscs/"},
    {"Rescan discs", "重新扫描光盘"},
    {"Disc scan finished.", "光盘扫描完成。"},
    {"Disc 1 not found: put .cue/.bin/.img in %sdiscs/ and press Rescan discs.",
     "未找到光盘 1：请把 .cue、.bin 或 .img 放到 %sdiscs/，然后按「重新扫描光盘」。"},
    {"Disc 1 not found: put your .img in %sdiscs/.",
     "未找到光盘 1：请把你的 .img 放到 %sdiscs/。"},
    {"Add disc...", "添加光盘…"},
    {"Choose a Serial Experiments Lain disc", "选择 Serial Experiments Lain 光盘"},
    {"Disc image (.cue, .bin, .img)", "光盘镜像（.cue、.bin、.img）"},
    {"or drop .cue, .bin or .img files on this window. Either disc, any order.",
     "或将 .cue、.bin 或 .img 文件拖到此窗口。任一张光盘、顺序不限。"},
    {"Importing and verifying...", "正在导入并校验…"},
    {"Display", "显示"},
    {"Windowed", "窗口"},
    {"Fullscreen", "全屏"},
    {"Window size", "窗口大小"},
    {"Render scale", "渲染倍率"},
    {"Auto", "自动"},
    {"(%d x %d internal)", "（内部 %d x %d）"},
    {"(window resolution; 1x = original pixels)", "（窗口分辨率；1x = 原始像素）"},
    {"VSync", "垂直同步"},
    {"Smooth motion (60 fps)", "平滑运动（60 fps）"},
    {"12/30 fps screens get in-between frames; game speed unchanged",
     "12/30 fps 的画面会补插中间帧；游戏速度不变"},
    {"Language", "语言"},
    {"Interface language", "界面语言"},
    {"Auto (system)", "自动（跟随系统）"},
    {"Text / subtitles", "文本／字幕"},
    {"Voices", "语音"},
    {"Add subtitle folder...", "添加字幕文件夹…"},
    {"Folder with .ass or .srt subtitle files", "包含 .ass 或 .srt 字幕文件的文件夹"},
    {"Working...", "处理中…"},
    {"Packs live in: %slang", "语言包位于：%slang"},
    {"Mods", "模组"},
    {"Unused", "未使用内容"},
    {"Play game", "开始游戏"},
    {"Quit", "退出"},
    {"Import disc 1 to play. Disc 2 is needed for Site B.",
     "导入光盘 1 后才能开始。Site B 需要光盘 2。"},

    /* In-game F1 menu */
    {"menu focus: %s (frame %d)", "菜单焦点：%s（第 %d 帧）"},
    {"Settings (F1)", "设置（F1）"},
    {"Settings", "设置"},
    {"(in-between frames; game speed unchanged)", "（补插中间帧；游戏速度不变）"},
    {"(original frame rate)", "（原始帧率）"},
    {"%d x %d internal; 1x = original pixels", "内部 %d x %d；1x = 原始像素"},
    {"window resolution", "窗口分辨率"},
    {"Sound", "声音"},
    {"Volume", "音量"},
    {"Controls", "按键"},
    {"Up", "上"},
    {"Down", "下"},
    {"Left", "左"},
    {"Right", "右"},
    {"Cross", "×（叉）"},
    {"Circle", "○（圈）"},
    {"Square", "□（方）"},
    {"Triangle", "△（三角）"},
    {"Start", "开始"},
    {"Select", "选择"},
    {"Click a button, then press a key (Esc cancels).", "点击一个按钮，然后按键（Esc 取消）。"},
    {"press a key...", "请按键…"},
    {"Default keys", "默认按键"},
    {"Cheats", "作弊"},
    {"Cheats change how the game runs, not your saves.", "作弊只改变游戏运行方式，不会修改存档。"},
    {"Open any node", "解锁所有节点"},
    {"(ignore the progress a node needs)", "（忽略节点所需的进度）"},
    {"Turbo (hold Tab)", "加速（按住 Tab）"},
    {"Off", "关闭"},
    {"Movies and voices keep their normal speed.", "影片和语音保持正常速度。"},
    {"Genome save title", "基因组存档标题"},
    {"(normally 1 save in 70)", "（正常情况下约 70 次存档出现 1 次）"},
    {"Lain's hidden stats", "Lain 的隐藏数值"},
    {"Since the last load; save titles show one.", "自上次读档以来；存档标题会显示其中一项。"},
    {"Heart-flutter  %3d%%", "心动  %3d%%"},
    {"nodes opened", "已打开节点"},
    {"Dejection      %3d%%", "失落      %3d%%"},
    {"Lain's reactions", "Lain 的反应"},
    {"Harumage       %3d%%", "春闹       %3d%%"},
    {"empty cells pressed", "按下空格"},
    {"Poison waves   %3d ", "毒素波   %3d "},
    {"screensavers", "屏保次数"},
    {"No mods are on.", "未启用任何模组。"},
    {"%d file%s", "%d 个文件"},
    {", %d replaced by later mods", "，%d 个被后续模组覆盖"},
    {", %d skipped", "，%d 个被跳过"},
    {"Mods are chosen on the launcher's Mods tab and apply when the game starts.",
     "模组在启动器的「模组」标签页选择，游戏启动时生效。"},
    {"Resume", "继续"},
    {"Quit game", "退出游戏"},
    {"Esc / F1 to close", "Esc／F1 关闭"},

    /* Mods tab */
    {"by %s", "作者：%s"},
    {"Folder: mods/%s", "文件夹：mods/%s"},
    {"Files", "文件"},
    {"Nothing in this mod yet. Replacement files go in folders named after the",
     "此模组还没有内容。替换文件放在以归档名命名的文件夹中"},
    {"archive (SITEA.BIN/0631.png), a script in main.lua. See MODDING.md.",
     "（如 SITEA.BIN/0631.png），脚本放在 main.lua。详见 MODDING.md。"},
    {"checking...", "校验中…"},
    {"not used", "未使用"},
    {"used", "已使用"},
    {"Texture pack: %d HD texture%s", "高清材质包：%d 张高清贴图"},
    {"Code", "代码"},
    {"Script: main.lua (%d Lua file%s)", "脚本：main.lua（%d 个 Lua 文件）"},
    {"Lua files without a main.lua (not run)", "没有 main.lua 的 Lua 文件（不会运行）"},
    {"Plugin: %s", "插件：%s"},
    {"Script and plugin messages appear in the F1 menu's Mods tab.",
     "脚本和插件消息显示在 F1 菜单的「模组」标签页。"},
    {"Ignored (not named like a replacement):", "已忽略（命名不符合替换规则）："},
    {"Mods replace pictures, text, voices and other game files with your own. ",
     "模组用你自己的文件替换图片、文本、语音和其他游戏文件。"},
    {"They are applied when you press Play; your discs are never changed.",
     "按下「开始游戏」时生效；你的光盘镜像不会被修改。"},
    {"Open mods folder", "打开模组文件夹"},
    {"Refresh", "刷新"},
    {"New mod...", "新建模组…"},
    {"Mod name", "模组名称"},
    {"Create", "创建"},
    {"Created mods/%s. Add replacement files to it.", "已创建 mods/%s。请向其中添加替换文件。"},
    {"Cancel", "取消"},
    {"Export originals", "导出原始文件"},
    {"Writes every picture, voice and file of your discs to mods/_originals,\nas files a mod can replace. Copy the ones you want to change into your mod.",
     "把你的光盘中的每张图片、每段语音和每个文件写入 mods/_originals，\n作为模组可替换的文件。将想修改的文件复制到你的模组中。"},
    {"Exporting originals...", "正在导出原始文件…"},
    {"Exported %d files to mods/_originals.", "已导出 %d 个文件到 mods/_originals。"},
    {"Open", "打开"},
    {"Export failed: %s", "导出失败：%s"},
    {"Import disc 1 on the Setup tab to check and use mods.",
     "请先在「设置」标签页导入光盘 1，才能校验和使用模组。"},
    {"Save the game's textures while playing", "游戏运行时保存贴图"},
    {"Every picture the game draws is saved to the texture_dump folder, named for a\ntexture pack: upscale one, keep its name, put it in a mod's textures folder.",
     "游戏绘制的每张图片都会保存到 texture_dump 文件夹，并按材质包规则命名：\n放大其中一张、保持文件名不变，放入模组的 textures 文件夹。"},
    {"No mods installed. Put a mod's folder in the mods folder, or make one with New mod.",
     "未安装模组。将模组文件夹放入 mods 文件夹，或使用「新建模组」。"},
    {"port/MODDING.md explains how mods work.", "port/MODDING.md 说明了模组的用法。"},
    {"Load order: when two mods change the same file, the lower one wins.",
     "加载顺序：两个模组修改同一文件时，靠下的生效。"},
    {"%d problem%s", "%d 个问题"},
    {"%d note%s", "%d 条提示"},
    {"Move up", "上移"},
    {"Move down", "下移"},
    {"Open folder", "打开文件夹"},

    /* Unused gallery */
    {"Couldn't read the discs: ", "无法读取光盘："},
    {"Couldn't read disc 1 (import it first).", "无法读取光盘 1（请先导入）。"},
    {"Lain's unused animations", "Lain 未使用的动画"},
    {"LAPKS.BIN holds 59 packs of Lain's animated sprite. No code path of the game ever loads these 11. Their first and last frames are her usual standing pose, and some share frames with animations the game does play; the rest of each is never seen.",
     "LAPKS.BIN 中有 59 组 Lain 的动画。游戏的代码从未加载这 11 组。它们的首尾帧是她常见的站姿，部分帧也出现在游戏实际使用的动画中；其余部分从未出现过。"},
    {"Fidgets with her hands, then winks.", "摆弄双手，然后眨眼。"},
    {"Head turn / look around (1 of 4).", "转头／环顾（1/4）。"},
    {"Head turn / look around (2 of 4).", "转头／环顾（2/4）。"},
    {"Head turn / look around (3 of 4).", "转头／环顾（3/4）。"},
    {"Head turn / look around (4 of 4).", "转头／环顾（4/4）。"},
    {"A full turn-around. About 40% of its frames also appear in animations the game plays.",
     "完整转身。约 40% 的帧也出现在游戏实际使用的动画中。"},
    {"Another full turn-around. About 40% of its frames also appear in animations the game plays.",
     "另一个完整转身。约 40% 的帧也出现在游戏实际使用的动画中。"},
    {"Looks aside with her eyes closed. About half its frames also appear in animations the game plays.",
     "闭眼看向一旁。约一半的帧也出现在游戏实际使用的动画中。"},
    {"A gesture with raised hands. About 40% of its frames also appear in a similar gesture the game plays.",
     "举手的手势。约 40% 的帧也出现在游戏实际使用的类似手势中。"},
    {"Another raised-hands gesture. About 40% of its frames also appear in animations the game plays.",
     "另一个举手手势。约 40% 的帧也出现在游戏实际使用的动画中。"},
    {"Raises her arm, surprised, then points.", "惊讶地抬起手臂，然后指向某处。"},
    {"Pack %d", "动画包 %d"},
    {"Unused photos", "未使用的照片"},
    {"Pictures in SITEA.BIN that no node, slideshow or screen of the game ever loads.",
     "SITEA.BIN 中没有任何节点、幻灯片或画面加载过的图片。"},
    {"SITEA 631", "SITEA 631"},
    {"Two boys on a sports field, one in a #3 baseball shirt.",
     "运动场上的两个男孩，其中一个穿着 3 号棒球服。"},
    {"SITEA 651", "SITEA 651"},
    {"A building entrance with stairs, at night.", "夜晚，一栋建筑的入口和楼梯。"},
    {"SITEA 744", "SITEA 744"},
    {"An orange building under power lines.", "电线下的橙色建筑。"},
    {"SITEA 787", "SITEA 787"},
    {"A spare mouth frame for Lain's name-calling face.", "Lain 喊名字表情的一张备用嘴部帧。"},
    {"Unused sounds", "未使用的声音"},
    {"Sounds on the disc that the game never plays.", "光盘中存在、但游戏从未播放的声音。"},
    {"Sound effect (bank sample 3)", "音效（音色库采样 3）"},
    {"In the sound bank as programs 2 and 3; no sound call, animation or music uses them.",
     "在音色库中为程序 2 和 3；没有任何音效调用、动画或音乐使用它们。"},
    {"Sound effect (bank sample 23)", "音效（音色库采样 23）"},
    {"Program 22 of the sound bank; never triggered.", "音色库的程序 22；从未被触发。"},
    {"Name voice: word endings", "名字语音：词尾"},
    {"A_END ... U_END: samples for the voice that calls your name, which the code never asks for.",
     "A_END … U_END：用于呼唤玩家名字的语音采样，但代码从未请求它们。"},
    {"Name voice: pi pu pe po", "名字语音：pi pu pe po"},
    {"A bug drops pi, pu, pe and po from the name, so Lain never says these.",
     "一个程序错误使名字中的 pi、pu、pe、po 被丢弃，所以 Lain 从不说这些音。"},
};

static int s_zh;

void i18n_set_language(const char *code) {
    if (!code || !*code || SDL_strcasecmp(code, "auto") == 0) {
        /* This fork defaults to Chinese when the system locale is unknown
         * (common on the Switch). */
        s_zh = 1;
        SDL_Locale *loc = SDL_GetPreferredLocales();
        if (loc && loc[0].language) {
            s_zh = SDL_strcasecmp(loc[0].language, "zh") == 0;
        }
        if (loc) {
            SDL_free(loc);
        }
        return;
    }
    s_zh = SDL_strcasecmp(code, "zh") == 0;
}

const char *i18n_language(void) {
    return s_zh ? "zh" : "en";
}

int i18n_is_zh(void) {
    return s_zh;
}

const char *tr(const char *en) {
    if (!en || !s_zh) {
        return en;
    }
    for (size_t i = 0; i < sizeof kEntries / sizeof kEntries[0]; i++) {
        if (strcmp(kEntries[i].en, en) == 0) {
            return kEntries[i].zh;
        }
    }
    return en;
}
