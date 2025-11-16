cmake_minimum_required(VERSION 3.10)
project(pg_ai_ext LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(pg_ai_ext SHARED src/pg_ai_ext.c src/ai_bridge.cpp)
set_target_properties(pg_ai_ext PROPERTIES LINKER_LANGUAGE CXX
OUTPUT_NAME "pg_ai_ext"
PREFIX "")

# locate pg_config and use it to get server include dir
find_program(PG_CONFIG pg_config)
if(PG_CONFIG)
  execute_process(COMMAND ${PG_CONFIG} --includedir-server
                  OUTPUT_VARIABLE PG_INCLUDEDIR
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "Postgres server include dir: ${PG_INCLUDEDIR}")
else()
  message(FATAL_ERROR "pg_config not found; set PATH so pg_config is available")
endif()

# add Postgres server headers
target_include_directories(pg_ai_ext PRIVATE ${PG_INCLUDEDIR})

# locate ai-sdk includes and libraries
find_path(AI_SDK_INCLUDE_DIR
  NAMES ai/types/generate_options.h
  PATHS /opt/ai-sdk/include /usr/local/include
  NO_DEFAULT_PATH)

find_library(AI_SDK_OPENAI_LIB
  NAMES ai-sdk-cpp-openai libai-sdk-cpp-openai
  PATHS /opt/ai-sdk/lib /usr/local/lib /usr/lib
  NO_DEFAULT_PATH)

find_library(AI_SDK_CORE_LIB
  NAMES ai-sdk-cpp-core libai-sdk-cpp-core
  PATHS /opt/ai-sdk/lib /usr/local/lib /usr/lib
  NO_DEFAULT_PATH)

if(NOT AI_SDK_INCLUDE_DIR)
  message(FATAL_ERROR "AI SDK headers not found; set AI SDK installation under /opt/ai-sdk or add path")
endif()
if(NOT AI_SDK_OPENAI_LIB OR NOT AI_SDK_CORE_LIB)
  message(FATAL_ERROR "AI SDK libraries not found; ensure libai-sdk-cpp-openai.a and libai-sdk-cpp-core.a are installed")
endif()

#AI-sdk include directory
target_include_directories(pg_ai_ext PRIVATE ${AI_SDK_INCLUDE_DIR})
target_compile_definitions(pg_ai_ext PRIVATE AI_SDK_HAS_OPENAI=1)

target_link_options(pg_ai_ext PRIVATE -stdlib=libc++ -undefined dynamic_lookup)

find_library(BROTLIDEC_LIB brotlidec PATHS /opt/homebrew/lib NO_DEFAULT_PATH)
find_library(BROTLICOMMON_LIB brotlicommon PATHS /opt/homebrew/lib NO_DEFAULT_PATH)
find_library(SSL_LIB ssl 
    PATHS /opt/homebrew/lib /usr/local/lib 
    NO_DEFAULT_PATH)
find_library(CRYPTO_LIB crypto 
    PATHS /opt/homebrew/lib /usr/local/lib 
    NO_DEFAULT_PATH)

#link the library
target_link_libraries(pg_ai_ext PRIVATE
    ${AI_SDK_OPENAI_LIB}
    ${AI_SDK_CORE_LIB}
    ${BROTLIDEC_LIB}
    ${BROTLICOMMON_LIB}
    curl
    ${SSL_LIB}
    ${CRYPTO_LIB}
    z)

install(TARGETS pg_ai_ext DESTINATION /Users/gershonkumar/project/lib/postgresql)
install(FILES pg_ai_ext.control pg_ai_ext--1.0.0.sql
        DESTINATION /Users/gershonkumar/project/share/postgresql/extension)