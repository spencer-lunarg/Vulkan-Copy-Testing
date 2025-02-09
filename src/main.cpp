#include <vulkan/vulkan_core.h>
#include "context.h"
#include "factory.h"

void Test(Context& cx) {
    vct::Image image(cx, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UINT, {8, 8, 1}, VK_IMAGE_USAGE_STORAGE_BIT);
    cx.SetLayout(image, VK_IMAGE_LAYOUT_GENERAL);
    vct::Buffer output(cx, 4096);

    vct::Buffer src(cx, 4096);
    src.Set(0x33);

    vct::DescriptorSet descriptor_set(cx);
    descriptor_set.Update(0, output);
    descriptor_set.Update(1, image);
    vct::Pipeline pipeline(cx, descriptor_set.layout_, "simple.comp.spv");

    cx.BeginCmd();
    VkBufferImageCopy region = {0, 0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0, 0, 0}, {2, 2, 1}};
    cx.CopyBufferToImage(src, image, &region);
    cx.EndCmd();
    cx.Submit();

    cx.BeginCmd();
    cx.Dispatch(pipeline, descriptor_set);
    cx.EndCmd();
    cx.Submit();

    output.Print(64);

    // vct::Buffer dst(cx, 4096);

    // uint32_t *src_ptr = static_cast<uint32_t *>(src.Map());
    // src_ptr[0] = 0x11111111;
    // src_ptr[1] = 0x22222222;
    // src_ptr[2] = 0x33333333;
    // src_ptr[3] = 0;
    // src_ptr[4] = 5;
    // src_ptr[5] = 6;
    // src_ptr[6] = 7;
    // src.Unmap();

    // cx.BeginCmd();
    // VkBufferCopy region = {0, 0, 128};
    // cx.CopyBuffer(src, dst, &region);
    // cx.EndCmd();
    // cx.Submit();

    // dst.Print(128);
}

int main() {
    Context cx;
    if (!cx.Setup()) return -1;
    Test(cx);

    return 0;
}
