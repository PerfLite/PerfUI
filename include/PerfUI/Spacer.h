#pragma once

#include "UIElement.h"
#include "Types.h"

namespace PerfUI {

class PERFUI_API Spacer : public UIElement {
public:
    explicit Spacer(float size = 0.0f, float flexWeight = 0.0f, std::string name = "Spacer");
    ~Spacer() override = default;

    Spacer& size(float px);
    Spacer& flex(float weight);

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override {
        // Spacers do not render any graphics
        (void)backend;
    }

private:
    float m_fixedSize{ 0.0f };
};

} // namespace PerfUI
