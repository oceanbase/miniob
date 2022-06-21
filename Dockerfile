# how to use
# docker build -t miniob .
# make sure docker has been installed

FROM centos:7

ARG HOME_DIR=/root
ARG GIT_SOURCE=github

ENV LANG=en_US.UTF-8

# install rpm
RUN yum install -y make git wget centos-release-scl scl-utils which flex

# prepare env
RUN yum install -y devtoolset-11-gcc devtoolset-11-gcc-c++
WORKDIR ${HOME_DIR}
RUN echo "export PATH=/opt/rh/devtoolset-11/root/bin:/usr/local/oceanbase/devtools/bin:$PATH" >> .bashrc
RUN echo "export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH" >> .bashrc

ENV PATH /opt/rh/devtoolset-11/root/bin:/usr/local/oceanbase/devtools/bin:$PATH
ENV LD_LIBRARY_PATH /usr/local/lib64:$LD_LIBRARY_PATH

# clone deps and compile deps
RUN mkdir -p ${HOME_DIR}/deps
WORKDIR ${HOME_DIR}/deps
RUN wget http://yum-test.obvos.alibaba-inc.com/oceanbase/development-kit/el/7/x86_64/obdevtools-cmake-3.20.2-3.el7.x86_64.rpm \
    && rpm -ivh obdevtools-cmake-3.20.2-3.el7.x86_64.rpm && rm -f obdevtools-cmake-3.20.2-3.el7.x86_64.rpm

RUN git clone https://github.com/libevent/libevent  -b release-2.1.12-stable  \
    && mkdir -p ${HOME_DIR}/deps/libevent/build  \
    && cmake -DEVENT__DISABLE_OPENSSL=ON -B ${HOME_DIR}/deps/libevent/build ${HOME_DIR}/deps/libevent \
    && make -C ${HOME_DIR}/deps/libevent/build -j install \
    && rm -rf ${HOME_DIR}/deps/*

RUN git clone https://github.com/open-source-parsers/jsoncpp.git \
    && mkdir -p ${HOME_DIR}/deps/jsoncpp/build \
    && cmake -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF -B ${HOME_DIR}/deps/jsoncpp/build ${HOME_DIR}/deps/jsoncpp/ \
    && make -C ${HOME_DIR}/deps/jsoncpp/build -j install \
    && rm -rf ${HOME_DIR}/deps/*

RUN git clone https://github.com/google/googletest \
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

# clone miniob code
RUN mkdir -p ${HOME_DIR}/source
WORKDIR ${HOME_DIR}/source
RUN git clone https://${GIT_SOURCE}.com/oceanbase/miniob
RUN echo "mkdir -p build && cd build && cmake .. -DDEBUG=ON -DCMAKE_C_COMPILER=`which gcc` -DCMAKE_CXX_COMPILER=`which g++` && make -j4" > ${HOME_DIR}/source/miniob/build.sh && chmod +x ${HOME_DIR}/source/miniob/build.sh
RUN mkdir -p ${HOME_DIR}/source/miniob/build
WORKDIR ${HOME_DIR}/source/miniob/build
RUN cmake -B ${HOME_DIR}/source/miniob/build -DDEBUG=ON -DCMAKE_C_COMPILER=`which gcc` -DCMAKE_CXX_COMPILER=`which g++` ${HOME_DIR}/source/miniob/ \
    && make -j4 \
    && rm -rf ${HOME_DIR}/source/miniob/build

WORKDIR ${HOME_DIR}

ENTRYPOINT exec /sbin/init

