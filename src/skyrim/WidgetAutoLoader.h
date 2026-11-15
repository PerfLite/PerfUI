#pragma once

#include "PerfUI/UIContext.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <filesystem>

namespace PerfUI {
class ScriptHost;
}

namespace PerfUI::Skyrim {

class WidgetAutoLoader {
public:
    static WidgetAutoLoader& GetSingleton() {
        static WidgetAutoLoader instance;
        return instance;
    }

    static void RegisterSkyrimLuaBindings(ScriptHost& host);

    void Initialize(UIContext* ctx);
    void ScanAndLoad();

private:
    struct LoadedWidget;

    UIContext* m_context{ nullptr };
    std::vector<std::shared_ptr<LoadedWidget>> m_widgets;
    std::mutex m_mutex;
    bool m_initialized{ false };
};

} // namespace PerfUI::Skyrim
