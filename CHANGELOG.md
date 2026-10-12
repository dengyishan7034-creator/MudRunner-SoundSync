<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2026 dengyishan7034-creator -->

# 0.4.2 — 2026-10-13

- 完整源码替换原主 DLL，输出一个 dinput8.dll。
- 自动解析游戏声源，移除 mr_native.dll 与索引依赖。
- 自有混音、声音缓存、原生摄像机只读采集和 Steam 接口发现。
- 增加网络转发、断线静音、定位和系统输出检查。

- 修复 Steam/EOS 覆盖层的输入接口解析递归：提前绑定系统 DLL 的原生导出，并加入拦截解析器模拟回归。

- 用已验证的渲染器摄像机视图代替图形接口挂接，兼容实际摄像机方向与滚转，移除 GetProcAddress/IAT 和 Direct3D 观察依赖。
