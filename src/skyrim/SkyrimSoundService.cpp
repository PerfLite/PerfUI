#include "SkyrimSoundService.h"
#include "Pch.h"
#include <string>

namespace PerfUI::Skyrim {

void SkyrimSoundService::PlayUISound(const char* editorId) {
    if (!editorId || !*editorId) return;

    SKSE::GetTaskInterface()->AddTask([soundName = std::string(editorId)]() {
        auto* audioManager = RE::BSAudioManager::GetSingleton();
        if (!audioManager) return;

        RE::BSSoundHandle handle;
        audioManager->BuildSoundDataFromEditorID(handle, soundName.c_str(), 0x1A);
        if (handle.IsValid()) {
            handle.Play();
        }
    });
}

} // namespace PerfUI::Skyrim
