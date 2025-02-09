#include <vulkan/vulkan_core.h>
#include "VkBootstrap.h"

namespace vct {
class Image;
class Pipeline;
class DescriptorSet;
}  // namespace vct

class Context {
  public:
    Context();
    ~Context();

    bool Setup();
    uint32_t GetHostVisibleMemoryIndex(const uint32_t type_bits) const;

    void BeginCmd();
    void EndCmd();
    void Submit();
    void SetLayout(vct::Image& image, VkImageLayout new_layout);
    void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, const VkBufferCopy* region);
    void CopyBufferToImage(VkBuffer buffer, const vct::Image& image, const VkBufferImageCopy* region);
    void Dispatch(const vct::Pipeline& pipeline, const vct::DescriptorSet& descriptor_set);

    vkb::Instance instance_;
    vkb::InstanceDispatchTable instance_dt_;
    VkSurfaceKHR surface_;
    vkb::Device device_;
    vkb::DispatchTable dt_;
    vkb::Swapchain swapchain_;
    VkQueue queue_;
    VkCommandPool cmd_pool_;
    VkCommandBuffer cmd_buffer_;
};