# how to use
# use docker build: docker build -t miniob .
# use docker compose: docker compose up -d --build
# make sure docker has been installed
FROM ubuntu:24.04

ENV LANG en_US.utf8

RUN apt-get update && apt-get install -y locales apt-utils && rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8

# # Change sources to Aliyun
# RUN cp /etc/apt/sources.list.d/ubuntu.sources /etc/apt/sources.list.d/ubuntu.sources.bak \
#     && echo 'Types: deb' > /etc/apt/sources.list.d/ubuntu.sources \
#     && echo 'URIs: http://mirrors.aliyun.com/ubuntu/' >> /etc/apt/sources.list.d/ubuntu.sources \
#     && echo 'Suites: noble noble-updates noble-security' >> /etc/apt/sources.list.d/ubuntu.sources \
#     && echo 'Components: main restricted universe multiverse' >> /etc/apt/sources.list.d/ubuntu.sources \
#     && echo 'Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg' >> /etc/apt/sources.list.d/ubuntu.sources \
#     && apt-get update

RUN apt-get update \
    && apt-get install -y build-essential gdb cmake git wget flex texinfo libreadline-dev diffutils bison \
    && apt-get install -y clang-format vim sudo

RUN apt-get install -y openssh-server

# Try cloning from GitHub, if it fails, use cnpmjs.org mirror
RUN git clone https://github.com/oceanbase/miniob /tmp/miniob || \
    git clone https://githubfast.com/oceanbase/miniob /tmp/miniob \
    && cd /tmp/miniob \
    && THIRD_PARTY_INSTALL_PREFIX=/usr/local bash build.sh init \
    && mkdir -p /root/docker/bin \
    && touch /etc/.firstrun \
    && cp docker/bin/* /root/docker/bin/ \
    && rm -rf /tmp/miniob

RUN mkdir /var/run/sshd

RUN apt-get install -y zsh \
    && mkdir ~/.oh-my-zsh \
    && git clone https://gitee.com/mirrors/ohmyzsh.git ~/.oh-my-zsh \
    && cp ~/.oh-my-zsh/templates/zshrc.zsh-template ~/.zshrc \
    && sed -i "s/robbyrussell/bira/" ~/.zshrc \
    && usermod --shell /bin/zsh root \
    && echo "export LD_LIBRARY_PATH=/usr/local/lib64:\$LD_LIBRARY_PATH" >> ~/.zshrc

RUN chmod +x /root/docker/bin/*

WORKDIR /root

ENTRYPOINT /root/docker/bin/starter.sh
