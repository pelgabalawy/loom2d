#pragma once
#include "math/rect.hpp"
#include <vector>
#include <string>

namespace loom {

struct AnimationFrame {
    Rect  source;     // pixel rect on the sprite sheet (0,0,0,0 = full texture)
    float duration;   // seconds this frame is shown
};

class Animation {
public:
    std::string name;
    bool        loop = true;

    Animation() = default;
    explicit Animation(std::string name, bool loop = true);

    void add_frame(Rect source, float duration = 0.1f);

    // Uniform strip: cols × rows frames across a sprite sheet
    void add_strip(int sheet_w, int sheet_h,
                   int frame_w, int frame_h,
                   int cols, int rows = 1,
                   float frame_duration = 0.1f);

    void update(float dt);
    void reset();

    bool                  finished()     const;
    const AnimationFrame& current_frame() const;
    int                   frame_index()  const { return m_index; }
    int                   frame_count()  const { return static_cast<int>(m_frames.size()); }

private:
    std::vector<AnimationFrame> m_frames;
    float m_elapsed = 0.f;
    int   m_index   = 0;
    bool  m_done    = false;
};

} // namespace loom
