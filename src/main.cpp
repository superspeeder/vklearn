#include "main.hpp"

#include <unordered_set>

App::App()
{
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

App::~App()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
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

void App::create_window()
{
    glfwInit();
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, false);

    m_window = glfwCreateWindow(800, 600, "Window", nullptr, nullptr);

    spdlog::debug("Created window.");
}

void App::create_instance()
{
    vk::ApplicationInfo application_info{};
    application_info.apiVersion = VK_API_VERSION_1_3;

    uint32_t ext_count;
    const char** exts = glfwGetRequiredInstanceExtensions(&ext_count);

    spdlog::debug("There are {} required instance extensions:", ext_count);
    for (uint32_t i = 0; i < ext_count; i++)
    {
        spdlog::debug("- {}", exts[i]);
    }

    m_instance = vk::createInstance(vk::InstanceCreateInfo({}, &application_info, 0, nullptr, ext_count, exts));

    spdlog::debug("Created Vulkan 1.3 instance.");
}

void App::select_gpu()
{
    m_physical_device = m_instance.enumeratePhysicalDevices()[0];

    auto props = m_physical_device.getProperties();
    spdlog::debug("Selected GPU: {}.", props.deviceName.data());
}

void App::pick_queue_families()
{
    const auto queue_families = m_physical_device.getQueueFamilyProperties();

    uint32_t index = 0;
    for (const auto& qf : queue_families)
    {
        if (qf.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            m_graphics_family = index;
        }

        if (m_physical_device.getSurfaceSupportKHR(index, m_surface))
        {
            m_present_family = index;
        }


        if (m_graphics_family.has_value() && m_present_family.has_value()) break;
    }

    spdlog::debug("Selected queue families: (g: {}, p: {}).", m_graphics_family.value(), m_present_family.value());
}

void App::create_surface()
{
    VkSurfaceKHR s;
    glfwCreateWindowSurface(m_instance, m_window, nullptr, &s);
    m_surface = s;

    spdlog::debug("Created surface.");
}

void App::create_device()
{
    const std::unordered_set<uint32_t> unique_queue_families = {
        m_graphics_family.value(),
        m_present_family.value(),
    };

    float queue_priorities[1] = {1.0f};
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    for (const auto& queue_family : unique_queue_families)
    {
        queue_create_infos.push_back(vk::DeviceQueueCreateInfo({}, queue_family, queue_priorities));
    }

    vk::DeviceCreateInfo device_create_info{};

    vk::PhysicalDeviceFeatures2 features{};
    features.features.geometryShader = true;
    features.features.tessellationShader = true;
    features.features.multiDrawIndirect = true;
    features.features.drawIndirectFirstInstance = true;
    features.features.fillModeNonSolid = true;
    features.features.wideLines = true;
    features.features.largePoints = true;

    vk::PhysicalDeviceVulkan13Features vk13features{};
    vk13features.synchronization2 = true;

    features.pNext = &vk13features;


    std::vector<const char*> exts = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };


    device_create_info.pNext = &features;
    device_create_info.setPEnabledExtensionNames(exts);
    device_create_info.setQueueCreateInfos(queue_create_infos);

    m_device = m_physical_device.createDevice(device_create_info);

    spdlog::debug("Created device.");

    m_graphics_queue = m_device.getQueue(m_graphics_family.value(), 0);
    m_present_queue = m_device.getQueue(m_present_family.value(), 0);
}

void App::create_swapchain()
{
    spdlog::debug("Configuring swapchain.");

    vk::SurfaceCapabilitiesKHR caps = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);

    vk::SwapchainCreateInfoKHR create_info{};

    create_info.clipped = true;
    create_info.surface = m_surface;
    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    create_info.preTransform = caps.currentTransform;
    create_info.oldSwapchain = m_swapchain;
    create_info.imageArrayLayers = 1;

    create_info.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && create_info.minImageCount > caps.maxImageCount)
    {
        create_info.minImageCount = caps.maxImageCount;
    }

    spdlog::debug("Swapchain will have >={} images.", create_info.minImageCount);

    std::vector<uint32_t> queue_family_indices;
    if (m_graphics_family.value() == m_present_family.value())
    {
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
    }
    else
    {
        queue_family_indices = {m_graphics_family.value(), m_present_family.value()};
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.setQueueFamilyIndices(queue_family_indices);
    }

    create_info.presentMode = select_present_mode();
    spdlog::debug("Swapchain present mode: {}.", vk::to_string(create_info.presentMode));

    const auto format = select_surface_format();

    m_swapchain_format = format.format;
    create_info.imageFormat = format.format;

    m_swapchain_color_space = format.colorSpace;
    create_info.imageColorSpace = format.colorSpace;

    spdlog::debug("Swapchain format: {}.", vk::to_string(create_info.imageFormat));
    spdlog::debug("Swapchain color space: {}.", vk::to_string(create_info.imageColorSpace));

    m_swapchain_extent = select_extent(caps);
    create_info.imageExtent = m_swapchain_extent;
    spdlog::debug("Swapchain extent: {} x {}.", m_swapchain_extent.width, m_swapchain_extent.height);

    auto old_swapchain = m_swapchain;

    m_swapchain = m_device.createSwapchainKHR(create_info);
    m_swapchain_images = m_device.getSwapchainImagesKHR(m_swapchain);

    spdlog::debug("Swapchain created with {} images.", m_swapchain_images.size());

    if (old_swapchain)
    {
        m_device.destroy(old_swapchain);
    }
}

