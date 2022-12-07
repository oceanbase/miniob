# how to use
# docker build -t miniob .
# make sure docker has been installed
# FROM rockylinux:8
FROM openanolis/anolisos:8.6

ARG HOME_DIR=/root
ARG DOCKER_CONFIG_DIR=${HOME_DIR}/docker

ENV LANG=en_US.UTF-8

# install rpm
# note: gcc-c++ in rockylinux 8 and gcc-g++ in rockylinux 9. use `dnf groupinfo "Development Tools"` to list the tools
RUN dnf install -y make cmake git wget which flex gdb gcc gcc-c++ diffutils readline-devel texinfo
# rockylinux:9 RUN dnf --enablerepo=crb install -y texinfo
# rockylinux:8
# RUN dnf --enablerepo=powertools install -y texinfo

# clone deps and compile deps
# RUN mkdir -p ${HOME_DIR}/deps
WORKDIR ${HOME_DIR}/deps

RUN git clone https://gitee.com/mirrors/libevent.git  -b release-2.1.12-stable  \
    && mkdir -p ${HOME_DIR}/deps/libevent/build  \
    && cmake -DEVENT__DISABLE_OPENSSL=ON -B ${HOME_DIR}/deps/libevent/build ${HOME_DIR}/deps/libevent \
    && make -C ${HOME_DIR}/deps/libevent/build -j install \
    && rm -rf ${HOME_DIR}/deps/*

RUN git clone https://gitee.com/mirrors/jsoncpp.git \
    && mkdir -p ${HOME_DIR}/deps/jsoncpp/build \
    && cmake -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF -B ${HOME_DIR}/deps/jsoncpp/build ${HOME_DIR}/deps/jsoncpp/ \
    && make -C ${HOME_DIR}/deps/jsoncpp/build -j install \
    && rm -rf ${HOME_DIR}/deps/*

RUN git clone https://gitee.com/mirrors/googletest.git \
    && mkdir -p ${HOME_DIR}/deps/googletest/build  \
    && cmake -B ${HOME_DIR}/deps/googletest/build ${HOME_DIR}/deps/googletest \
    && make -C ${HOME_DIR}/deps/googletest/build -j install \
    && rm -rf ${HOME_DIR}/deps/*

RUN wget http://ftp.gnu.org/gnu/bison/bison-3.7.tar.gz \
    && tar xzvf bison-3.7.tar.gz \
    && cd bison-3.7 \
    && ./configure --prefix=/usr/local \
    && make -j install \
    && rm -rf ${HOME_DIR}/deps/*

# install openssh
RUN yum -y install openssh openssh-clients openssh-server

# change root password
RUN echo "root:root" | chpasswd
RUN mkdir /var/run/sshd

# install zsh and on-my-zsh
RUN yum install -y zsh \
    && git clone https://gitee.com/mirrors/ohmyzsh.git ~/.oh-my-zsh \
    && cp ~/.oh-my-zsh/templates/zshrc.zsh-template ~/.zshrc \
    && sed -i "s/robbyrussell/bira/" ~/.zshrc \
    && usermod --shell /bin/zsh root \
    && echo "export LD_LIBRARY_PATH=/usr/local/lib64:\$LD_LIBRARY_PATH" >> ~/.zshrc

RUN mkdir -p ${DOCKER_CONFIG_DIR}/bin
RUN mkdir -p ${DOCKER_CONFIG_DIR}/config

WORKDIR ${DOCKER_CONFIG_DIR}/bin

# copy vscode config files
COPY config/* ${DOCKER_CONFIG_DIR}/config/

# copy starter scripts
COPY bin/* ${DOCKER_CONFIG_DIR}/bin/

RUN chmod +x ${DOCKER_CONFIG_DIR}/bin/*

ENTRYPOINT ./starter.sh