
#if __clang__
#include "headersNondependent.h"
#endif

enum IMAGE_FORMAT
{
    IMAGE_FORMAT_NULL,
    IMAGE_FORMAT_RAW_RGBA32,
    IMAGE_FORMAT_PNG_FILTERED,
    IMAGE_FORMAT_PNG_COMPRESSED,
    IMAGE_FORMAT_PNG_FINAL,
};

struct BpImage
{
    void *data;
    unsigned int dataSize;
    V2 dim;
    IMAGE_FORMAT imageFormat;
};

static const char *G_PNG_FILTER_NAMES[] = {"", "Sub", "Up", "Average", "Paeth", "Optimal"};