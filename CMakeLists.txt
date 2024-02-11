CMAKE_MINIMUM_REQUIRED     (VERSION 3.28)
project                    (relic-mercs-game 
                            DESCRIPTION "A turn-based co-petetive multiplayer game"
                            VERSION 0.1.0
)
set                        (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set                        (BINARY_NAME "relicServer")


file                       (GLOB SOURCES "source/*.c")
add_executable             (${BINARY_NAME} ${SOURCES})

target_compile_options     (${BINARY_NAME} PRIVATE -std=c2x -fvisibility=hidden 
                            $<$<CONFIG:Debug>:-O0;-g3;-ggdb>
                            $<$<CONFIG:Release>:-O3>)
target_compile_definitions (${BINARY_NAME} PRIVATE $<$<CONFIG:Debug>: DEBUG>)

add_subdirectory           (dependencies/libwebsockets)
