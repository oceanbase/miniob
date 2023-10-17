ADD_EXECUTABLE(clog_reader)
MESSAGE("Begin to build clog_reader")

SET(CLOG_READER_SRC clog_reader_cmd.cpp)

TARGET_SOURCES(clog_reader PRIVATE ${CLOG_READER_SRC})
TARGET_LINK_LIBRARIES(clog_reader observer_static)
TARGET_INCLUDE_DIRECTORIES(clog_reader PRIVATE ${PROJECT_SOURCE_DIR}/src/observer/)

# Target 必须在定义 ADD_EXECUTABLE 之后， programs 不受这个限制
# TARGETS和PROGRAMS 的默认权限是OWNER_EXECUTE, GROUP_EXECUTE, 和WORLD_EXECUTE，即755权限， programs 都是处理脚步类
# 类型分为RUNTIME／LIBRARY／ARCHIVE, prog
INSTALL(TARGETS clog_reader RUNTIME DESTINATION bin)
