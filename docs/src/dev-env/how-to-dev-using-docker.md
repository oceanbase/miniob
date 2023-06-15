本篇文章介绍如何使用Docker来开发MiniOB。

MiniOB 依赖的第三方组件比较多，搭建开发环境比较繁琐，建议同学们直接使用我们提供的Docker环境进行开发。

首先要确保本地已经安装了Docker。
如果对Docker还不太熟悉，可以先在网上大致了解一下。

我们提供了原始的Dockerfile，也有已经打包好的镜像，可以选择自己喜欢的方式。
自行构建参考[本文档](./how_to_dev_in_docker_container_by_vscode.md)。

- 使用docker hub 镜像运行

```bash
docker run --privileged -d --name=miniob oceanbase/miniob
```
此命令会创建一个新的容器，然后可以执行下面的命令进入容器：

```bash
docker exec -it miniob /usr/bin/zsh
```

Docker环境说明：
docker基于`anolisos:8.6`制作。

镜像包含：

- jsoncpp
- google test
- libevent
- flex
- bison(3.7)
- gcc/g++ (version=11)
- miniob 源码(/root/source/miniob)

docker中在/root/source/miniob目录下载了github的源码，可以根据个人需要，下载自己仓库的源代码，也可以直接使用git pull 拉取最新代码。
/root/source/miniob/build.sh 提供了一个编译脚本，以DEBUG模式编译miniob。
