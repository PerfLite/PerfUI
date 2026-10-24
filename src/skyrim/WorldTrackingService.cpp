#include "WorldTrackingService.h"
#include <cmath>
#include <algorithm>

namespace PerfUI::Skyrim {

namespace {

struct MainUpdateHook {
    static void thunk(RE::Main* a_this, float a_budget) {
        static std::atomic<bool> s_loggedThread{ false };
        if (!s_loggedThread.exchange(true)) {
            SKSE::log::info(">>> Thread Check: Main::Update Game Thread ID = {} (Main::threadID = {})",
                ::GetCurrentThreadId(), a_this ? a_this->threadID : 0);
        }

        func(a_this, a_budget);

        WorldTrackingService::GetSingleton().Update();
    }
    static inline REL::Relocation<decltype(thunk)> func;
};

} // namespace

WorldTrackingService& WorldTrackingService::GetSingleton() {
    static WorldTrackingService s_instance;
    return s_instance;
}

bool WorldTrackingService::InstallHooks() {
    static std::atomic<bool> s_installed{ false };
    if (s_installed.exchange(true)) return true;

    auto& trampoline = SKSE::GetTrampoline();
    REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(35551, 36544) };
    MainUpdateHook::func = trampoline.write_branch<5>(target.address(), MainUpdateHook::thunk);

    SKSE::log::info("WorldTrackingService: Main::Update hook installed at 0x{:X}", target.address());
    return true;
}

uint32_t WorldTrackingService::RegisterActor(RE::Actor* actor, const RE::NiPoint3& offset, bool checkLoS) {
    if (!actor) return 0;
    return RegisterHandle(actor->GetHandle(), offset, checkLoS);
}

uint32_t WorldTrackingService::RegisterHandle(RE::ObjectRefHandle handle, const RE::NiPoint3& offset, bool checkLoS) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t id = m_nextId++;
    TrackedTarget target;
    target.id = id;
    target.handle = handle;
    target.offset = offset;
    target.isStatic = false;
    target.checkLoS = checkLoS;
    m_targets[id] = target;
    return id;
}

uint32_t WorldTrackingService::RegisterStatic(const RE::NiPoint3& worldPos, bool checkLoS) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t id = m_nextId++;
    TrackedTarget target;
    target.id = id;
    target.staticPos = worldPos;
    target.offset = { 0.0f, 0.0f, 0.0f };
    target.isStatic = true;
    target.checkLoS = checkLoS;
    m_targets[id] = target;
    return id;
}

void WorldTrackingService::Unregister(uint32_t targetId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_targets.erase(targetId);
}

void WorldTrackingService::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_targets.clear();
}

std::shared_ptr<const WorldAnchorSnapshot> WorldTrackingService::GetLatestSnapshot() const {
    return m_latestSnapshot.load(std::memory_order_acquire);
}

void WorldTrackingService::Update() {
    auto* camera = RE::Main::WorldRootCamera();
    if (!camera) {
        return;
    }

    auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
    float sw = 1920.0f;
    float sh = 1080.0f;
    if (renderer && renderer->data.renderWindows[0].windowWidth > 0) {
        sw = static_cast<float>(renderer->data.renderWindows[0].windowWidth);
        sh = static_cast<float>(renderer->data.renderWindows[0].windowHeight);
    }

    RE::NiPoint3 camPos = camera->world.translate;

    auto snapshot = std::make_shared<WorldAnchorSnapshot>();
    snapshot->screenWidth = sw;
    snapshot->screenHeight = sh;
    snapshot->frameNumber = ++m_frameCounter;

    std::vector<TrackedTarget> targetsCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_targets.empty()) {
            m_latestSnapshot.store(snapshot, std::memory_order_release);
            return;
        }
        targetsCopy.reserve(m_targets.size());
        for (const auto& [id, t] : m_targets) {
            targetsCopy.push_back(t);
        }
    }

    // Throttled Line-of-Sight checks (up to 4 checks per frame)
    constexpr size_t maxLoSPerFrame = 4;
    size_t losCheckedCount = 0;
    auto* player = RE::PlayerCharacter::GetSingleton();

    const auto& rtData = camera->GetRuntimeData();
    const auto& rtData2 = camera->GetRuntimeData2();

    for (auto& target : targetsCopy) {
        WorldAnchorPoint pt;
        pt.targetId = target.id;

        RE::NiPoint3 worldPos;
        RE::NiPointer<RE::TESObjectREFR> refr;

        if (target.isStatic) {
            worldPos = target.staticPos + target.offset;
        } else {
            refr = target.handle.get();
            if (!refr || refr->IsDeleted()) {
                pt.isValid = false;
                snapshot->points[target.id] = pt;
                continue;
            }
            worldPos = refr->GetPosition() + target.offset;
        }

        float dx = worldPos.x - camPos.x;
        float dy = worldPos.y - camPos.y;
        float dz = worldPos.z - camPos.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (target.checkLoS && player && refr) {
            if (losCheckedCount < maxLoSPerFrame) {
                bool dummy = false;
                target.lastLoSResult = player->HasLineOfSight(refr.get(), dummy);
                losCheckedCount++;
            }
            pt.hasLineOfSight = target.lastLoSResult;
        } else {
            pt.hasLineOfSight = true;
        }

        float screenX = 0.0f;
        float screenY = 0.0f;
        float screenZ = 0.0f;

        bool inFrustum = RE::NiCamera::WorldPtToScreenPt3(
            rtData.worldToCam,
            rtData2.port,
            worldPos,
            screenX, screenY, screenZ,
            1e-5f
        );

        pt.screenPos = { screenX * sw, (1.0f - screenY) * sh };
        pt.depth = screenZ;
        pt.distance = dist;
        pt.inFrontOfCamera = (screenZ > 0.0f && screenZ <= 1.0f);
        pt.inFrustum = inFrustum;
        pt.isValid = true;

        snapshot->points[target.id] = pt;
    }

    // Update state cache for LoS in registry
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& t : targetsCopy) {
            auto it = m_targets.find(t.id);
            if (it != m_targets.end()) {
                it->second.lastLoSResult = t.lastLoSResult;
            }
        }
    }

    m_latestSnapshot.store(snapshot, std::memory_order_release);
}

} // namespace PerfUI::Skyrim
