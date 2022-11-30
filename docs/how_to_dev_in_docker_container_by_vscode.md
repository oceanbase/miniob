# 使用 Docker 和 VSCode 远程开发 Miniob

本文档阐述如何使用 Docker 构建一个具备 Miniob 开发环境的容器，并且通过 VSCode 的 Remote-SSH 插件 SSH 到容器中进行远程开发。Docker 和 VSCode 可以安装在不同机器上。

方法简单易行，仅需配置 2 个 环境变量（仓库 URL、SSH秘钥）及安装必要的 VSCode 开发插件即可。

- [使用 Docker 和 VSCode 远程开发 Miniob](#使用-docker-和-vscode-远程开发-miniob)
  - [安装 Docker 和 Docker-compose](#安装-docker-和-docker-compose)
    - [Windows](#windows)
    - [Linux](#linux)
  - [安装 VSCode](#安装-vscode)
  - [配置 Dockerfile](#配置-dockerfile)
    - [配置 Root 密码](#配置-root-密码)
  - [配置 Docker Compose](#配置-docker-compose)
    - [Fork 仓库](#fork-仓库)
    - [Clone by SSH](#clone-by-ssh)
    - [Clone by HTTPS](#clone-by-https)
      - [私有仓库](#私有仓库)
      - [公有仓库](#公有仓库)
    - [配置映射端口](#配置映射端口)
  - [启动容器](#启动容器)
    - [Clone by SSH](#clone-by-ssh-1)
      - [Linux Shell](#linux-shell)
      - [Windows Powershell](#windows-powershell)
    - [Clone by HTTPS](#clone-by-https-1)
    - [管理容器](#管理容器)
  - [使用 VSCode 远程开发](#使用-vscode-远程开发)
    - [安装 Remote-SSH 插件](#安装-remote-ssh-插件)
    - [连接 Docker 容器](#连接-docker-容器)
      - [配置 SSH Host](#配置-ssh-host)
      - [连接容器](#连接容器)
    - [打开代码目录](#打开代码目录)
    - [安装开发插件](#安装开发插件)
    - [进行 Debug](#进行-debug)
      - [启动 Server](#启动-server)
      - [启动 Client](#启动-client)
      - [执行 SQL](#执行-sql)


## 安装 Docker 和 Docker-compose

### Windows

参考本仓库文档 [在 Windows 上使用 Docker](how_to_dev_miniob_by_docker_on_windows.md) 安装 Docker Desktop 即可。只需执行 **安装 Docker** 这一小节。

### Linux

如果你选择在远程建议参考官方文档安装 [Docker Engine](https://docs.docker.com/engine/install/) 和 [Docker Compose](https://docs.docker.com/compose/install/linux/).

## 安装 VSCode

前往官网下载 [Visual Studio Code](https://code.visualstudio.com/)，正常安装即可。

## 配置 Dockerfile

文件在 docker/Dockerfile.

### 配置 Root 密码

默认密码 root，可在 64 行自行修改。不需修改可跳过。

若镜像于服务器运行建议使用强密码，并在第一次登录后关闭密码登录，改用秘钥登录。

## 配置 Docker Compose

### Fork 仓库

Fork 本仓库，复制 Fork 后仓库的 HTTPS 地址，如 https://github.com/oceanbase/miniob.git. 

也可以不 Fork，用本仓库 Git HTTPS 地址，后续自行进行 `git remote add`. 

下面配置镜像中的仓库 URL 及克隆方式。

修改 **docker/docker-compose.yml** 文件的最后两行。注意等号前后不要有空格。

### Clone by SSH

`REPO_ADDR` 设置为 SSH URL, 如：git@github.com:oceanbase/miniob.git

### Clone by HTTPS

#### 私有仓库

如果 fork 出来的是私有仓库。

假设仓库 HTTPS 地址为：https://github.com/oceanbase/miniob.git.   

**REPO_ADDR 格式**：https://\<username\>:\<password\>@github.com/oceanbase/miniob.git

 `username` 和 `password` GitHub / Gitee(GitHub 不支持 password，需要创建 token，见[链接](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token))。  
 
 **用户名如果是邮箱，字符 '@' 需要转义为 '%40'**.

```docker
# example
REPO_ADDR=https://oceanbase%40email.com:mypassword@github.com/oceanbase/miniob.git
```

#### 公有仓库

如果 fork 出的是公有仓库，直接将 `REPO_ADDR` 设置为仓库地址即可。

```Docker
REPO_ADDR=https://github.com/oceanbase/miniob.git
```

### 配置映射端口

默认映射本机端口 10000 到容器端口 22. 可在 docker-compose.yml 中自行修改。不许修改可跳过此步骤。

## 启动容器 

### Clone by SSH

如果用 SSH 的方式 clone，需要将 Github/Gitee 对应的 SSH 私钥设置到环境变量内再启动 Container.

**假设 Github/Gitee 上传的公钥对应的秘钥位于 ~/.ssh/id_rsa.**

terminal 进入本仓库 docker 目录，在 docker 目录下执行命令启动容器。

#### Linux Shell

```bash
export PRIVATE_KEY=$(cat ~/.ssh/id_rsa) && docker-compose up -d --build
```

#### Windows Powershell

```bash
$env:PRIVATE_KEY=$(cat ~/.ssh/id_rsa) && docker-compose up -d --build
```

### Clone by HTTPS

不需要 PRIVATE_KEY 环境变量。

```bash
docker-compose up -d --build
```

此输出表示容器启动成功。

![container started](./images/vsc_container_started.png)

### 管理容器

一些管理容器的命令。

```bash
# 停止容器
docker stop miniob-dev

# 启动容器
docker start miniob-dev

# 重启容器
docker restart miniob-dev
```

## 使用 VSCode 远程开发

打开 VSCode，设置远程开发及调试。

### 安装 Remote-SSH 插件

一般会默认安装。快捷键组合 Ctrl + Shift + x 打开 VSCode 插件面板，检查 Remote - SSH 插件是否安装，如果没有安装就安装此插件。

![remote ssh](./images/vsc_remote_ssh_extension.png)

### 连接 Docker 容器

#### 配置 SSH Host

按快捷键 Ctrl + p, 输入 `remote-ssh: connect to host` 连接远程主机。

![connect to host](./images/vsc_remote_ssh_connect_cmd.png)

选择 `Add new ssh host`.

![add ssh host](./images/vsc_add_new_ssh_host.png)

输入命令.

```bash
ssh root@localhost -p 10000
```

以 root 用户连接容器的 10000 端口.

![ssh cmd](./images/vsc_ssh_connect.png)

选择写入哪个配置文件，默认第一个就行。

![ssh config ifle](./images/vsc_config_file.png)

#### 连接容器

按快捷键 Ctrl + p, 输入 `remote-ssh: connect to host` 连接远程主机。

主机选 localhost 并回车，跳出密码输入页面。若在之前步骤没有修改，密码默认为 root.

![password](./images/vsc_pwd.png)

### 打开代码目录

代码位于 /root/source/miniob. 点击左侧 `open folder`，进入该目录。需要再输入一次密码。

![open folder](./images/vsc_open_folder.png)

### 安装开发插件

 Ctrl + Shift + x 打开 VSCode 插件面板, 安装如下插件. 注意要安装在 Docker 容器中，点击 `install in SSH:localhost`.

C/C++ 和 C/C++ Extension Pack.

![ext1](./images/vsc_cpp_extensions.png)

C/C++ Extension Pack 安装好后会让你选择 kits, 选 GCC 8.5 这个。

![kits](./images/vsc_kit_for_miniob.png)

CMake 和 CMake Tools.

![ext2](./images/vscode_search_plugs.png) 

### 进行 Debug

#### 启动 Server

在 src/observer/sql/parser/parse_stage.cpp 的 handle_event 函数开头打个断点, 也就是第 90 行。

按 F5 开始调试。出现如下界面表示 miniob 的 server 端启动成功。

![server started](./images/vsc_server_started.png) 

#### 启动 Client

Ctrl + Shift + ` 新启动一个 shell.

执行 `./build/bin/obclient` 启动客户端.

#### 执行 SQL

执行一句 SQL 测试断点是否正常工作。

```sql
show tables;
```

![debug](./images/vsc_debug.png)

正常进入断点。后续可以配置 Git 进行开发了。