void App::create_syncs()
{
    m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_image_available_semaphores[i] = m_device.createSemaphore({});
        m_render_finished_semaphores[i] = m_device.createSemaphore({});
        m_in_flight_fences[i] = m_device.createFence({vk::FenceCreateFlagBits::eSignaled});
    }

    spdlog::debug("Created sync objects for presentation.");
}

void App::create_command_pool()
{
    m_command_pool = m_device.createCommandPool(
        vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphics_family.value()));

    m_render_command_buffers = m_device.allocateCommandBuffers({m_command_pool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT});

    spdlog::debug("Created graphics command pool & allocated {} buffers", MAX_FRAMES_IN_FLIGHT);
}

vk::SurfaceFormatKHR App::select_surface_format()
{
    auto formats = m_physical_device.getSurfaceFormatsKHR(m_surface);
    for (const auto& format : formats)
    {
        if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && (format.format == vk::Format::eB8G8R8A8Srgb || format.format ==
            vk::Format::eR8G8B8A8Srgb))
        {
            return format;
        }
    }

    return formats[0];
}

vk::Extent2D App::select_extent(const vk::SurfaceCapabilitiesKHR& caps)
{
    if (caps.currentExtent.height == UINT32_MAX)
    {
        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);

        return {
            std::clamp(static_cast<uint32_t>(w), caps.minImageExtent.width, caps.maxImageExtent.width),
            std::clamp(static_cast<uint32_t>(h), caps.minImageExtent.height, caps.maxImageExtent.height),
        };
    }

    return caps.currentExtent;
}

vk::PresentModeKHR App::select_present_mode()
{
    auto present_modes = m_physical_device.getSurfacePresentModesKHR(m_surface);
    for (const auto& mode : present_modes)
    {
        if (mode == vk::PresentModeKHR::eMailbox) return mode;
    }

    return vk::PresentModeKHR::eFifo; // Fifo is a required mode so I don't need to check that it is present.
}

void App::mainloop()
{
    pre_mainloop();

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        render();
    }

    m_device.waitIdle();
}

void App::pre_mainloop()
{
    vk::Fence completeFence = m_device.createFence({});
    vk::CommandBuffer buffer = m_device.allocateCommandBuffers({m_command_pool, vk::CommandBufferLevel::ePrimary, 1})[0];

    buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    std::vector<vk::ImageMemoryBarrier> barriers;
    for (const auto& image : m_swapchain_images)
    {
        barriers.push_back({
            vk::AccessFlagBits::eNone, vk::AccessFlagBits::eNone,
            vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR, m_graphics_family.value(), m_present_family.value(),
            image, vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        });
    }

    buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barriers);
    buffer.end();

    vk::SubmitInfo submit_info{};
    submit_info.setCommandBuffers(buffer);
    m_graphics_queue.submit(submit_info, completeFence);

    m_device.waitForFences(completeFence, true, UINT64_MAX);

    m_device.freeCommandBuffers(m_command_pool, buffer);
}

void App::render()
{
    m_device.waitForFences(m_in_flight_fences[m_current_frame], true, UINT64_MAX);
    m_device.resetFences(m_in_flight_fences[m_current_frame]);

    uint32_t image_index = m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_image_available_semaphores[m_current_frame], {}).value;

    auto buf = m_render_command_buffers[m_current_frame];
    buf.reset();
    buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    buf.end();

    vk::SubmitInfo submit_info{};

    vk::PipelineStageFlags stage = vk::PipelineStageFlagBits::eTopOfPipe;
    submit_info.setCommandBuffers(buf);
    submit_info.setWaitSemaphores(m_image_available_semaphores[m_current_frame]);
    submit_info.setSignalSemaphores(m_render_finished_semaphores[m_current_frame]);
    submit_info.setWaitDstStageMask(stage);
    m_graphics_queue.submit(submit_info, m_in_flight_fences[m_current_frame]);

    vk::PresentInfoKHR present_info{};
    present_info.setSwapchains(m_swapchain);
    present_info.setImageIndices(image_index);
    present_info.setWaitSemaphores(m_render_finished_semaphores[m_current_frame]);
    m_present_queue.presentKHR(present_info);

    m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}


int main()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Hello");

    std::unique_ptr<App> app = std::make_unique<App>();

    app->mainloop();

    return 0;
}
