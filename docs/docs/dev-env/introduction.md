---
title: 开发环境介绍
---

# 搭建开发环境

每个人的开发环境各不相同，这里介绍了 Linux/MacOS/Windows 三类操作系统上搭建 MiniOB 的开发环境，以及推荐使用的IDE（VSCode）的调试方法。
首先，大家需要清楚 MiniOB 当前只能在 Linux/MacOS 操作系统上编译。因此，如果是使用 Windows 操作系统的同学，需要首先准备 Linux 的虚拟环境（例如，WSL2，Docker，虚拟机等）。
下面提供了多种开发环境的搭建方法，请大家根据自己的实际情况选择合适的文档进行学习阅读。
## 在线开发环境
在线开发环境的优点是无需安装任何软件，开箱即用，几乎是一键初始化完成开发环境。但是在网络受限时，可能无法使用，或者体验比较差。如果希望使用在线开发环境的同学可以参考[使用在线开发环境开发 MiniOB](dev_by_online.md)。
## Linux/MacOS 开发环境
对于使用 Linux/MacOS 操作系统的同学，可以参考[使用 VSCode 开发 MiniOB](./how_to_dev_miniob_by_vscode.md)。
## Windows 开发环境
对于使用 Windows 操作系统的同学，需要先准备 Linux 的虚拟环境（例如，WSL2，Docker，虚拟机等），之后才能在 Linux 的虚拟环境上开发 MiniOB。

使用 Docker 容器开发，可以参考[使用 Docker 开发 MiniOB](how-to-dev-using-docker.md)或[Windows 使用Docker开发MiniOB](how_to_dev_miniob_by_docker_on_windows.md)或热心同学的[手把手教你在windows上用docker和vscode配置环境](how_to_dev_in_docker_container_by_vscode_on_windows.md)。

上述三篇文档的详细程度依次递增，可以根据实际情况选择阅读。
## 远程开发环境
对于有远程开发环境（例如云服务器等）的同学不受本地操作系统的限制，可以参考：[开发环境搭建(远程调试)](how_to_dev_in_docker_container_by_vscode.md)
## 调试（Debug）介绍
miniob 的调试（debug）方法可以参考[MiniOB 调试](miniob-how-to-debug.md)。
