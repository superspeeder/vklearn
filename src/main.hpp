#pragma once

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>


class App
{
public:
    App();

    ~App();

    void create_window();
    void create_instance();
    void select_gpu();
    void pick_queue_families();
    void create_surface();
    void create_device();
    void create_swapchain();
    void create_syncs();
    void create_command_pool();
    vk::SurfaceFormatKHR select_surface_format();
    vk::Extent2D select_extent(const vk::SurfaceCapabilitiesKHR& caps);
    vk::PresentModeKHR select_present_mode();

    void mainloop();

    void pre_mainloop();
    void render();

private:
    GLFWwindow* m_window;

    vk::Instance m_instance;
    vk::PhysicalDevice m_physical_device;
    vk::SurfaceKHR m_surface;
    vk::Device m_device;
    vk::SwapchainKHR m_swapchain;

    vk::Format m_swapchain_format;
    vk::ColorSpaceKHR m_swapchain_color_space;
    vk::Extent2D m_swapchain_extent;
    std::vector<vk::Image> m_swapchain_images;

    std::vector<vk::Semaphore> m_image_available_semaphores;
    std::vector<vk::Semaphore> m_render_finished_semaphores;
    std::vector<vk::Fence> m_in_flight_fences;

    uint32_t m_current_frame = 0;

    vk::CommandPool m_command_pool;
    std::vector<vk::CommandBuffer> m_render_command_buffers;

    std::optional<uint32_t> m_graphics_family;
    std::optional<uint32_t> m_present_family;

    vk::Queue m_graphics_queue;
    vk::Queue m_present_queue;
};

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
