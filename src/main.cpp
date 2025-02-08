#include "context.h"
#include "factory.h"

int main() {
    Context cx;
    if (!cx.Setup()) return -1;

    vct::Buffer src(cx, 4096);
    vct::Buffer dst(cx, 4096);
    dst.Set(0);

    uint32_t *src_ptr = static_cast<uint32_t *>(src.Map());
    src_ptr[0] = 0x11111111;
    src_ptr[1] = 0x22222222;
    src_ptr[2] = 0x33333333;
    src_ptr[3] = 0;
    src_ptr[4] = 5;
    src_ptr[5] = 6;
    src_ptr[6] = 7;
    src.Unmap();

    cx.BeginCmd();
    VkBufferCopy region = {0, 0, 128};
    cx.CopyBuffer(src, dst, &region);
    cx.EndCmd();
    cx.Submit();

    dst.Print(128);

    return 0;
}
