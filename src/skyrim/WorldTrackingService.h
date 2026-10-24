#pragma once

#include "Pch.h"
#include "PerfUI/WorldAnchor.h"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <atomic>

namespace PerfUI::Skyrim {

struct TrackedTarget {
    uint32_t id{ 0 };
    RE::ObjectRefHandle handle;
    RE::NiPoint3 staticPos{ 0.0f, 0.0f, 0.0f };
    RE::NiPoint3 offset{ 0.0f, 0.0f, 0.0f };
    bool isStatic{ false };
    bool checkLoS{ false };
    bool lastLoSResult{ true };
};

class WorldTrackingService {
public:
    static WorldTrackingService& GetSingleton();

    bool InstallHooks();

    uint32_t RegisterActor(RE::Actor* actor, const RE::NiPoint3& offset = { 0.0f, 0.0f, 120.0f }, bool checkLoS = false);
    uint32_t RegisterHandle(RE::ObjectRefHandle handle, const RE::NiPoint3& offset = { 0.0f, 0.0f, 0.0f }, bool checkLoS = false);
    uint32_t RegisterStatic(const RE::NiPoint3& worldPos, bool checkLoS = false);
    void Unregister(uint32_t targetId);
    void Clear();

    // Called on Game Thread inside Main::Update
    void Update();

    // Called on Render Thread inside D3D11Hook::Hooked_Present
    std::shared_ptr<const WorldAnchorSnapshot> GetLatestSnapshot() const;

private:
    WorldTrackingService() = default;
    ~WorldTrackingService() = default;

    std::mutex m_mutex;
    std::unordered_map<uint32_t, TrackedTarget> m_targets;
    uint32_t m_nextId{ 1 };
    size_t m_losIndex{ 0 };
    uint64_t m_frameCounter{ 0 };

    std::atomic<std::shared_ptr<const WorldAnchorSnapshot>> m_latestSnapshot{ nullptr };
};

} // namespace PerfUI::Skyrim
