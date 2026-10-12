<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2026 dengyishan7034-creator -->

# MudRunner SoundSync

MudRunner（泥泞轮胎）联机引擎声同步模组。根据其他玩家的车型、引擎状态和位置播放声音，让附近车辆的点火、加速与熄火具有对应的声音反馈。

模组自动读取车辆和摄像机数据，直接使用本机游戏中的声音资源，无需手动配置车型或额外下载声音包。运行文件只有一个 `dinput8.dll`。

当前版本：**0.4.2**。

## 功能

- 自动识别玩家驾驶的车型和真实引擎开关。
- 同步车辆位置与发动机参数。
- 根据摄像机方向计算左右声道，并随距离衰减音量。
- 引擎关闭或远程状态超时后渐弱到静音。
- 自动解析车型定义、父级、声音模板和别名。
- 从游戏资源包或已配置的普通资源目录读取声音，在内存中解压和处理。
- 提供完整源码及构建工具。

## 运行要求

| 项目 | 要求 |
| --- | --- |
| 系统 | 64 位 Windows |
| 游戏 | MudRunner 221004，64 位 |
| 联机接口 | SteamNetworking005 |
| 玩家安装 | 参与同步的玩家均需安装，建议使用相同版本 |
| DLC 声音 | 收听方需要具有并配置对应的 DLC 资源 |

纯 Epic/EOS 联机接口暂不支持。其他游戏构建和所有第三方模组尚未全面验证。

<details>
<summary>已适配游戏文件的 SHA-256</summary>

```text
d030802772c9148a2e986abd99de94eda47b8ba92b94ee44cee8503cfefbd926
```

</details>

## 安装与更新

从 [Releases](https://github.com/dengyishan7034-creator/MudRunner-SoundSync/releases) 获取预编译安装包，或按下方步骤从源码构建。若尚无发行附件，请使用源码构建。

1. 退出游戏。
2. 备份游戏根目录已有的 `dinput8.dll`。
3. 将安装包中的 `dinput8.dll` 复制到 `MudRunner.exe` 所在目录，覆盖同名文件。
4. 其他玩家完成相同安装后，重新启动游戏并联机。

安装后无需手动选择车型。靠近其他玩家的车辆，确认点火、熄火和左右经过时的声音表现。

从旧版升级时，只需替换 `dinput8.dll`；旧的 `mr_native.dll` 和 `_mp_sync` 可以保留。保留现有 `d3d9.dll`，游戏归档和 `Config.xml` 无需修改。

卸载或回退时，退出游戏并恢复安装前备份的 `dinput8.dll`。详细步骤见 [安装方案.txt](安装方案.txt)。

## 从源码构建

准备以下工具：

- 64 位 Windows。
- 64 位 Python 3.9 或更新版本。
- 64 位 TinyCC；已验证的编译器版本为 0.9.27。

在仓库根目录运行，将编译器路径替换为实际位置：

```text
python tools/build.py --tcc "C:/tcc/tcc.exe"
```

生成的安装文件位于 `build/dinput8.dll`。构建无需原模组二进制或游戏文件；运行时需要自己的游戏安装。

### 运行测试

```text
python tools/build.py --tcc "C:/tcc/tcc.exe" --tests
python tests/test_standalone.py
python tests/test_game_audio.py
```

测试使用 Python 标准库、合成游戏内存和自行生成的音频资源，不需要游戏文件。声音输出检查需要可用的 Windows 音频设备。

0.4.2 已通过 67 项回归检查、72 组解压检查，以及双人吉普联机中的引擎声、左右定位和熄火验证。当前游戏资源中 36 种车型的声源定义已核对一致。更多验证范围见 [TESTING](docs/TESTING.md)。

## 常见问题

### 听不到其他玩家的引擎声

先确认双方均已安装并重启、对方引擎已开启，且车辆距离在播放范围内。DLC 车型还需要收听方具有对应资源。

游戏根目录的 `mr_sync.log` 可用于排查：

| 日志内容 | 含义 |
| --- | --- |
| `gameGuard=1` | 游戏构建通过适配检查 |
| `AUDIO output running` | 声音输出已打开 |
| `NET ready=1` | 网络接口已就绪 |
| `accepted` 计数增长 | 正在接收有效同步状态 |
| `AUDIO model=... loaded` | 对应车型声音已加载 |
| `camera=1` | 原生摄像机数据有效 |

### 声音没有左右方向

检查日志中的 `camera` 状态。摄像机读取暂不可用时，模组会使用本地车辆位置计算距离，并将声音居中播放。

### 如何报告问题

请在 [Issues](https://github.com/dengyishan7034-creator/MudRunner-SoundSync/issues) 中提供模组版本、游戏构建、双方车型、问题发生时的操作，以及双方对应的 `mr_sync.log`。

## 当前限制

- 最多混合 8 个远程声源，播放范围为 200 米。
- 音调由归一化发动机参数估算，尚未校准为真实 RPM。
- 资源读取支持普通目录、ZIP32 的 stored/deflate 方法及 16 bit PCM WAV；ZIP64、加密归档、特殊 MountPoint 和其他音频编码暂不支持。
- 第三方车型能否自动读取，取决于其资源定义是否采用支持的格式。
- 多玩家长期运行、其他游戏构建及全部第三方模组尚未全面验收。

## 开发与许可

实现包含车辆状态读取、Steam 状态同步、原生摄像机采集、游戏资源解析和独立混音输出。模块结构见 [ARCHITECTURE](docs/ARCHITECTURE.md)。

本项目原创代码、工具、测试及说明采用 **GNU Affero General Public License v3.0 only（AGPL-3.0-only）**，全文见 [LICENSE](LICENSE)。游戏资源和第三方运行库不随项目分发，也不由本项目重新授权。

源码获取和许可说明见 [SOURCE_OFFER.txt](SOURCE_OFFER.txt) 与 [LICENSING](docs/LICENSING.md)。
