include(FetchContent)

FetchContent_Declare(
    libevent
    GIT_REPOSITORY https://github.com/libevent/libevent.git
    GIT_TAG release-2.1.11-stable
)

set(EVENT__DISABLE_OPENSSL ON)

FetchContent_MakeAvailable(libevent)