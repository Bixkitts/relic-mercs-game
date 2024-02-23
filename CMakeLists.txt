CMAKE_MINIMUM_REQUIRED     (VERSION 3.28)
project                    (relic-mercs-game 
                            DESCRIPTION "A turn-based co-petetive multiplayer game"
                            VERSION 0.1.0
)
# So that LSPs know what's up
set                        (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set                        (BINARY_NAME "relicServer")

file                       (GLOB SOURCES "source/*.c")

add_executable             (${BINARY_NAME})
target_sources             (${BINARY_NAME} PRIVATE ${SOURCES})

target_compile_options     (${BINARY_NAME} PRIVATE -std=c2x
                            $<$<CONFIG:Debug>:-O0;-g3;-ggdb;-Wall>
                            $<$<CONFIG:Release>:-O3;-flto>)
target_compile_definitions (${BINARY_NAME} PRIVATE $<$<CONFIG:Debug>: DEBUG>)

target_link_options        (${BINARY_NAME} PRIVATE -lpthread)

add_subdirectory           (${CMAKE_CURRENT_SOURCE_DIR}/dependencies/bb-net-lib)
target_link_libraries      (${BINARY_NAME} PRIVATE bbnetlib)
