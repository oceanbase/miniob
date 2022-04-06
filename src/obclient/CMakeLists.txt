ADD_EXECUTABLE(obclient)
MESSAGE("Begin to build " obclient)

INCLUDE(CheckIncludeFiles)

#INCLUDE_DIRECTORIES([AFTER|BEFORE] [SYSTEM] dir1 dir2 ...)
TARGET_INCLUDE_DIRECTORIES(obclient PRIVATE . ${PROJECT_SOURCE_DIR}/../deps /usr/local/include /usr/include)
# 父cmake 设置的include_directories 和link_directories并不传导到子cmake里面
#INCLUDE_DIRECTORIES(BEFORE ${CMAKE_INSTALL_PREFIX}/include)

TARGET_LINK_DIRECTORIES(obclient PRIVATE /usr/local/lib ${PROJECT_BINARY_DIR}/../../lib)

# stdio.h 必须放在readline/readline.h 前面，因为readline头文件不能直接单独编译
CHECK_INCLUDE_FILES("stdio.h;readline/readline.h" HAVE_READLINE_HEADER)

FIND_LIBRARY(READLINE_LIBRARY readline)
IF (HAVE_READLINE_HEADER AND READLINE_LIBRARY)
    TARGET_LINK_LIBRARIES(obclient ${READLINE_LIBRARY})
    ADD_DEFINITIONS(-DUSE_READLINE)
ELSE ()
    MESSAGE ("readline is not found")
ENDIF()

FILE(GLOB_RECURSE ALL_SRC *.cpp)
FOREACH (F ${ALL_SRC})

    SET(PRJ_SRC ${PRJ_SRC} ${F})
    MESSAGE("Use " ${F})

ENDFOREACH (F)

# 指定目标文件位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin)
MESSAGE("Binary directory:" ${EXECUTABLE_OUTPUT_PATH})
TARGET_SOURCES(obclient PRIVATE ${PRJ_SRC})
TARGET_LINK_LIBRARIES(obclient common pthread dl)


# Target 必须在定义 ADD_EXECUTABLE 之后， programs 不受这个限制
# TARGETS和PROGRAMS 的默认权限是OWNER_EXECUTE, GROUP_EXECUTE, 和WORLD_EXECUTE，即755权限， programs 都是处理脚步类
# 类型分为RUNTIME／LIBRARY／ARCHIVE, prog
INSTALL(TARGETS obclient RUNTIME DESTINATION bin)
