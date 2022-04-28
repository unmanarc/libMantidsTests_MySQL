cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

set(APP_NAME "hellomantids")
set(SVERSION 1.0.0)


######################################
find_library (
    MARIADB_LIB
    NAMES mariadb libmariadb mysql libmysql libmysqlclient mysqlclient # what to look for
    PATH_SUFFIXES mysql
)
if (NOT MARIADB_LIB)
    message("-- WARNING: Did not found lib MARIADB_LIB")
else()
    get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
    get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

    find_path (
            MYSQL_INCLUDE_DIR NAMES mysql.h PATHS /usr/include /usr/include/mysql /usr/include/mariadb
        )

    if (MYSQL_INCLUDE_DIR)
        message("-- MARIADB_LIB found at [${MARIADB_LIB}] for ${LIB_NAME}")
        message("-- MARIADB_INCLUDE_DIR found at [${MYSQL_INCLUDE_DIR}]")


        project(${APP_NAME})
        project(${PROJECT_NAME} VERSION ${SVERSION} DESCRIPTION "Hello World for libMantids WEB Application - MariaDB version")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

        file(GLOB_RECURSE EDV_INCLUDE_FILES "src/*.h*")
        file(GLOB_RECURSE EDV_SOURCE_FILES "src/*.c*")
        file(GLOB_RECURSE EDV_INCLUDE_FILES2 "src/*/*.h*")
        file(GLOB_RECURSE EDV_SOURCE_FILES2 "src/*/*.c*")

        configure_file(src/config.h.in ${${APP_NAME}_SOURCE_DIR}/src/config.h)

        add_executable( ${APP_NAME} ${EDV_INCLUDE_FILES} ${EDV_SOURCE_FILES} ${EDV_INCLUDE_FILES2} ${EDV_SOURCE_FILES2} )

        install( TARGETS ${APP_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

        if (EXTRAPREFIX)
            target_include_directories(${APP_NAME} PUBLIC ${EXTRAPREFIX}/include)
            target_link_libraries(${APP_NAME} "-L${EXTRAPREFIX}/lib")
            target_link_libraries(${APP_NAME} "-L${EXTRAPREFIX}/lib64")
        endif()


        if (CMAKE_INSTALL_PREFIX)
            target_include_directories(${APP_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/include)
            target_include_directories(${APP_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/usr/include)
            target_link_libraries(${APP_NAME} "-L${CMAKE_INSTALL_PREFIX}/lib")
            target_link_libraries(${APP_NAME} "-L${CMAKE_INSTALL_PREFIX}/lib64")
        endif()

        target_link_libraries(${APP_NAME} "-lmdz_xrpc_fast")
        target_link_libraries(${APP_NAME} "-lmdz_xrpc_webserver")
        target_link_libraries(${APP_NAME} "-lmdz_xrpc_common")
        target_link_libraries(${APP_NAME} "-lmdz_auth")
        target_link_libraries(${APP_NAME} "-lmdz_auth_remote")
        target_link_libraries(${APP_NAME} "-lmdz_db")
        target_link_libraries(${APP_NAME} "-lmdz_db_mariadb")
        target_link_libraries(${APP_NAME} "-lmdz_proto_http")
        target_link_libraries(${APP_NAME} "-lmdz_proto_mime")
        target_link_libraries(${APP_NAME} "-lmdz_net_sockets")
        target_link_libraries(${APP_NAME} "-lmdz_thr_mutex")
        target_link_libraries(${APP_NAME} "-lmdz_thr_safecontainers")
        target_link_libraries(${APP_NAME} "-lmdz_thr_threads")
        target_link_libraries(${APP_NAME} "-lmdz_prg_service")
        target_link_libraries(${APP_NAME} "-lmdz_prg_logs")
        target_link_libraries(${APP_NAME} "-lmdz_mem_vars")
        target_link_libraries(${APP_NAME} "-lmdz_hlp_functions")

        target_link_libraries(${APP_NAME} "-lssl")
        target_link_libraries(${APP_NAME} "-lcrypto")
        target_link_libraries(${APP_NAME} "-lpthread")


        if (WIN32)
            target_link_libraries(${APP_NAME} "-lboost_regex-mt")
        else()
            target_link_libraries(${APP_NAME} "-lboost_regex")
        endif()

        find_package(PkgConfig REQUIRED)
        pkg_check_modules(JSONCPP jsoncpp)
        link_libraries(${JSONCPP_LIBRARIES})
        target_include_directories(${APP_NAME} PUBLIC ${JSONCPP_INCLUDE_DIRS})
        target_link_libraries(${APP_NAME} ${JSONCPP_LIBRARIES})

        target_include_directories(${LIB_NAME} PRIVATE ${MYSQL_INCLUDE_DIR})
        target_link_libraries(${LIB_NAME} ${MARIADB_LIB})

    else()
        message("-- WARNING: Did not found lib MARIADB_INCLUDE_DIR")
    endif()

endif()
