ADD_EXECUTABLE(obclient)
MESSAGE("Begin to build " obclient)

INCLUDE(readline)
MINIOB_FIND_READLINE()

IF(USE_REPLXX)
    TARGET_LINK_LIBRARIES(obclient replxx::replxx)
    MESSAGE(STATUS "obclient uses replxx")
ENDIF()

FILE(GLOB_RECURSE ALL_SRC *.cpp)
FOREACH (F ${ALL_SRC})

    SET(PRJ_SRC ${PRJ_SRC} ${F})
    MESSAGE("Use " ${F})

ENDFOREACH (F)

TARGET_SOURCES(obclient PRIVATE ${PRJ_SRC})
TARGET_LINK_LIBRARIES(obclient common pthread dl)

# Target 必须在定义 ADD_EXECUTABLE 之后， programs 不受这个限制
# TARGETS和PROGRAMS 的默认权限是OWNER_EXECUTE, GROUP_EXECUTE, 和WORLD_EXECUTE，即755权限， programs 都是处理脚步类
# 类型分为RUNTIME／LIBRARY／ARCHIVE, prog
INSTALL(TARGETS obclient RUNTIME DESTINATION bin)
