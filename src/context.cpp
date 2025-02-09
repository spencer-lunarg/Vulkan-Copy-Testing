#include "context.h"
#include <vulkan/vulkan_core.h>
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
    selector.set_minimum_version(1, 3).defer_surface_initialization();
    selector.add_required_extension("VK_KHR_maintenance5");
    VkPhysicalDeviceMaintenance5Features features_maintenance5 = vku::InitStructHelper();
    features_maintenance5.maintenance5 = true;
    VkPhysicalDeviceVulkan13Features features_13 = vku::InitStructHelper();
    features_13.synchronization2 = true;
    selector.add_required_extension_features(features_maintenance5).set_required_features_13(features_13);
    auto phys_ret = selector.select();
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

void Context::SetLayout(vct::Image& image, VkImageLayout new_layout) {
    if (image.layout_ == new_layout) return;
    BeginCmd();

    VkImageMemoryBarrier2 image_barrier = vku::InitStructHelper();
    image_barrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    image_barrier.srcAccessMask = 0;
    image_barrier.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    image_barrier.oldLayout = image.layout_;
    image_barrier.newLayout = new_layout;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = image;
    image_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkDependencyInfo dependency_info = vku::InitStructHelper();
    dependency_info.dependencyFlags = 0;
    dependency_info.bufferMemoryBarrierCount = 0;
    dependency_info.memoryBarrierCount = 0;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_barrier;

    dt_.cmdPipelineBarrier2(cmd_buffer_, &dependency_info);

    EndCmd();
    Submit();

    image.layout_ = new_layout;
}

void Context::CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, const VkBufferCopy* region) {
    dt_.cmdCopyBuffer(cmd_buffer_, src_buffer, dst_buffer, 1, region);
}

void Context::CopyBufferToImage(VkBuffer buffer, const vct::Image& image, const VkBufferImageCopy* region) {
    dt_.cmdCopyBufferToImage(cmd_buffer_, buffer, image, image.layout_, 1, region);
}

void Context::Dispatch(const vct::Pipeline& pipeline, const vct::DescriptorSet& descriptor_set) {
    dt_.cmdBindPipeline(cmd_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    dt_.cmdBindDescriptorSets(cmd_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout_, 0, 1, &descriptor_set.handle_, 0,
                              nullptr);
    dt_.cmdDispatch(cmd_buffer_, 1, 1, 1);
}