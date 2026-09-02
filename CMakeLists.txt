cmake_minimum_required(VERSION 3.25)

project(InTheLineOfDutyFixes VERSION 0.9.1 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "In the Line of Duty Fixes supports Windows only")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Configure an x86 toolchain; the supported game is 32-bit")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(ild_script_patch STATIC
    src/console_clipboard.cpp
    src/console_editor.cpp
    src/script_patch.cpp
    src/script_patch.h
)

target_include_directories(ild_script_patch PUBLIC src)

add_library(dinput8 SHARED
    src/update_bridge.cpp
    src/console_hooks.cpp
    src/x86_detour.cpp
    src/dinput8_proxy.cpp
    src/iat_hook.cpp
    src/iat_hook.h
    src/sha256.cpp
    src/sha256.h
    src/dinput8_proxy.def
)

target_link_libraries(dinput8 PRIVATE ild_script_patch advapi32 shell32)
set_target_properties(dinput8 PROPERTIES PREFIX "" OUTPUT_NAME "dinput8")

add_library(dinput8_console_qa SHARED EXCLUDE_FROM_ALL
    src/update_bridge.cpp
    src/dinput8_proxy.cpp src/console_hooks.cpp src/x86_detour.cpp src/iat_hook.cpp src/sha256.cpp
    src/dinput8_proxy.def
)
target_compile_definitions(dinput8_console_qa PRIVATE ILD_CONSOLE_QA=1)
target_link_libraries(dinput8_console_qa PRIVATE ild_script_patch advapi32 shell32 ole32)
if(MSVC)
    target_compile_options(dinput8_console_qa PRIVATE /W4 /WX /permissive- /utf-8)
    target_link_options(dinput8_console_qa PRIVATE /DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/dinput8_proxy.def)
endif()

if(MSVC)
    target_compile_options(ild_script_patch PRIVATE /W4 /WX /permissive- /utf-8)
    target_compile_options(dinput8 PRIVATE /W4 /WX /permissive- /utf-8)
    target_link_options(dinput8 PRIVATE /DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/dinput8_proxy.def)
endif()

enable_testing()

add_executable(ild_script_patch_tests
    tests/script_patch_tests.cpp
)

target_link_libraries(ild_script_patch_tests PRIVATE ild_script_patch)

if(MSVC)
    target_compile_options(ild_script_patch_tests PRIVATE /W4 /WX /permissive- /utf-8)
endif()

add_test(NAME script_patch COMMAND ild_script_patch_tests)
add_test(NAME update_ui_contract COMMAND ${CMAKE_COMMAND}
    -DSOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/update_ui_contract.cmake)
add_executable(ild_console_editor_tests tests/console_editor_tests.cpp)
target_link_libraries(ild_console_editor_tests PRIVATE ild_script_patch)
if(MSVC)
    target_compile_options(ild_console_editor_tests PRIVATE /W4 /WX /permissive- /utf-8)
endif()
add_test(NAME console_editor COMMAND ild_console_editor_tests)
add_executable(ild_clipboard_tests EXCLUDE_FROM_ALL tests/clipboard_tests.cpp)
target_link_libraries(ild_clipboard_tests PRIVATE ild_script_patch)
if(MSVC)
    target_compile_options(ild_clipboard_tests PRIVATE /W4 /WX /permissive- /utf-8)
endif()
add_executable(ild_restart_stub EXCLUDE_FROM_ALL tools/qa/restart_stub.cpp)

configure_file(
    updater/ProductInfo.cs.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/ProductInfo.cs
    @ONLY
)

set(ILD_UPDATER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/updater/InTheLineOfDutyFixesUpdater.exe)
file(TO_NATIVE_PATH "${ILD_UPDATER_OUTPUT}" ILD_UPDATER_OUTPUT_NATIVE)
set(ILD_CSC "C:/Windows/Microsoft.NET/Framework/v4.0.30319/csc.exe")
file(RELATIVE_PATH ILD_PRODUCT_INFO ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/generated/ProductInfo.cs)
set(ILD_UPDATER_SOURCE_ARGS
    updater/Arguments.cs
    updater/Models.cs
    updater/Program.cs
    updater/UpdateApplier.cs
    updater/UpdateClient.cs
    updater/UpdateService.cs
    ${ILD_PRODUCT_INFO}
)
set(ILD_UPDATER_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/updater/Arguments.cs
    ${CMAKE_CURRENT_SOURCE_DIR}/updater/Models.cs
    ${CMAKE_CURRENT_SOURCE_DIR}/updater/Program.cs
    ${CMAKE_CURRENT_SOURCE_DIR}/updater/UpdateApplier.cs
    ${CMAKE_CURRENT_SOURCE_DIR}/updater/UpdateClient.cs
    ${CMAKE_CURRENT_SOURCE_DIR}/updater/UpdateService.cs
    ${CMAKE_CURRENT_BINARY_DIR}/generated/ProductInfo.cs
)
set(ILD_UPDATER_NATIVE_SOURCE_ARGS)
foreach(ILD_SOURCE IN LISTS ILD_UPDATER_SOURCE_ARGS)
    file(TO_NATIVE_PATH "${ILD_SOURCE}" ILD_NATIVE_SOURCE)
    list(APPEND ILD_UPDATER_NATIVE_SOURCE_ARGS "${ILD_NATIVE_SOURCE}")
endforeach()

add_custom_command(
    OUTPUT ${ILD_UPDATER_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/updater
    COMMAND ${ILD_CSC}
        /nologo
        /target:winexe
        /platform:x86
        /optimize+
        /warn:4
        /warnaserror+
        /out:${ILD_UPDATER_OUTPUT_NATIVE}
        /reference:System.dll
        /reference:System.Core.dll
        /reference:System.Web.Extensions.dll
        /reference:System.IO.Compression.dll
        /reference:System.IO.Compression.FileSystem.dll
        ${ILD_UPDATER_NATIVE_SOURCE_ARGS}
    DEPENDS ${ILD_UPDATER_DEPENDS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

add_custom_target(ild_updater ALL DEPENDS ${ILD_UPDATER_OUTPUT})

add_executable(ild_hidden_desktop_launcher
    tools/hidden_desktop_launcher.cpp
)

if(MSVC)
    target_compile_options(ild_hidden_desktop_launcher PRIVATE /W4 /WX /permissive- /utf-8)
endif()
