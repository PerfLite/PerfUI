#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <functional>
#include <chrono>

#include "PerfUI/UIContext.h"
#include "PerfUI/UIElement.h"
#include "MarkupTypes.h"
#include "WidgetRegistry.h"
#include "ICallbackResolver.h"

namespace PerfUI {

class PERFUI_API MarkupLoader {
public:
    explicit MarkupLoader(UIContext& ctx);
    ~MarkupLoader();

    UIElement* loadFile(const std::filesystem::path& file, UIElement* parent = nullptr);
    UIElement* loadString(std::string_view xml, UIElement* parent = nullptr);
    UIElement* findByName(std::string_view name) const;
    void bindCallback(std::string name, std::function<void()> cb);
    // One-way value binding for <Text bind="key"/>. Values survive XML hot reload.
    void setBinding(std::string key, std::string value);
    void setCallbackResolver(ICallbackResolver* resolver) { m_resolver = resolver; }
    ICallbackResolver* callbackResolver() const { return m_resolver; }
    const std::string& scriptAttribute() const { return m_scriptAttribute; }
    void onScriptReload(std::function<void(const std::string&)> cb) { m_onScriptReload = std::move(cb); }
    // The old root remains valid in onBeforeReload; the new root is attached in onAfterReload.
    void onBeforeReload(std::function<void(UIElement*)> cb);
    void onAfterReload(std::function<void(UIElement*)> cb);
    LoadResult lastResult() const { return m_lastResult; }

    void enableHotReload(bool on);
    bool isHotReloadEnabled() const { return m_hotReloadEnabled; }
    void poll();
    void setPollInterval(std::chrono::milliseconds interval) { m_pollInterval = interval; }

    UIElement* loadedRoot() const { return m_root; }
    const std::filesystem::path& loadedFilePath() const { return m_sourceFilePath; }

    void clearResult() { m_lastResult = LoadResult{}; }

    // Stylesheet (PSS) API
    bool loadStyles(const std::filesystem::path& path);
    void reloadStyles();

private:
    UIElement* parseAndBuildTree(std::string_view xml, UIElement* parent);
    void setupElementProperties(UIElement* element, const std::string& tag, const AttrMap& attrs, int lineNumber);

    UIContext& m_context;
    UIElement* m_parent{ nullptr };
    UIElement* m_root{ nullptr };
    std::filesystem::path m_sourceFilePath;

    CallbackMap m_callbacks;
    ICallbackResolver* m_resolver{ nullptr };
    std::string m_scriptAttribute;
    std::function<void(const std::string&)> m_onScriptReload;
    LoadResult m_lastResult;

    bool m_hotReloadEnabled{ false };
    std::filesystem::file_time_type m_lastWriteTime{};
    std::chrono::steady_clock::time_point m_lastCheckTime{};
    std::chrono::milliseconds m_pollInterval{ 500 };
};

} // namespace PerfUI
