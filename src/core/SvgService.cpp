#include "PerfUI/SvgService.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

#pragma warning(push)
#pragma warning(disable: 4244 4456 4702)
#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>
#pragma warning(pop)

namespace PerfUI {

SvgRasterResult SvgService::RasterizeFile(std::string_view filePath, float targetWidth, float targetHeight, float dpi) {
    std::string pathStr(filePath);
    NSVGimage* image = nsvgParseFromFile(pathStr.c_str(), "px", dpi > 0.0f ? dpi : 96.0f);
    if (!image) return {};

    float origW = image->width;
    float origH = image->height;
    if (origW <= 0.0f) origW = 32.0f;
    if (origH <= 0.0f) origH = 32.0f;

    float outW = targetWidth > 0.0f ? targetWidth : origW;
    float outH = targetHeight > 0.0f ? targetHeight : origH;

    if (targetWidth > 0.0f && targetHeight <= 0.0f) {
        outH = targetWidth * (origH / origW);
    } else if (targetHeight > 0.0f && targetWidth <= 0.0f) {
        outW = targetHeight * (origW / origH);
    }

    uint32_t w = static_cast<uint32_t>(std::max(1.0f, std::round(outW)));
    uint32_t h = static_cast<uint32_t>(std::max(1.0f, std::round(outH)));

    float scaleX = static_cast<float>(w) / origW;
    float scaleY = static_cast<float>(h) / origH;
    float scale = std::min(scaleX, scaleY);

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return {};
    }

    SvgRasterResult result;
    result.width = w;
    result.height = h;
    result.rgbaPixels.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4, 0);

    float tx = (static_cast<float>(w) - (origW * scale)) * 0.5f;
    float ty = (static_cast<float>(h) - (origH * scale)) * 0.5f;

    nsvgRasterize(rast, image, tx, ty, scale, result.rgbaPixels.data(), static_cast<int>(w), static_cast<int>(h), static_cast<int>(w * 4));

    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    return result;
}

SvgRasterResult SvgService::RasterizeString(std::string_view svgContent, float targetWidth, float targetHeight, float dpi) {
    if (svgContent.empty()) return {};

    std::string buffer(svgContent);
    NSVGimage* image = nsvgParse(buffer.data(), "px", dpi > 0.0f ? dpi : 96.0f);
    if (!image) return {};

    float origW = image->width;
    float origH = image->height;
    if (origW <= 0.0f) origW = 32.0f;
    if (origH <= 0.0f) origH = 32.0f;

    float outW = targetWidth > 0.0f ? targetWidth : origW;
    float outH = targetHeight > 0.0f ? targetHeight : origH;

    if (targetWidth > 0.0f && targetHeight <= 0.0f) {
        outH = targetWidth * (origH / origW);
    } else if (targetHeight > 0.0f && targetWidth <= 0.0f) {
        outW = targetHeight * (origW / origH);
    }

    uint32_t w = static_cast<uint32_t>(std::max(1.0f, std::round(outW)));
    uint32_t h = static_cast<uint32_t>(std::max(1.0f, std::round(outH)));

    float scaleX = static_cast<float>(w) / origW;
    float scaleY = static_cast<float>(h) / origH;
    float scale = std::min(scaleX, scaleY);

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return {};
    }

    SvgRasterResult result;
    result.width = w;
    result.height = h;
    result.rgbaPixels.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4, 0);

    float tx = (static_cast<float>(w) - (origW * scale)) * 0.5f;
    float ty = (static_cast<float>(h) - (origH * scale)) * 0.5f;

    nsvgRasterize(rast, image, tx, ty, scale, result.rgbaPixels.data(), static_cast<int>(w), static_cast<int>(h), static_cast<int>(w * 4));

    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    return result;
}

} // namespace PerfUI
