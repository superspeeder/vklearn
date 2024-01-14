#include "main.hpp"

#include <unordered_set>

App::App() {
    create_window();
    create_instance();
    create_surface();
    select_gpu();
    pick_queue_families();
    create_device();
    create_swapchain();
    create_syncs();
    create_command_pool();
}

App::~App() {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_device.destroy(m_in_flight_fences[i]);
        m_device.destroy(m_render_finished_semaphores[i]);
        m_device.destroy(m_image_available_semaphores[i]);
    }

    m_device.destroy(m_command_pool);
    m_device.destroy(m_swapchain);
    m_device.destroy();

    m_instance.destroy(m_surface);
    m_instance.destroy();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void App::create_window() {
    glfwInit();
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, false);

    m_window = glfwCreateWindow(800, 600, "Window", nullptr, nullptr);
}

void App::create_instance() {
    vk::ApplicationInfo application_info{};
    application_info.apiVersion = VK_API_VERSION_1_3;

    uint32_t ext_count;
    const char **exts = glfwGetRequiredInstanceExtensions(&ext_count);

    m_instance = vk::createInstance(vk::InstanceCreateInfo({}, &application_info, 0, nullptr, ext_count, exts));
}

void App::select_gpu() {
  m_physical_device = m_instance.enumeratePhysicalDevices()[0];
}

void App::pick_queue_families() {

}

void App::create_surface() {
    VkSurfaceKHR s;
    glfwCreateWindowSurface(m_instance, m_window, nullptr, &s);
    m_surface = s;
}

void App::create_device() {

  const std::unordered_set<uint32_t> unique_queue_families = {
            m_graphics_family.value(),
            m_present_family.value(),
    };

    float queue_priorities[1] = {1.0f };
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    for (const auto& queue_family : unique_queue_families) {
        queue_create_infos.push_back(vk::DeviceQueueCreateInfo({}, queue_family, queue_priorities));
    }


    vk::DeviceCreateInfo device_create_info{};


    vk::PhysicalDeviceFeatures2 features{
        .features = vk::PhysicalDeviceFeatures{
            .geometryShader = true,
            .tessellationShader = true,
            .multiDrawIndirect = true,
            .drawIndirectFirstInstance = true,
            .fillModeNonSolid = true,
            .wideLines = true,
            .largePoints = true,
        }
    };

    device_create_info.pNext = &features;

    m_device = m_physical_device.createDevice()

}

void App::create_swapchain() {

}

void App::create_syncs() {

}

void App::create_command_pool() {

}


int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Hello");


    return 0;
}
