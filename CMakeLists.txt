cmake_minimum_required(VERSION 3.5)

project(gravity_simulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(deps/io_context)
add_subdirectory(deps/gl_context)

add_executable(gravity_simulation_exe
    main.cpp
    Bodies.cpp
    Renderer.cpp
    GLSimShaders.cpp
    BodiesCalculator.cpp
)
target_link_libraries(gravity_simulation_exe
    gl_context
    io
)

target_include_directories(gravity_simulation_exe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/io_context/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/gl_context/include/
)

target_compile_options(gravity_simulation_exe PRIVATE
    -Wall
    -Wextra
    -Wpedantic
#    -Werror
)

install(TARGETS gravity_simulation_exe
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
