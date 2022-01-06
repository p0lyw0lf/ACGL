cmake_minimum_required(VERSION 3.7)

#set(ACGL_BUILD_STATIC YES)

if (DEFINED ACGL_BUILD_STATIC)
    set(CMAKE_BUILD_SHARED_LIBS OFF)
    if (UNIX)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
        set(CMAKE_EXE_LINKER_FLAGS "-static")
    endif (UNIX)
endif ()

project(acgl VERSION 0.1.1 DESCRIPTION "Another Custom GUI Library")
find_package(SDL2 CONFIG REQUIRED)

set(SOURCE_FILES
    "src/gui.c"
    "src/gui_safety.c"
    "src/inputhandler.c"
)

if (DEFINED ACGL_BUILD_STATIC)
    add_library(acgl STATIC
        ${SOURCE_FILES}
    )
    #[[ if (MSVC)
        target_compile_options(acgl PUBLIC/PRIVATE /MT)
        target_compile_options(acgl PUBLIC/PRIVATE  /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
    endif (MSVC) ]]
else ()
    add_library(acgl SHARED
        ${SOURCE_FILES}
    )
endif()

set_target_properties(acgl PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(acgl PROPERTIES SOVERSION 0)
set_target_properties(acgl PROPERTIES PUBLIC_HEADER "include/acgl.h")

target_include_directories(acgl PRIVATE include/acgl)
target_link_libraries(acgl PRIVATE SDL2::SDL2main SDL2::SDL2)

configure_file(acgl.pc.in acgl.pc @ONLY)
include(GNUInstallDirs)
install(TARGETS acgl
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER
)
install(FILES ${CMAKE_BINARY_DIR}/acgl.pc
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
