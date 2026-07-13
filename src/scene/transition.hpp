#pragma once
#include "graphics/color.hpp"

namespace loom {

class Renderer;

// A scene transition: a short animation played over the top of everything while
// one scene is swapped for another.
//
// The manager drives it: update() every frame, then the swap happens the moment
// swap_ready() first turns true (mid-fade, so the cut is hidden), and the
// transition is discarded once done(). draw() paints over the whole screen, on
// top of the scenes AND the UI.
class Transition {
public:
    virtual ~Transition() = default;

    virtual void update(float dt) = 0;

    // True once the outgoing scene should be exchanged for the incoming one.
    virtual bool swap_ready() const = 0;
    // True once the animation has finished and the transition can be dropped.
    virtual bool done() const = 0;

    // Draw over the finished frame, in screen space: (0,0) to (screen_w, screen_h).
    virtual void draw(Renderer& renderer, int screen_w, int screen_h) = 0;
};

// Fade out to a solid colour, swap the scene at the midpoint, fade back in.
//
// Deliberately GPU-free in everything but draw(): the alpha curve is plain maths,
// so the timing is unit-tested without a window. A cross-fade (both scenes on
// screen at once) needs render-to-texture and arrives with Canvas support.
class Fade : public Transition {
public:
    explicit Fade(float duration = 0.4f, Color color = Color::black());

    void update(float dt) override;
    bool swap_ready() const override;
    bool done() const override;
    void draw(Renderer& renderer, int screen_w, int screen_h) override;

    // Opacity of the overlay right now: ramps 0 -> 1 over the first half of the
    // fade, then back 1 -> 0 over the second half.
    float alpha() const;

    float elapsed() const { return m_elapsed; }

    float duration;
    Color color;

private:
    float m_elapsed = 0.f;
};

} // namespace loom
