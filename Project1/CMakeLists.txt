cmake_minimum_required(VERSION 3.10)

# プロジェクト名と使用する言語
project(Project1 LANGUAGES CXX)

# C++17標準を有効にする
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windowsでの文字コード問題に対処する
if(MSVC)
  add_definitions(-DUNICODE -D_UNICODE)
  add_compile_options(/utf-8)  # UTF-8コードページを使用（/source-charset と /execution-charset を含む）
  add_compile_options(/validate-charset)  # 文字コード検証有効化
  add_compile_options(/w14640) # 警告を有効化して文字コードの問題を検出
endif()

# ビルドタイプのデフォルト値を設定
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    message(STATUS "Build type not specified: Defaulting to Debug")
endif()

# すべての出力を一時ディレクトリに設定 (権限の問題回避のため)
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/output")
file(MAKE_DIRECTORY ${OUTPUT_DIR})
file(MAKE_DIRECTORY "${OUTPUT_DIR}/Debug")
file(MAKE_DIRECTORY "${OUTPUT_DIR}/Release")

# ビルド関連の出力ディレクトリを設定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# ビルドタイプ別の出力ディレクトリを設定
foreach(OUTPUT_CONFIG ${CMAKE_CONFIGURATION_TYPES} Debug Release)
    string(TOUPPER ${OUTPUT_CONFIG} OUTPUT_CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} "${OUTPUT_DIR}/${OUTPUT_CONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} "${OUTPUT_DIR}/${OUTPUT_CONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} "${OUTPUT_DIR}/${OUTPUT_CONFIG}")
    set(CMAKE_PDB_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} "${OUTPUT_DIR}/${OUTPUT_CONFIG}")
endforeach()

message(STATUS "Runtime output directory set to: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")

# CMakeのメッセージ出力を詳細に設定
set(CMAKE_VERBOSE_MAKEFILE ON)
message(STATUS "Verbose makefile enabled")

# ソースファイルの指定
set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/WinMain.cpp
    ${CMAKE_SOURCE_DIR}/src/tinyxml2.cpp
    ${CMAKE_SOURCE_DIR}/src/WindowRoutines.cpp
    ${CMAKE_SOURCE_DIR}/src/FileUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/Win32WindowContext.cpp
    ${CMAKE_SOURCE_DIR}/src/WinApplication.cpp
)

# 実行可能ファイルの生成
add_executable(Project1 ${SOURCES})
message(STATUS "Executable target 'Project1' created with sources: ${SOURCES}")

# インクルードディレクトリの指定
target_include_directories(Project1 PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/resource
)

# リソースファイルの設定
if(WIN32)
    set(RESOURCE_FILE ${CMAKE_SOURCE_DIR}/resource/DialogResource.rc)
    if(EXISTS ${RESOURCE_FILE})
        target_sources(Project1 PRIVATE ${RESOURCE_FILE})
        message(STATUS "Resource file added: ${RESOURCE_FILE}")
    else()
        message(WARNING "Resource file not found: ${RESOURCE_FILE}")
    endif()
endif()

# ビルド後に実行ファイルをコピーするカスタムコマンド（ターゲット定義後に追加）
add_custom_command(
    TARGET Project1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying executable to x64/bin/Debug directory"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/x64/bin/Debug
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        $<TARGET_FILE:Project1> 
        ${CMAKE_SOURCE_DIR}/x64/bin/Debug/
)