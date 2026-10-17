#include "PerfUI/Spacer.h"

namespace PerfUI {

Spacer::Spacer(float size, float flexWeight, std::string name)
    : UIElement(std::move(name))
    , m_fixedSize(size)
{
    if (flexWeight > 0.0f) {
        layout().flex(flexWeight);
    } else if (size > 0.0f) {
        m_desiredSize = Dimensions{ size, size };
    }
}

Spacer& Spacer::size(float px) {
    m_fixedSize = px;
    markLayoutDirty();
    return *this;
}

Spacer& Spacer::flex(float weight) {
    layout().flex(weight);
    markLayoutDirty();
    return *this;
}

void Spacer::measure(Dimensions availableSize) {
    (void)availableSize;
    if (m_fixedSize > 0.0f) {
        m_desiredSize = Dimensions{ m_fixedSize, m_fixedSize };
    } else {
        m_desiredSize = Dimensions{ 0.0f, 0.0f };
    }
}

} // namespace PerfUI
