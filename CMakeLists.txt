cmake_minimum_required(VERSION 3.25)
project(holonight-greeter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(GNUInstallDirs)
install(FILES LICENSE ASSET_PROVENANCE.md LICENSES/GPL-3.0-or-later.txt
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME}")

option(BUILD_TESTING "Build tests" ON)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Quick Qml Network DBus)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
find_package(HolonightQt 0.1.1 REQUIRED COMPONENTS Core Controls)
unset(HOLONIGHT_QML_IMPORT_PATH CACHE)
find_path(HOLONIGHT_QML_IMPORT_PATH NAMES Holonight/qmldir
  HINTS "${HolonightQt_DIR}/../../.."
  PATH_SUFFIXES lib/qt6/qml lib64/qt6/qml NO_DEFAULT_PATH REQUIRED)
if(IS_ABSOLUTE "${QT6_INSTALL_QML}")
  set(GREETER_QT_QML_PATH "${QT6_INSTALL_QML}")
else()
  set(GREETER_QT_QML_PATH "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}")
endif()
get_filename_component(GREETER_DEPENDENCY_LIB_DIR "${HOLONIGHT_QML_IMPORT_PATH}/../.." ABSOLUTE)
file(RELATIVE_PATH GREETER_INSTALL_QML_PATH "${CMAKE_INSTALL_FULL_BINDIR}"
  "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qt6/qml")
set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
find_package(LayerShellQt REQUIRED)
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
  src/compositoradapter.cpp src/compositoradapter.h
  src/sessionlauncher.cpp src/sessionlauncher.h
  src/desktopentry.cpp src/desktopentry.h
  src/greetdclient.cpp src/greetdclient.h
  src/state.cpp src/state.h
  src/controller.cpp src/controller.h
  src/services.cpp src/services.h
)
target_include_directories(greeter_core PUBLIC src)
target_link_libraries(greeter_core PUBLIC Qt6::Core Qt6::Network Qt6::DBus tomlplusplus::tomlplusplus)
target_compile_options(greeter_core PRIVATE -Wall -Wextra -Wpedantic)

set(GREETER_QML_FILES qml/Main.qml qml/LoginPanel.qml qml/FooterSelector.qml qml/Background.qml)
set(GREETER_QML_ASSETS assets/backgrounds/wallpaper.png qml/images/no-avatar.png)
qt_add_executable(holonight-greeter src/main.cpp)
qt_add_resources(holonight-greeter controls_config PREFIX "/" FILES qtquickcontrols2.conf)
target_compile_definitions(holonight-greeter PRIVATE
  GREETER_BUILD_EXECUTABLE="$<TARGET_FILE:holonight-greeter>"
  HOLONIGHT_QML_IMPORT_PATH="${HOLONIGHT_QML_IMPORT_PATH}"
  GREETER_INSTALL_QML_PATH="${GREETER_INSTALL_QML_PATH}")
qt_add_qml_module(holonight-greeter
  URI Holonight.Greeter VERSION 1.0
  QML_FILES ${GREETER_QML_FILES}
  RESOURCES ${GREETER_QML_ASSETS}
)
target_link_libraries(holonight-greeter PRIVATE greeter_core Qt6::Gui Qt6::Quick Qt6::Qml
  HolonightQt::Core HolonightQt::Controls LayerShellQt::Interface)
target_sources(holonight-greeter PRIVATE src/outputmanager.cpp src/outputmanager.h)

add_executable(holonight-greeter-session src/sessionmain.cpp)
target_link_libraries(holonight-greeter-session PRIVATE greeter_core Qt6::Core)

install(TARGETS holonight-greeter holonight-greeter-session
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/greeter.toml DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/holonight)
install(FILES assets/backgrounds/wallpaper.png
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/holonight-greeter/backgrounds)
install(FILES ${GREETER_FACE_FILES}
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps/faces)
install(FILES deploy/holonight-greeter.conf DESTINATION ${CMAKE_INSTALL_LIBDIR}/tmpfiles.d)
install(FILES LICENSE deploy/greetd-config.toml docs/CAGE.md
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/holonight-greeter)
install(PROGRAMS deploy/holonight-greeter-cage-diagnostic
  DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/holonight-greeter)

if(BUILD_TESTING)
  find_package(Qt6 6.11 REQUIRED COMPONENTS Test QmlPrivate)
  find_package(GTest REQUIRED)
  enable_testing()
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_executable(greeter_tests tests/main.cpp tests/core_test.cpp)
  target_link_libraries(greeter_tests PRIVATE greeter_core GTest::gtest Qt6::Test)
  target_compile_definitions(greeter_tests PRIVATE
    GREETER_EXECUTABLE_PATH="$<TARGET_FILE:holonight-greeter>")
  add_test(NAME greeter_tests COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-isolated-test.py" "$<TARGET_FILE:greeter_tests>")
  set_tests_properties(greeter_tests PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${GREETER_DEPENDENCY_LIB_DIR};QT_QUICK_CONTROLS_STYLE=Holonight" TIMEOUT 120)
  add_test(NAME greeter_tests_Fusion COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-isolated-test.py" "$<TARGET_FILE:greeter_tests>")
  set_tests_properties(greeter_tests_Fusion PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${GREETER_DEPENDENCY_LIB_DIR};QT_QUICK_CONTROLS_STYLE=Fusion" TIMEOUT 120)
  qt_add_executable(greeter_runtime_tests tests/runtime/main.cpp
    tests/runtime/controls_test.cpp tests/runtime/fakes.h)
  qt_add_qml_module(greeter_runtime_tests
    URI Holonight.Greeter VERSION 1.0 OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime-qml"
    QML_FILES ${GREETER_QML_FILES}
    RESOURCES ${GREETER_QML_ASSETS})
  qt_add_resources(greeter_runtime_tests controls_config PREFIX "/" FILES qtquickcontrols2.conf)
  target_link_libraries(greeter_runtime_tests PRIVATE greeter_core GTest::gtest
    Qt6::Test Qt6::Quick Qt6::QmlPrivate HolonightQt::Core HolonightQt::Controls)
  target_compile_definitions(greeter_runtime_tests PRIVATE
    HOLONIGHT_QML_IMPORT_PATH="${HOLONIGHT_QML_IMPORT_PATH}")
  foreach(style Holonight Fusion)
    foreach(dpr 1 1.25)
      add_test(NAME runtime_${style}_${dpr} COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-isolated-test.py" "$<TARGET_FILE:greeter_runtime_tests>")
      set_tests_properties(runtime_${style}_${dpr} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=${style};QT_SCALE_FACTOR=${dpr};LD_LIBRARY_PATH=${GREETER_DEPENDENCY_LIB_DIR}"
        TIMEOUT 120)
    endforeach()
  endforeach()
  add_test(NAME runtime_import_policy COMMAND bash
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check-runtime-qml-imports.sh" "${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME runtime_import_policy_fixtures COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/runtime/policy_fixtures.py")
endif()

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
  file(GLOB_RECURSE FORMAT_FILES CONFIGURE_DEPENDS src/*.cpp src/*.h tests/*.cpp)
  add_custom_target(format-check COMMAND ${CLANG_FORMAT} --dry-run --Werror ${FORMAT_FILES})
endif()

add_custom_target(qml-lint
  COMMAND Qt6::qmllint --bare --max-warnings 0 -I "${HOLONIGHT_QML_IMPORT_PATH}" -I "${GREETER_QT_QML_PATH}"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/LoginPanel.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/FooterSelector.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/Background.qml"
  DEPENDS holonight-greeter)
