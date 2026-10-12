<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2026 dengyishan7034-creator -->

# MudRunner SoundSync 0.4.2

**完全由本仓库源码编译的单 DLL 实现。** 不需要原主 DLL、二进制补丁、mr_native.dll、声源索引或随包 WAV。

源码地址：https://github.com/dengyishan7034-creator/MudRunner-SoundSync

## 功能与实现

- 自动读取实际驾驶车型、引擎开关、车辆标识和物理坐标。
- 复用游戏已初始化的 SteamNetworking005 接口；观察游戏自身的收发来发现玩家，约每 100 ms 在通道 7 同步完整状态。
- 运行时解析本机车型 XML、父级、声音模板和别名，直接读取 Config.xml 中普通 ZIP/目录的游戏声音，在内存中解压与转换 PCM。
- 自有立体声混音器：左右定位、距离衰减、参数平滑、熄火渐弱及 3 秒超时静音。
- 独立音频线程、约 70 ms 输出队列、有 64 MiB 上限的缓存；未使用的音频可以回收。
- DirectInput 代理提前绑定 Windows 系统原生导出，兼容解析接口拦截；直接只读获取游戏渲染器的最终视图矩阵，不挂接图形接口。

## 编译与检查

Windows x64、64 位 Python 3.9+、64 位 TinyCC（本次 0.9.27）。Python 检查只用标准库。

~~~~text
python tools/build.py --tcc "C:/tcc/tcc.exe" --tests
python tests/test_standalone.py
python tests/test_game_audio.py
~~~~

摄像机检查使用合成游戏内存，不创建图形设备。所有模组 DLL 均从 src/ 编译，构建不读取原模组 DLL 或游戏文件。检查范围见 [TESTING](docs/TESTING.md)。

## 安装

双方退出游戏，备份根目录旧 dinput8.dll，再将 build/dinput8.dll 复制到 MudRunner.exe 所在目录。只更新一个 DLL。

旧 mr_native.dll、_mp_sync 可以保留，独立版不会加载它们。不需要删除现有 d3d9.dll、修改游戏归档或 Config.xml。双方应使用同一版本；安装步骤见 [安装方案.txt](安装方案.txt)。

## 实测

2026-10-13：0.4.2 已验证真实双人吉普联机，用户确认对方引擎声、左右方向和熄火均正常。67 项回归与 72 组解压检查通过。原主 DLL 已完全移除，发布文件与实测文件校验一致。

## 支持边界

适配本次检查的 MudRunner 221004 x64 布局，游戏 SHA-256：

~~~~text
d030802772c9148a2e986abd99de94eda47b8ba92b94ee44cee8503cfefbd926
~~~~

当前资源中 36 种车型的声音定义已核对一致；新增符合支持格式的车型也在运行时解析。游戏虚拟文件系统的全部语义并未实现，特殊 MountPoint、ZIP64、加密 ZIP、所有第三方模板写法及 PCM16 以外编码暂未全面支持。收听 DLC 声音需要本机具备对应资源。

仅实现游戏本次使用的 SteamNetworking005 接口。纯 Epic/EOS 未实现。最大 8 个远程声音、200 米范围。摄像机未捕获时先以本地车辆位置计算距离并居中播放，日志 camera=0 指出这一状态。

音调来自归一化发动机参数估算，尚未校准为真实 RPM；声音文件来自游戏，混合与输出由本模组完成。

## 授权与发布

所有本仓库原创实现、工具与测试采用 AGPL-3.0-only，全文见 [LICENSE](LICENSE)。没有原主 DLL 或其二进制片段参与构建。游戏与系统/Steam 运行库仍由各自提供者授权，本仓库不包含或重新授权它们，见 [LICENSING](docs/LICENSING.md)。

向玩家提供实际版本的免费源码入口，网络修改版的适用义务见 [SOURCE_OFFER.txt](SOURCE_OFFER.txt)。本地打包不代表源码已上传 GitHub。
