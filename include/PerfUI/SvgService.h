#pragma once

#include "PerfUI/Export.h"
#include <vector>
#include <string>
#include <string_view>
#include <cstdint>

namespace PerfUI {

struct PERFUI_API SvgRasterResult {
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    std::vector<uint8_t> rgbaPixels;
    bool valid() const { return width > 0 && height > 0 && !rgbaPixels.empty(); }
};

class PERFUI_API SvgService {
public:
    /// Rasterize an SVG file from disk into raw 32-bit RGBA pixel buffer.
    /// If targetWidth or targetHeight <= 0, natural SVG dimensions (from viewBox/width/height) are used.
    static SvgRasterResult RasterizeFile(std::string_view filePath, float targetWidth = 0.0f, float targetHeight = 0.0f, float dpi = 96.0f);

    /// Rasterize SVG XML content from memory/string into raw 32-bit RGBA pixel buffer.
    /// If targetWidth or targetHeight <= 0, natural SVG dimensions (from viewBox/width/height) are used.
    static SvgRasterResult RasterizeString(std::string_view svgContent, float targetWidth = 0.0f, float targetHeight = 0.0f, float dpi = 96.0f);
};

} // namespace PerfUI
