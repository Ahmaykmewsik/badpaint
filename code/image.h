
#if __clang__
#include "headersNondependent.h"
#endif

enum IMAGE_FORMAT
{
    IMAGE_FORMAT_NULL,
    IMAGE_FORMAT_R8G8B8A8,
    IMAGE_FORMAT_PDF,
};

struct BpImage
{
    void *data;
    unsigned int dataSize;
    V2 dim;
    IMAGE_FORMAT imageFormat;
};