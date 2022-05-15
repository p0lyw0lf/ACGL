cmake_minimum_required(VERSION 3.7)
project(acgl VERSION 0.1.1 DESCRIPTION "Another Custom GUI Library")

set(SOURCE_FILES
    "src/gui.c"
    "src/gui_safety.c"
    "src/inputhandler.c"
    "src/threads.c"
)
set(HEADER_FILES
  "include/acgl/common.h"
  "include/acgl/contracts.h"
  "include/acgl/gui.h"
  "include/acgl/gui_safety.h"
  "include/acgl/inputhandler.h"
  "include/acgl/threads.h"
)

add_library(acgl STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(acgl PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(acgl PROPERTIES SOVERSION 0)
set_target_properties(acgl PROPERTIES PUBLIC_HEADER "include/acgl.h")
target_include_directories(acgl PRIVATE "include/acgl")

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

# Look for SDL2 if not already found
if (NOT SDL2_FOUND)
  find_package(SDL2 CONFIG REQUIRED)
endif()
target_link_libraries(acgl PRIVATE SDL2::SDL2)