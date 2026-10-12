<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2026 dengyishan7034-creator -->

# 构成

| 源码 | 职责 |
| --- | --- |
| proxy.c | 系统 DirectInput 转发、日志与启动/停止 |
| game_state.c | 已验证游戏布局的只读适配和构建特征检查 |
| network.c | 游戏拥有的 Steam 接口发现、原始收发转发 |
| state_sync.c | MRA3 包校验、10 Hz 同步、会话/序列过滤 |
| hooks.c、camera.c | 网络方法指针转发、原生摄像机矩阵与监听位置 |
| model_sounds.c | 运行时车型、父级、模板与声音别名解析 |
| game_audio.c | 自写 ZIP32/DEFLATE/CRC/PCM16 读取器 |
| mixer.c、win_audio.h | 自有缓存、立体声混音与 Windows 输出 |

没有原主 DLL 的 RVA、混音函数或状态结构依赖。系统 dinput8.dll 只作为 Windows 正常运行库转发对象。

复用现有 Steam 接口，不自行初始化 Steam、不调用游戏的 Steam 退出函数、不关闭游戏会话。除通道 7 的状态之外，游戏数据和返回值原样转发。

参考：[Steam Networking](https://partner.steamgames.com/doc/api/ISteamNetworking)、[waveOutOpen](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutopen)。资源格式依据 RFC 1951 和 PKWARE ZIP 格式说明。

摄像机取自 game+0xbb8220 的渲染器对象（匹配 vtable game+0xa4a528）及其 +8 摄像机成员；矩阵连读一致并检查正交/有限范围后使用。游戏 getter 指令特征纳入构建保护。线程只读，不调用游戏的图形接口。
