set(X11BACKEND_SOURCES
    logging.cpp
    egl_x11_backend.cpp
    scene_qpainter_x11_backend.cpp
    x11windowed_backend.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/platformsupport/scenes/opengl)

if(Vulkan_FOUND)
    set(X11BACKEND_SOURCES ${X11BACKEND_SOURCES} x11_vulkan_backend.cpp)
endif()

add_library(KWinWaylandX11Backend MODULE ${X11BACKEND_SOURCES})
set_target_properties(KWinWaylandX11Backend PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/org.kde.kwin.waylandbackends/")
target_link_libraries(KWinWaylandX11Backend eglx11common kwin kwinxrenderutils X11::XCB SceneQPainterBackend SceneOpenGLBackend)
if(X11_Xinput_FOUND)
    target_link_libraries(KWinWaylandX11Backend ${X11_Xinput_LIB})
endif()

if(Vulkan_FOUND)
target_link_libraries(KWinWaylandX11Backend kwin X11::XCB SceneVulkanBackend kwinvulkanutils)
endif()

install(
    TARGETS
        KWinWaylandX11Backend
    DESTINATION
        ${PLUGIN_INSTALL_DIR}/org.kde.kwin.waylandbackends/
)
