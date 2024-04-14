find_package(raylib 3.0 REQUIRED) 

set(CMAKE_C_STANDARD 11) 

add_subdirectory(external)

add_subdirectory(channel)
add_subdirectory(osc)
add_subdirectory(filter)
add_subdirectory(amp)

add_executable(${PROJECT_NAME} synth.c)
target_link_libraries(${PROJECT_NAME} raylib fft channel osc filter amp)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()


