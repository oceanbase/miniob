FROM centos:7

ARG HOME_DIR=/root
ARG GIT_SOURCE=github

ENV LANG=en_US.UTF-8

# install rpm
RUN yum install -y make git wget centos-release-scl scl-utils which

# clone deps
RUN mkdir -p ${HOME_DIR}/deps
WORKDIR ${HOME_DIR}/deps
RUN wget http://yum-test.obvos.alibaba-inc.com/oceanbase/development-kit/el/7/x86_64/obdevtools-cmake-3.20.2-3.el7.x86_64.rpm \
    && rpm -ivh obdevtools-cmake-3.20.2-3.el7.x86_64.rpm
RUN git clone https://github.com/libevent/libevent  -b release-2.1.12-stable
RUN git clone https://github.com/open-source-parsers/jsoncpp.git
RUN git clone https://github.com/google/googletest

# prepare env
RUN yum install -y devtoolset-11-gcc devtoolset-11-gcc-c++
WORKDIR ${HOME_DIR}
RUN echo "export PATH=/opt/rh/devtoolset-11/root/bin:/usr/local/oceanbase/devtools/bin:$PATH" >> .bashrc
RUN echo "export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH" >> .bashrc

ENV PATH /opt/rh/devtoolset-11/root/bin:/usr/local/oceanbase/devtools/bin:$PATH
ENV LD_LIBRARY_PATH /usr/local/lib64:$LD_LIBRARY_PATH

# compile deps
RUN mkdir -p ${HOME_DIR}/deps/libevent/build  \
    && mkdir -p ${HOME_DIR}/deps/googletest/build  \
    && mkdir -p ${HOME_DIR}/deps/jsoncpp

WORKDIR ${HOME_DIR}/deps/libevent/build
RUN cmake .. -DEVENT__DISABLE_OPENSSL=ON && make -j && make install

WORKDIR ${HOME_DIR}/deps/googletest/build
RUN cmake .. && make -j && make install

WORKDIR ${HOME_DIR}/deps/jsoncpp/build
RUN cmake -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF .. && make -j && make install

# clone miniob code
RUN mkdir -p ${HOME_DIR}/source
WORKDIR ${HOME_DIR}/source
RUN git clone https://${GIT_SOURCE}.com/oceanbase/miniob
RUN mkdir -p ${HOME_DIR}/source/miniob/build
WORKDIR ${HOME_DIR}/source/miniob/build
RUN cmake .. -DDEBUG=ON -DCMAKE_C_COMPILER=`which gcc` -DCMAKE_CXX_COMPILER=`which g++`  \
    && make -j4

WORKDIR ${HOME_DIR}

ENTRYPOINT exec /sbin/init

