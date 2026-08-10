cmake_minimum_required(VERSION 3.25)
project(holonight-greeter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)

option(BUILD_TESTING "Build tests" ON)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Quick Qml Network DBus)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
find_package(HolonightQt REQUIRED COMPONENTS Core Controls)
find_package(tomlplusplus CONFIG REQUIRED)

set(GREETER_FACE_FILES
  assets/faces/aster.png
  assets/faces/echo.png
  assets/faces/kael.png
  assets/faces/lyra.png
  assets/faces/nyx.png
  assets/faces/sora.png
  assets/faces/tux.png
  assets/faces/vault-boy.png
)

add_library(greeter_core STATIC
  src/config.cpp src/config.h
  src/desktopentry.cpp src/desktopentry.h
  src/greetdclient.cpp src/greetdclient.h
  src/state.cpp src/state.h
  src/controller.cpp src/controller.h
  src/services.cpp src/services.h
)
target_include_directories(greeter_core PUBLIC src)
target_link_libraries(greeter_core PUBLIC Qt6::Core Qt6::Network Qt6::DBus tomlplusplus::tomlplusplus)
target_compile_options(greeter_core PRIVATE -Wall -Wextra -Wpedantic)

qt_add_executable(holonight-greeter src/main.cpp)
qt_add_qml_module(holonight-greeter
  URI Holonight.Greeter VERSION 1.0
  QML_FILES qml/Main.qml qml/LoginPanel.qml
  RESOURCES assets/backgrounds/wallpaper.png qml/images/no-avatar.png
)
target_link_libraries(holonight-greeter PRIVATE greeter_core Qt6::Gui Qt6::Quick Qt6::Qml
  HolonightQt::Core HolonightQt::Controls)

install(TARGETS holonight-greeter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/greeter.toml DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/holonight)
install(FILES assets/backgrounds/wallpaper.png
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/holonight-greeter/backgrounds)
install(FILES ${GREETER_FACE_FILES}
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps/faces)
install(FILES deploy/holonight-greeter.conf DESTINATION ${CMAKE_INSTALL_LIBDIR}/tmpfiles.d)
install(FILES deploy/greetd-config.toml docs/CAGE.md
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/holonight-greeter)

if(BUILD_TESTING)
  find_package(Qt6 6.11 REQUIRED COMPONENTS Test)
  find_package(GTest REQUIRED)
  enable_testing()
  add_executable(greeter_tests tests/main.cpp tests/core_test.cpp)
  target_link_libraries(greeter_tests PRIVATE greeter_core GTest::gtest Qt6::Test)
  target_compile_definitions(greeter_tests PRIVATE
    GREETER_EXECUTABLE_PATH="$<TARGET_FILE:holonight-greeter>")
  add_test(NAME greeter_tests COMMAND greeter_tests)
endif()

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
  file(GLOB_RECURSE FORMAT_FILES CONFIGURE_DEPENDS src/*.cpp src/*.h tests/*.cpp)
  add_custom_target(format-check COMMAND ${CLANG_FORMAT} --dry-run --Werror ${FORMAT_FILES})
endif()
