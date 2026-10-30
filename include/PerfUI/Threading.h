#pragma once

// PerfUI threading contract
// ---------------------------------------------------------------------------
// * Render thread (the D3D11 Present hook): owns UIContext, layout and every
//   UIElement. Only this thread may call update/layout/render or touch widget
//   trees.
// * Game thread / SKSE tasks: never touch widget trees or draw directly.
//   Publish plain data through Snapshot<T> (latest value wins) or EventQueue<T>
//   (ordered events such as floating text), and let the render thread consume it.
// * Texture creation (loadTexture/loadSvg) is mutex-guarded inside the backend
//   and may be called from any thread; draw calls must stay on the render thread.
// * Overlay callbacks run on the render thread inside a fault guard: throwing or
//   crashing disables that overlay for the session instead of killing the game.
// * Keep T small and self-contained: resolve game objects into plain data before
//   publishing, never hand raw engine pointers to the render thread.

#include <cstdint>
#include <iterator>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace PerfUI {

// Latest-value hand-off: producers overwrite, the consumer reads when it needs to.
template <typename T>
class Snapshot {
public:
    void publish(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value = std::move(value);
        ++m_version;
    }

    std::optional<T> read() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_value;
    }

    // Copies the value only when it changed since the previous successful call.
    bool readChanged(T& out, uint64_t& seenVersion) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_value.has_value() || m_version == seenVersion) {
            return false;
        }
        out = *m_value;
        seenVersion = m_version;
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_value.has_value();
    }

private:
    mutable std::mutex m_mutex;
    std::optional<T> m_value;
    uint64_t m_version{ 0 };
};

// Ordered event hand-off: producers append, the consumer drains once per frame.
template <typename T>
class EventQueue {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_items.push_back(std::move(value));
    }

    std::vector<T> drain() {
        std::vector<T> out;
        std::lock_guard<std::mutex> lock(m_mutex);
        out.swap(m_items);
        return out;
    }

    void drainInto(std::vector<T>& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        out.insert(out.end(), std::make_move_iterator(m_items.begin()), std::make_move_iterator(m_items.end()));
        m_items.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_items.size();
    }

private:
    mutable std::mutex m_mutex;
    std::vector<T> m_items;
};

} // namespace PerfUI
