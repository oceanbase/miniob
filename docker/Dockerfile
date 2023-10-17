# how to use
# use docker build: docker build -t miniob .
# use docker compose: docker compose up -d --build
# make sure docker has been installed
FROM ubuntu:latest

# ENV LANG=en_US.UTF-8
# locale
RUN apt-get update && apt-get install -y locales apt-utils && rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG en_US.utf8

# dev tools
RUN apt-get update && apt-get install -y build-essential gdb cmake git wget flex texinfo libreadline-dev diffutils bison

# install openssh
RUN apt-get install -y openssh-server

# init miniob dependencies
RUN git clone https://github.com/oceanbase/miniob /tmp/miniob \
    && cd /tmp/miniob \
    && bash build.sh init \
    && rm -rf /tmp/miniob

RUN mkdir /var/run/sshd

# install zsh and on-my-zsh
RUN apt-get install -y zsh \
    && git clone https://gitee.com/mirrors/ohmyzsh.git ~/.oh-my-zsh \
    && cp ~/.oh-my-zsh/templates/zshrc.zsh-template ~/.zshrc \
    && sed -i "s/robbyrussell/bira/" ~/.zshrc \
    && usermod --shell /bin/zsh root \
    && echo "export LD_LIBRARY_PATH=/usr/local/lib64:\$LD_LIBRARY_PATH" >> ~/.zshrc

RUN mkdir -p /root/docker/bin && touch /etc/.firstrun

# copy starter scripts
COPY bin/* /root/docker/bin/

RUN chmod +x /root/docker/bin/*

WORKDIR /root

ENTRYPOINT /root/docker/bin/starter.sh
