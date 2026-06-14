// Single translation unit that provides the stb_image implementation for the
// asset library. The renderer links the same stb headers but never defines the
// implementation, so there is no duplicate-symbol conflict when both static
// libraries are linked into one executable.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>
