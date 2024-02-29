CMAKE_MINIMUM_REQUIRED      (VERSION 3.28)
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

target_compile_options     (${BINARY_NAME} PRIVATE -std=gnu11
                            $<$<CONFIG:Debug>:-O0;-g3;-ggdb;-Wall>
                            $<$<CONFIG:Release>:-O3;-flto>)
target_compile_definitions (${BINARY_NAME} PRIVATE $<$<CONFIG:Debug>: DEBUG>)

target_link_options        (${BINARY_NAME} PRIVATE -lpthread)

add_subdirectory           (${CMAKE_CURRENT_SOURCE_DIR}/dependencies/bb-net-lib)
target_link_libraries      (${BINARY_NAME} PRIVATE bbnetlib)

# Copy website next to binary
set  (WEBSITE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test-clients/website/")
#file (COPY "${WEBSITE_DIR}" DESTINATION "${CMAKE_BINARY_DIR}")
add_custom_target(copy_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${WEBSITE_DIR} ${CMAKE_BINARY_DIR} # Command to copy the files
    DEPENDS ${WEBSITE_DIR} # Specify that the copying action depends on the source directory
    COMMENT "Copying test-clients/website to binary directory..."
)
