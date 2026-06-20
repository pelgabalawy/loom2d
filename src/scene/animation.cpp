#include "scene/animation.hpp"
#include <stdexcept>

namespace loom {

Animation::Animation(std::string name, bool loop)
    : name(std::move(name)), loop(loop) {}

void Animation::add_frame(Rect source, float duration) {
    m_frames.push_back({source, duration});
}

void Animation::add_strip(int sheet_w, int sheet_h,
                           int frame_w, int frame_h,
                           int cols, int rows, float frame_duration)
{
    (void)sheet_w; (void)sheet_h;
    for (int row = 0; row < rows; ++row)
        for (int col = 0; col < cols; ++col)
            add_frame({static_cast<float>(col * frame_w),
                       static_cast<float>(row * frame_h),
                       static_cast<float>(frame_w),
                       static_cast<float>(frame_h)},
                      frame_duration);
}

void Animation::update(float dt) {
    if (m_done || m_frames.empty()) return;
    m_elapsed += dt;
    while (m_elapsed >= m_frames[m_index].duration) {
        m_elapsed -= m_frames[m_index].duration;
        m_index++;
        if (m_index >= static_cast<int>(m_frames.size())) {
            if (loop) {
                m_index = 0;
            } else {
                m_index = static_cast<int>(m_frames.size()) - 1;
                m_done  = true;
                return;
            }
        }
    }
}

void Animation::reset() {
    m_elapsed = 0.f;
    m_index   = 0;
    m_done    = false;
}

bool Animation::finished() const { return m_done; }

const AnimationFrame& Animation::current_frame() const {
    if (m_frames.empty())
        throw std::runtime_error("Animation has no frames");
    return m_frames[m_index];
}

} // namespace loom
