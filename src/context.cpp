#include "context.h"
#include "factory.h"
#include <cstdint>
#include <iostream>
#include <vulkan/utility/vk_struct_helper.hpp>

Context::Context() {}

Context::~Context() {
    if (cmd_pool_ != VK_NULL_HANDLE) {
        dt_.destroyCommandPool(cmd_pool_, nullptr);
    }

    vkb::destroy_device(device_);
    vkb::destroy_instance(instance_);
}

bool Context::Setup() {
    vkb::InstanceBuilder instance_builder;
    instance_builder.set_app_name("Vulkan Copy Testing").set_engine_name("LunarG").require_api_version(1, 3, 0);
    auto instance_builder_return = instance_builder.build();
    if (!instance_builder_return) {
        std::cerr << instance_builder_return.error().message() << "\n";
        return false;
    }
    instance_ = instance_builder_return.value();
    instance_dt_ = instance_.make_table();

    auto system_info_ret = vkb::SystemInfo::get_system_info();
    if (!system_info_ret) {
        std::cerr << system_info_ret.error().message().c_str() << "\n";
        return false;
    }
    auto system_info = system_info_ret.value();

    vkb::PhysicalDeviceSelector selector{instance_};
    auto phys_ret = selector.set_minimum_version(1, 3).defer_surface_initialization().select();
    if (!phys_ret) {
        std::cerr << phys_ret.error().message() << "\n";
        return false;
    }

    vkb::DeviceBuilder device_builder{phys_ret.value()};
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        std::cerr << dev_ret.error().message() << "\n";
        return false;
    }
    device_ = dev_ret.value();
    dt_ = device_.make_table();

    auto graphics_queue_ret = device_.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        std::cerr << graphics_queue_ret.error().message() << "\n";
        return false;
    }
    queue_ = graphics_queue_ret.value();

    VkCommandPoolCreateInfo pool_info = vku::InitStructHelper();
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = device_.get_queue_index(vkb::QueueType::graphics).value();
    if (dt_.createCommandPool(&pool_info, nullptr, &cmd_pool_) != VK_SUCCESS) {
        std::cout << "failed to create command pool\n";
        return false;
    }

    VkCommandBufferAllocateInfo cb_allocate_info = vku::InitStructHelper();
    cb_allocate_info.commandPool = cmd_pool_;
    cb_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_allocate_info.commandBufferCount = 1;

    if (dt_.allocateCommandBuffers(&cb_allocate_info, &cmd_buffer_) != VK_SUCCESS) {
        std::cout << "failed to allocate command buffer\n";
        return false;
    }

    return true;
}

uint32_t Context::GetHostVisibleMemoryIndex(const uint32_t type_bits) const {
    uint32_t type_mask = type_bits;
    VkPhysicalDeviceMemoryProperties mem_props = device_.physical_device.memory_properties;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_mask & 1) == 1) {
            if ((mem_props.memoryTypes[i].propertyFlags &
                 (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) != 0) {
                return i;
            }
        }
        type_mask >>= 1;
    }
    return UINT32_MAX;
}

void Context::BeginCmd() {
    VkCommandBufferBeginInfo begin_info = vku::InitStructHelper();
    begin_info.flags = 0;
    dt_.beginCommandBuffer(cmd_buffer_, &begin_info);
}

void Context::EndCmd() { dt_.endCommandBuffer(cmd_buffer_); }

void Context::Submit() {
    VkSubmitInfo submit_info = vku::InitStructHelper();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer_;

    if (dt_.queueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        std::cout << "failed to submit\n";
        return;
    }
    dt_.queueWaitIdle(queue_);
}

void Context::CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, const VkBufferCopy* region) {
    dt_.cmdCopyBuffer(cmd_buffer_, src_buffer, dst_buffer, 1, region);
}
