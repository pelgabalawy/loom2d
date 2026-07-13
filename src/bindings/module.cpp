#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <algorithm>
#include <string>

#include "platform/window.hpp"
#include "platform/paths.hpp"
#include "graphics/renderer.hpp"
#include "graphics/texture.hpp"
#include "graphics/camera.hpp"
#include "graphics/scaling.hpp"
#include "math/vec2.hpp"
#include "math/rect.hpp"
#include "core/game.hpp"
#include "core/timers.hpp"
#include "core/tween.hpp"
#include "scene/node.hpp"
#include "scene/sprite_node.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "scene/transition.hpp"
#include "scene/animation.hpp"
#include "scene/tilemap.hpp"
#include "text/font.hpp"
#include "text/text_node.hpp"
#include "ui/widget.hpp"
#include "ui/widgets.hpp"
#include "ui/ui_canvas.hpp"
#include "input/input.hpp"
#include "physics/physics.hpp"
#include "audio/audio.hpp"
#include "assets/asset_manager.hpp"

namespace py = pybind11;

// Trampoline for Python Game subclasses.
using loom::Game;

class PyGame : public loom::Game {
public:
    using loom::Game::Game;
    void on_start()              override { PYBIND11_OVERRIDE(void, Game, on_start);     }
    void on_update(float dt)     override { PYBIND11_OVERRIDE(void, Game, on_update, dt);}
    void on_draw()               override { PYBIND11_OVERRIDE(void, Game, on_draw);      }
    void on_stop()               override { PYBIND11_OVERRIDE(void, Game, on_stop);      }
    void on_resize(int w, int h) override { PYBIND11_OVERRIDE(void, Game, on_resize, w, h); }
};

// Trampoline for Python Scene subclasses. The manager owns scenes in C++ and a
// game hands them over without keeping a reference (`switch_to(MenuScene())`),
// so Scene needs the same smart_holder lifetime support as Node.
class PyScene : public loom::Scene, public py::trampoline_self_life_support {
public:
    using loom::Scene::Scene;
    void on_enter()          override { PYBIND11_OVERRIDE(void, loom::Scene, on_enter);      }
    void on_exit()           override { PYBIND11_OVERRIDE(void, loom::Scene, on_exit);       }
    void on_update(float dt) override { PYBIND11_OVERRIDE(void, loom::Scene, on_update, dt); }
    void on_draw()           override { PYBIND11_OVERRIDE(void, loom::Scene, on_draw);       }
};

// Trampoline so Python can subclass Transition and write its own.
class PyTransition : public loom::Transition, public py::trampoline_self_life_support {
public:
    using loom::Transition::Transition;
    void update(float dt)    override { PYBIND11_OVERRIDE_PURE(void, loom::Transition, update, dt); }
    bool swap_ready() const  override { PYBIND11_OVERRIDE_PURE(bool, loom::Transition, swap_ready); }
    bool done()       const  override { PYBIND11_OVERRIDE_PURE(bool, loom::Transition, done);       }
    void draw(loom::Renderer& r, int w, int h) override {
        PYBIND11_OVERRIDE_PURE(void, loom::Transition, draw, r, w, h);
    }
};

// Trampoline for Python Node subclasses.
//
// trampoline_self_life_support + py::smart_holder (below) keep the Python half
// of a subclass alive while C++ owns the object. Without it, `scene.add(Enemy())`
// — where Python retains no reference — lets the Python object die, and every
// PYBIND11_OVERRIDE silently falls back to the C++ base: the node stops updating.
class PyNode : public loom::Node, public py::trampoline_self_life_support {
public:
    using loom::Node::Node;
    void update(float dt) override {
        PYBIND11_OVERRIDE(void, loom::Node, update, dt);
    }
    void draw(loom::Renderer& r, const loom::Camera& c) override {
        PYBIND11_OVERRIDE(void, loom::Node, draw, r, c);
    }
};

// Trampoline so Python can subclass Widget and override on_click().
class PyWidget : public loom::Widget, public py::trampoline_self_life_support {
public:
    using loom::Widget::Widget;
    void on_click() override { PYBIND11_OVERRIDE(void, loom::Widget, on_click); }
};

// Walk a dotted attribute path ("x", or "tint.a") down to the object that holds
// the final attribute, so a tween can read and write it. Everything but the last
// component is followed with getattr; `leaf` comes back as that last component.
static py::object attr_owner(const py::object& root, const std::string& path,
                             std::string& leaf) {
    py::object owner = root;
    std::size_t start = 0;
    for (std::size_t dot = path.find('.'); dot != std::string::npos;
         dot = path.find('.', start)) {
        owner = owner.attr(path.substr(start, dot - start).c_str());
        start = dot + 1;
    }
    leaf = path.substr(start);
    return owner;
}

// ── Module ────────────────────────────────────────────────────────────────────

PYBIND11_MODULE(loom2d_native, m) {
    m.doc() = "loom2d native engine module";

    // ── Vec2 ──────────────────────────────────────────────────────────────────
    py::class_<loom::Vec2>(m, "Vec2")
        .def(py::init<float,float>(), py::arg("x")=0.f, py::arg("y")=0.f)
        .def_readwrite("x", &loom::Vec2::x)
        .def_readwrite("y", &loom::Vec2::y)
        .def("__add__",  [](const loom::Vec2& a, const loom::Vec2& b){ return a + b; })
        .def("__sub__",  [](const loom::Vec2& a, const loom::Vec2& b){ return a - b; })
        .def("__mul__",  [](const loom::Vec2& a, float s){ return a * s; })
        .def("__rmul__", [](const loom::Vec2& a, float s){ return a * s; })
        .def("__truediv__",[](const loom::Vec2& a, float s){ return a / s; })
        .def("__neg__",  [](const loom::Vec2& a){ return -a; })
        .def("__eq__",   &loom::Vec2::operator==)
        .def("length",     &loom::Vec2::length)
        .def("length_sq",  &loom::Vec2::length_sq)
        .def("normalized", &loom::Vec2::normalized)
        .def("dot",        &loom::Vec2::dot)
        .def("distance",   &loom::Vec2::distance)
        .def("lerp",       &loom::Vec2::lerp)
        .def("rotated",    &loom::Vec2::rotated)
        .def_static("zero",  &loom::Vec2::zero)
        .def_static("one",   &loom::Vec2::one)
        .def_static("up",    &loom::Vec2::up)
        .def_static("down",  &loom::Vec2::down)
        .def_static("left",  &loom::Vec2::left)
        .def_static("right", &loom::Vec2::right)
        .def("__repr__", &loom::Vec2::to_string);

    // ── Rect ──────────────────────────────────────────────────────────────────
    py::class_<loom::Rect>(m, "Rect")
        .def(py::init<float,float,float,float>(),
             py::arg("x")=0.f, py::arg("y")=0.f,
             py::arg("w")=0.f, py::arg("h")=0.f)
        .def_readwrite("x", &loom::Rect::x)
        .def_readwrite("y", &loom::Rect::y)
        .def_readwrite("w", &loom::Rect::w)
        .def_readwrite("h", &loom::Rect::h)
        .def("left",        &loom::Rect::left)
        .def("right",       &loom::Rect::right)
        .def("top",         &loom::Rect::top)
        .def("bottom",      &loom::Rect::bottom)
        .def("center",      &loom::Rect::center)
        .def("contains",    py::overload_cast<const loom::Vec2&>(&loom::Rect::contains, py::const_))
        .def("intersects",  &loom::Rect::intersects)
        .def("intersection",&loom::Rect::intersection)
        .def("expanded",    &loom::Rect::expanded);

    // ── Color ─────────────────────────────────────────────────────────────────
    py::class_<loom::Color>(m, "Color")
        .def(py::init<float,float,float,float>(),
             py::arg("r")=0.f, py::arg("g")=0.f,
             py::arg("b")=0.f, py::arg("a")=1.f)
        .def_readwrite("r", &loom::Color::r)
        .def_readwrite("g", &loom::Color::g)
        .def_readwrite("b", &loom::Color::b)
        .def_readwrite("a", &loom::Color::a)
        .def_static("black",       &loom::Color::black)
        .def_static("white",       &loom::Color::white)
        .def_static("red",         &loom::Color::red)
        .def_static("green",       &loom::Color::green)
        .def_static("blue",        &loom::Color::blue)
        .def_static("yellow",      &loom::Color::yellow)
        .def_static("transparent", &loom::Color::transparent)
        .def_static("cornflower",  &loom::Color::cornflower)
        .def("__repr__", [](const loom::Color& c){
            return "Color(" + std::to_string(c.r) + ", " + std::to_string(c.g)
                 + ", " + std::to_string(c.b) + ", " + std::to_string(c.a) + ")";
        });

    // ── AnimationFrame ────────────────────────────────────────────────────────
    py::class_<loom::AnimationFrame>(m, "AnimationFrame")
        .def(py::init<>())
        .def_readwrite("source",   &loom::AnimationFrame::source)
        .def_readwrite("duration", &loom::AnimationFrame::duration);

    // ── Animation ─────────────────────────────────────────────────────────────
    py::class_<loom::Animation>(m, "Animation")
        .def(py::init<std::string, bool>(),
             py::arg("name"), py::arg("loop")=true)
        .def_readwrite("name", &loom::Animation::name)
        .def_readwrite("loop", &loom::Animation::loop)
        .def("add_frame",  &loom::Animation::add_frame,
             py::arg("source"), py::arg("duration")=0.1f)
        .def("add_strip",  &loom::Animation::add_strip,
             py::arg("sheet_w"), py::arg("sheet_h"),
             py::arg("frame_w"), py::arg("frame_h"),
             py::arg("cols"), py::arg("rows")=1,
             py::arg("frame_duration")=0.1f)
        .def("update",        &loom::Animation::update)
        .def("reset",         &loom::Animation::reset)
        .def("finished",      &loom::Animation::finished)
        .def("frame_index",   &loom::Animation::frame_index)
        .def("frame_count",   &loom::Animation::frame_count);

    // ── Camera ────────────────────────────────────────────────────────────────
    py::class_<loom::Camera>(m, "Camera")
        .def(py::init<>())
        .def_property("position",   &loom::Camera::position,
                                    &loom::Camera::set_position)
        .def_property("zoom",       &loom::Camera::zoom,
                                    &loom::Camera::set_zoom)
        .def_property("rotation",   &loom::Camera::rotation,
                                    &loom::Camera::set_rotation)
        .def("move",                &loom::Camera::move)
        .def("set_viewport",        &loom::Camera::set_viewport,
             py::arg("width"), py::arg("height"))
        .def("world_to_screen",     &loom::Camera::world_to_screen)
        .def("screen_to_world",     &loom::Camera::screen_to_world)
        .def("visible_rect",        &loom::Camera::visible_rect);

    // ── ScaleMode ─────────────────────────────────────────────────────────────
    py::enum_<loom::ScaleMode>(m, "ScaleMode")
        .value("Fit",          loom::ScaleMode::Fit)
        .value("Stretch",      loom::ScaleMode::Stretch)
        .value("Expand",       loom::ScaleMode::Expand)
        .value("PixelPerfect", loom::ScaleMode::PixelPerfect)
        .export_values();

    // ── Texture ───────────────────────────────────────────────────────────────
    py::class_<loom::Texture, std::shared_ptr<loom::Texture>>(m, "Texture")
        .def_property_readonly("width",  &loom::Texture::width)
        .def_property_readonly("height", &loom::Texture::height)
        .def_property_readonly("path",   &loom::Texture::path);

    // ── Node ──────────────────────────────────────────────────────────────────
    py::classh<loom::Node, PyNode>(m, "Node")
        .def(py::init<>())
        .def(py::init<std::string>())
        .def_readwrite("name",    &loom::Node::name)
        .def_readwrite("visible", &loom::Node::visible)
        .def_property("position",
            &loom::Node::position,
            py::overload_cast<loom::Vec2>(&loom::Node::set_position))
        .def_property("x", &loom::Node::x, &loom::Node::set_x)
        .def_property("y", &loom::Node::y, &loom::Node::set_y)
        .def_property("rotation",
            &loom::Node::rotation, &loom::Node::set_rotation)
        .def_property("scale",
            &loom::Node::scale,
            py::overload_cast<loom::Vec2>(&loom::Node::set_scale))
        .def("world_position", &loom::Node::world_position)
        .def("world_rotation", &loom::Node::world_rotation)
        .def("world_scale",    &loom::Node::world_scale)
        .def("add_child",      &loom::Node::add_child)
        .def("remove_from_parent", &loom::Node::remove_from_parent)
        .def("update",         &loom::Node::update)
        .def("children", [](const loom::Node& n) {
            // Return a copy so Python list lifetime is independent of the node
            return n.children();
        });

    // ── SpriteNode ────────────────────────────────────────────────────────────
    py::classh<loom::SpriteNode, loom::Node>(m, "SpriteNode")
        .def(py::init<std::shared_ptr<loom::Texture>>())
        .def_readwrite("tint",   &loom::SpriteNode::tint)
        .def_readwrite("origin", &loom::SpriteNode::origin)
        .def_readwrite("flip_x", &loom::SpriteNode::flip_x)
        .def_readwrite("flip_y", &loom::SpriteNode::flip_y)
        .def("set_texture", &loom::SpriteNode::set_texture)
        .def("set_source",  &loom::SpriteNode::set_source)
        .def("add_animation", &loom::SpriteNode::add_animation)
        .def("play",          &loom::SpriteNode::play)
        .def("stop",          &loom::SpriteNode::stop);

    // ── Tileset ───────────────────────────────────────────────────────────────
    py::class_<loom::Tileset, std::shared_ptr<loom::Tileset>>(m, "Tileset")
        .def(py::init<std::shared_ptr<loom::Texture>, int, int, int>(),
             py::arg("texture"), py::arg("tile_w"), py::arg("tile_h"),
             py::arg("first_gid")=1)
        .def_readwrite("texture",   &loom::Tileset::texture)
        .def_readwrite("tile_w",    &loom::Tileset::tile_w)
        .def_readwrite("tile_h",    &loom::Tileset::tile_h)
        .def_readwrite("first_gid", &loom::Tileset::first_gid)
        .def_readwrite("columns",   &loom::Tileset::columns)
        .def_readwrite("margin",    &loom::Tileset::margin)
        .def_readwrite("spacing",   &loom::Tileset::spacing)
        .def_property_readonly("tile_count", &loom::Tileset::tile_count)
        .def_property_readonly("last_gid",   &loom::Tileset::last_gid)
        .def("contains",   &loom::Tileset::contains)
        .def("source_for", &loom::Tileset::source_for);

    // ── TileLayer ─────────────────────────────────────────────────────────────
    py::class_<loom::TileLayer, std::shared_ptr<loom::TileLayer>>(m, "TileLayer")
        .def(py::init<std::string, int, int>(),
             py::arg("name"), py::arg("width"), py::arg("height"))
        .def_readwrite("name",    &loom::TileLayer::name)
        .def_readwrite("visible", &loom::TileLayer::visible)
        .def_readwrite("opacity", &loom::TileLayer::opacity)
        .def_property_readonly("width",  [](const loom::TileLayer& l){ return l.width;  })
        .def_property_readonly("height", [](const loom::TileLayer& l){ return l.height; })
        .def("at",   &loom::TileLayer::at)
        .def("set",  &loom::TileLayer::set)
        .def("fill", &loom::TileLayer::fill);

    // ── Tilemap ───────────────────────────────────────────────────────────────
    py::classh<loom::Tilemap, loom::Node>(m, "Tilemap")
        .def(py::init<int, int, int, int>(),
             py::arg("tile_w"), py::arg("tile_h"),
             py::arg("width"), py::arg("height"))
        .def_readwrite("tile_w", &loom::Tilemap::tile_w)
        .def_readwrite("tile_h", &loom::Tilemap::tile_h)
        .def_property_readonly("width",  [](const loom::Tilemap& t){ return t.width;  })
        .def_property_readonly("height", [](const loom::Tilemap& t){ return t.height; })
        .def("add_tileset",
             py::overload_cast<std::shared_ptr<loom::Texture>, int, int, int>(
                 &loom::Tilemap::add_tileset),
             py::arg("texture"), py::arg("tile_w"), py::arg("tile_h"),
             py::arg("first_gid")=1)
        .def("tileset_for_gid", &loom::Tilemap::tileset_for_gid,
             py::return_value_policy::reference_internal)
        .def("add_layer",
             py::overload_cast<const std::string&>(&loom::Tilemap::add_layer),
             py::arg("name"))
        .def("layers",        &loom::Tilemap::layers)
        .def("layer",         &loom::Tilemap::layer, py::arg("index"))
        .def("layer_by_name", &loom::Tilemap::layer_by_name, py::arg("name"))
        .def("tile_to_world", &loom::Tilemap::tile_to_world,
             py::arg("tx"), py::arg("ty"))
        .def("world_to_tile", &loom::Tilemap::world_to_tile, py::arg("world"))
        .def("set_solid",     &loom::Tilemap::set_solid,
             py::arg("x"), py::arg("y"), py::arg("solid"))
        .def("is_solid",      &loom::Tilemap::is_solid, py::arg("x"), py::arg("y"))
        .def("set_collision_from_layer", &loom::Tilemap::set_collision_from_layer,
             py::arg("layer_index"))
        .def("clear_collision",   &loom::Tilemap::clear_collision)
        .def("rect_overlaps_solid", &loom::Tilemap::rect_overlaps_solid,
             py::arg("world_rect"))
        .def_property_readonly("tiles_drawn", &loom::Tilemap::tiles_drawn)
        .def_static("load", &loom::Tilemap::load, py::arg("path"));

    // ── TextAlign ─────────────────────────────────────────────────────────────
    py::enum_<loom::TextAlign>(m, "TextAlign")
        .value("Left",   loom::TextAlign::Left)
        .value("Center", loom::TextAlign::Center)
        .value("Right",  loom::TextAlign::Right)
        .export_values();

    // ── Font ──────────────────────────────────────────────────────────────────
    py::class_<loom::Font, std::shared_ptr<loom::Font>>(m, "Font")
        .def_static("load", &loom::Font::load,
                    py::arg("path"), py::arg("pixel_height"),
                    "Load a TTF/OTF and bake an ASCII atlas at the given pixel "
                    "height. Requires a running Game/Renderer.")
        .def_property_readonly("pixel_height", &loom::Font::pixel_height)
        .def_property_readonly("line_height",  &loom::Font::line_height)
        .def_property_readonly("ascent",       &loom::Font::ascent)
        .def("measure", &loom::Font::measure,
             py::arg("text"), py::arg("max_width") = 0.f,
             "Block (width, height) in pixels for the given text.")
        .def_static("wrap_lines", &loom::Font::wrap_lines,
                    py::arg("text"), py::arg("advances"), py::arg("max_width"),
                    "Greedy word-wrap helper: [start,end) byte ranges per line.");

    // ── TextNode ──────────────────────────────────────────────────────────────
    py::classh<loom::TextNode, loom::Node>(m, "TextNode")
        .def(py::init<std::shared_ptr<loom::Font>, std::string>(),
             py::arg("font"), py::arg("text") = "")
        .def_readwrite("color",  &loom::TextNode::color)
        .def_readwrite("origin", &loom::TextNode::origin)
        .def_property("text", &loom::TextNode::text, &loom::TextNode::set_text)
        .def_property("align", &loom::TextNode::align, &loom::TextNode::set_align)
        .def_property("max_width",
                      &loom::TextNode::max_width, &loom::TextNode::set_max_width)
        .def("set_font", &loom::TextNode::set_font)
        .def_property_readonly("font", &loom::TextNode::font)
        .def_property_readonly("size", &loom::TextNode::size);

    // ── Widget (UI base) ────────────────────────────────────────────────────────
    py::classh<loom::Widget, PyWidget>(m, "Widget")
        .def(py::init<>())
        .def(py::init<std::string>())
        .def_readwrite("name",      &loom::Widget::name)
        .def_readwrite("visible",   &loom::Widget::visible)
        .def_readwrite("enabled",   &loom::Widget::enabled)
        .def_readwrite("focusable", &loom::Widget::focusable)
        .def_readwrite("anchor",    &loom::Widget::anchor)
        .def_readwrite("pivot",     &loom::Widget::pivot)
        .def_readwrite("offset",    &loom::Widget::offset)
        .def_readwrite("size",      &loom::Widget::size)
        .def_property_readonly("hovered", [](loom::Widget& w){ return w.hovered; })
        .def_property_readonly("pressed", [](loom::Widget& w){ return w.pressed; })
        .def_property_readonly("focused", [](loom::Widget& w){ return w.focused; })
        .def("add_child",          &loom::Widget::add_child)
        .def("remove_from_parent", &loom::Widget::remove_from_parent)
        .def("clear_children",     &loom::Widget::clear_children)
        .def("rect",               &loom::Widget::rect)
        .def("resolve_layout",     &loom::Widget::resolve_layout, py::arg("parent_rect"))
        .def("children", [](const loom::Widget& w){ return w.children(); })
        .def("on_click", &loom::Widget::on_click);

    // ── Panel ─────────────────────────────────────────────────────────────────
    py::classh<loom::Panel, loom::Widget>(m, "Panel")
        .def(py::init<>())
        .def(py::init<loom::Color>(), py::arg("background"))
        .def_readwrite("background",   &loom::Panel::background)
        .def_readwrite("border_color", &loom::Panel::border_color)
        .def_readwrite("border_width", &loom::Panel::border_width);

    // ── Label ─────────────────────────────────────────────────────────────────
    py::classh<loom::Label, loom::Widget>(m, "Label")
        .def(py::init<>())
        .def(py::init<std::shared_ptr<loom::Font>, std::string>(),
             py::arg("font"), py::arg("text") = "")
        .def_readwrite("color",   &loom::Label::color)
        .def_readwrite("align",   &loom::Label::align)
        .def_readwrite("vcenter", &loom::Label::vcenter)
        .def_property("text", &loom::Label::text, &loom::Label::set_text)
        .def("set_font", &loom::Label::set_font)
        .def_property_readonly("font", &loom::Label::font);

    // ── Button ────────────────────────────────────────────────────────────────
    py::classh<loom::Button, loom::Widget>(m, "Button")
        .def(py::init<>())
        .def(py::init<std::shared_ptr<loom::Font>, std::string>(),
             py::arg("font"), py::arg("caption") = "")
        .def_readwrite("caption",     &loom::Button::caption)
        .def_readwrite("text_color",  &loom::Button::text_color)
        .def_readwrite("bg",          &loom::Button::bg)
        .def_readwrite("bg_hover",    &loom::Button::bg_hover)
        .def_readwrite("bg_pressed",  &loom::Button::bg_pressed)
        .def_readwrite("bg_disabled", &loom::Button::bg_disabled)
        .def_readwrite("on_clicked",  &loom::Button::on_clicked)
        .def("set_font", &loom::Button::set_font)
        .def_property_readonly("font", &loom::Button::font)
        .def("current_background", &loom::Button::current_background);

    // ── Image ─────────────────────────────────────────────────────────────────
    py::classh<loom::Image, loom::Widget>(m, "Image")
        .def(py::init<>())
        .def(py::init<std::shared_ptr<loom::Texture>>(), py::arg("texture"))
        .def_readwrite("tint",   &loom::Image::tint)
        .def_readwrite("source", &loom::Image::source)
        .def("set_texture", &loom::Image::set_texture)
        .def_property_readonly("texture", &loom::Image::texture);

    // ── Grid ──────────────────────────────────────────────────────────────────
    py::classh<loom::Grid, loom::Widget>(m, "Grid")
        .def(py::init<>())
        .def(py::init<int, loom::Vec2>(),
             py::arg("columns"), py::arg("spacing") = loom::Vec2(0.f, 0.f))
        .def_readwrite("columns", &loom::Grid::columns)
        .def_readwrite("spacing", &loom::Grid::spacing);

    // ── UICanvas ──────────────────────────────────────────────────────────────
    py::class_<loom::UICanvas>(m, "UICanvas")
        .def(py::init<>())
        .def("add",        &loom::UICanvas::add)
        .def("remove",     &loom::UICanvas::remove)
        .def("clear",      &loom::UICanvas::clear)
        .def("root",       &loom::UICanvas::root_ptr,
             py::return_value_policy::reference_internal)
        .def("set_screen", &loom::UICanvas::set_screen,
             py::arg("width"), py::arg("height"))
        .def_property_readonly("screen_width",  &loom::UICanvas::screen_width)
        .def_property_readonly("screen_height", &loom::UICanvas::screen_height)
        .def("layout",       &loom::UICanvas::layout)
        .def("update_input", &loom::UICanvas::update_input,
             py::arg("pointer"), py::arg("pressed"),
             py::arg("down"), py::arg("released"))
        .def("focus",        &loom::UICanvas::focus, py::arg("widget"))
        .def("clear_focus",  &loom::UICanvas::clear_focus)
        .def("focus_next",   &loom::UICanvas::focus_next)
        .def_readwrite("camera", &loom::UICanvas::camera);

    // ── Scene ─────────────────────────────────────────────────────────────────
    py::classh<loom::Scene, PyScene>(m, "Scene")
        .def(py::init<>())
        .def_readwrite("camera", &loom::Scene::camera)
        .def_property_readonly("ui", [](loom::Scene& s) -> loom::UICanvas& { return s.ui; },
                               py::return_value_policy::reference_internal,
                               "This scene's own screen-space UI layer.")
        .def_property_readonly("game", [](loom::Scene& s) { return s.game; },
                               py::return_value_policy::reference,
                               "The Game this scene belongs to; None until it is "
                               "handed to a SceneManager.")
        .def("add",    &loom::Scene::add)
        .def("remove", &loom::Scene::remove)
        .def("clear",  &loom::Scene::clear)
        .def("update", &loom::Scene::update)
        .def("root",   &loom::Scene::root_ptr,
             py::return_value_policy::reference_internal)
        // Lifecycle hooks — override these in a subclass.
        .def("on_enter",  &loom::Scene::on_enter)
        .def("on_exit",   &loom::Scene::on_exit)
        .def("on_update", &loom::Scene::on_update, py::arg("dt"))
        .def("on_draw",   &loom::Scene::on_draw);

    // ── Transitions ───────────────────────────────────────────────────────────
    py::classh<loom::Transition, PyTransition>(m, "Transition")
        .def(py::init<>())
        .def("update",     &loom::Transition::update, py::arg("dt"))
        .def("swap_ready", &loom::Transition::swap_ready)
        .def("done",       &loom::Transition::done);

    py::classh<loom::Fade, loom::Transition>(m, "Fade")
        .def(py::init<float, loom::Color>(),
             py::arg("duration") = 0.4f,
             py::arg("color")    = loom::Color::black(),
             "Fade out to a colour, swap the scene at the midpoint, fade back in.")
        .def_readwrite("duration", &loom::Fade::duration)
        .def_readwrite("color",    &loom::Fade::color)
        .def_property_readonly("alpha", &loom::Fade::alpha,
                               "Overlay opacity right now (0 -> 1 -> 0).")
        .def_property_readonly("elapsed", &loom::Fade::elapsed);

    // ── SceneManager ──────────────────────────────────────────────────────────
    py::class_<loom::SceneManager>(m, "SceneManager")
        .def("switch_to", &loom::SceneManager::switch_to,
             py::arg("scene"), py::arg("transition") = nullptr,
             "Replace the active scene.")
        .def("push", &loom::SceneManager::push,
             py::arg("scene"), py::arg("transition") = nullptr,
             "Lay a scene over the active one (pause menu). The scene below stays "
             "alive and keeps drawing, but stops updating.")
        .def("pop", &loom::SceneManager::pop,
             py::arg("transition") = nullptr,
             "Drop the top scene, revealing the one below. No-op if it is the last.")
        .def("update", &loom::SceneManager::update, py::arg("dt"))
        .def_property_readonly("current",       &loom::SceneManager::current)
        .def_property_readonly("depth",         &loom::SceneManager::depth)
        .def_property_readonly("transitioning", &loom::SceneManager::transitioning)
        .def_property_readonly("stack",         &loom::SceneManager::stack);

    // ── Timers ────────────────────────────────────────────────────────────────
    py::class_<loom::Timers>(m, "Timers")
        .def(py::init<>())
        .def("after", &loom::Timers::after,
             py::arg("delay"), py::arg("fn"),
             "Call fn once, `delay` seconds from now. Returns a handle.")
        .def("every", &loom::Timers::every,
             py::arg("interval"), py::arg("fn"), py::arg("times") = 0,
             "Call fn every `interval` seconds. times=0 repeats for ever; "
             "otherwise the timer retires after that many calls.")
        .def("cancel", &loom::Timers::cancel, py::arg("handle"),
             "Stop a timer. False if the handle was unknown or already done.")
        .def("clear",  &loom::Timers::clear, "Stop every timer.")
        .def("active", &loom::Timers::active, py::arg("handle"))
        .def("update", &loom::Timers::update, py::arg("dt"),
             "Advance every timer. run() drives this; exposed for headless tests.")
        .def_property_readonly("count", &loom::Timers::count);

    // ── Ease ──────────────────────────────────────────────────────────────────
    // Not export_values(): `Linear` and friends are far too generic to spill
    // into the module namespace. Spell it loom.Ease.OutQuad.
    py::enum_<loom::Ease>(m, "Ease")
        .value("Linear",       loom::Ease::Linear)
        .value("InQuad",       loom::Ease::InQuad)
        .value("OutQuad",      loom::Ease::OutQuad)
        .value("InOutQuad",    loom::Ease::InOutQuad)
        .value("InCubic",      loom::Ease::InCubic)
        .value("OutCubic",     loom::Ease::OutCubic)
        .value("InOutCubic",   loom::Ease::InOutCubic)
        .value("InQuart",      loom::Ease::InQuart)
        .value("OutQuart",     loom::Ease::OutQuart)
        .value("InOutQuart",   loom::Ease::InOutQuart)
        .value("InSine",       loom::Ease::InSine)
        .value("OutSine",      loom::Ease::OutSine)
        .value("InOutSine",    loom::Ease::InOutSine)
        .value("InExpo",       loom::Ease::InExpo)
        .value("OutExpo",      loom::Ease::OutExpo)
        .value("InOutExpo",    loom::Ease::InOutExpo)
        .value("InCirc",       loom::Ease::InCirc)
        .value("OutCirc",      loom::Ease::OutCirc)
        .value("InOutCirc",    loom::Ease::InOutCirc)
        .value("InBack",       loom::Ease::InBack)
        .value("OutBack",      loom::Ease::OutBack)
        .value("InOutBack",    loom::Ease::InOutBack)
        .value("InElastic",    loom::Ease::InElastic)
        .value("OutElastic",   loom::Ease::OutElastic)
        .value("InOutElastic", loom::Ease::InOutElastic)
        .value("InBounce",     loom::Ease::InBounce)
        .value("OutBounce",    loom::Ease::OutBounce)
        .value("InOutBounce",  loom::Ease::InOutBounce);

    m.def("ease", &loom::ease, py::arg("easing"), py::arg("t"),
          "Shape a normalised time (0..1) with an easing curve.");

    // ── Tween ─────────────────────────────────────────────────────────────────
    py::class_<loom::Tween, std::shared_ptr<loom::Tween>>(m, "Tween")
        .def(py::init<float, float, float, loom::Ease>(),
             py::arg("from_"), py::arg("to"), py::arg("duration"),
             py::arg("easing") = loom::Ease::Linear)
        .def_readwrite("from_",    &loom::Tween::from)
        .def_readwrite("to",       &loom::Tween::to)
        .def_readwrite("duration", &loom::Tween::duration)
        .def_readwrite("easing",   &loom::Tween::easing)
        .def_readwrite("delay",    &loom::Tween::delay)
        .def_readwrite("on_update",   &loom::Tween::on_update)
        .def_readwrite("on_complete", &loom::Tween::on_complete)
        .def("update", &loom::Tween::update, py::arg("dt"))
        .def("cancel", &loom::Tween::cancel,
             "Stop early. on_complete does NOT fire — it never arrived.")
        .def_property_readonly("done",      &loom::Tween::done)
        .def_property_readonly("cancelled", &loom::Tween::cancelled)
        .def_property_readonly("value",     &loom::Tween::value)
        .def_property_readonly("progress",  &loom::Tween::progress)
        .def_property_readonly("elapsed",   &loom::Tween::elapsed);

    // ── TweenManager ──────────────────────────────────────────────────────────
    py::class_<loom::TweenManager>(m, "TweenManager")
        .def(py::init<>())
        // The sugar: name an attribute and a destination, and it animates.
        // The tween holds a reference to the target, so a sprite being tweened
        // stays alive at least as long as the animation.
        .def("to",
             [](loom::TweenManager& tm, py::object target, const std::string& prop,
                float to, float duration, loom::Ease easing, float delay,
                std::function<void()> on_complete) {
                 std::string leaf;
                 py::object  owner = attr_owner(target, prop, leaf);
                 const float from  = py::getattr(owner, leaf.c_str()).cast<float>();

                 auto tween = std::make_shared<loom::Tween>(from, to, duration, easing);
                 tween->delay       = delay;
                 tween->on_complete = std::move(on_complete);
                 // Re-resolve the path each frame rather than caching the owner:
                 // an attribute reached through a by-value copy (a Color, say)
                 // must be read back and written whole to actually take effect.
                 tween->on_update = [target, prop](float v) {
                     std::string leaf;
                     py::object  owner = attr_owner(target, prop, leaf);
                     py::setattr(owner, leaf.c_str(), py::float_(v));
                 };
                 return tm.add(std::move(tween));
             },
             py::arg("target"), py::arg("prop"), py::arg("to"), py::arg("duration"),
             py::arg("easing")      = loom::Ease::Linear,
             py::arg("delay")       = 0.f,
             py::arg("on_complete") = nullptr,
             "Animate a float attribute of `target` to `to` over `duration` "
             "seconds. `prop` may be dotted ('x', 'tint.a'). Starts from whatever "
             "the attribute reads right now. Returns the Tween.")
        .def("add",    &loom::TweenManager::add, py::arg("tween"),
             "Run a Tween you built yourself.")
        .def("cancel", &loom::TweenManager::cancel, py::arg("tween"))
        .def("clear",  &loom::TweenManager::clear, "Cancel everything in flight.")
        .def("update", &loom::TweenManager::update, py::arg("dt"),
             "Advance every tween. run() drives this; exposed for headless tests.")
        .def_property_readonly("count", &loom::TweenManager::count);

    // ── save_dir() ────────────────────────────────────────────────────────────
    m.def("save_dir", &loom::save_dir, py::arg("org"), py::arg("app"),
          "The per-user directory this game may write save files to, created if "
          "needed. The OS decides where that is; loom2d.SaveFile builds on it.");

    // ── Key enum ──────────────────────────────────────────────────────────────
    py::enum_<loom::Key>(m, "Key")
        .value("A", loom::Key::A) .value("B", loom::Key::B)
        .value("C", loom::Key::C) .value("D", loom::Key::D)
        .value("E", loom::Key::E) .value("F", loom::Key::F)
        .value("G", loom::Key::G) .value("H", loom::Key::H)
        .value("I", loom::Key::I) .value("J", loom::Key::J)
        .value("K", loom::Key::K) .value("L", loom::Key::L)
        .value("M", loom::Key::M) .value("N", loom::Key::N)
        .value("O", loom::Key::O) .value("P", loom::Key::P)
        .value("Q", loom::Key::Q) .value("R", loom::Key::R)
        .value("S", loom::Key::S) .value("T", loom::Key::T)
        .value("U", loom::Key::U) .value("V", loom::Key::V)
        .value("W", loom::Key::W) .value("X", loom::Key::X)
        .value("Y", loom::Key::Y) .value("Z", loom::Key::Z)
        .value("Up",     loom::Key::Up)    .value("Down",  loom::Key::Down)
        .value("Left",   loom::Key::Left)  .value("Right", loom::Key::Right)
        .value("Space",  loom::Key::Space) .value("Enter", loom::Key::Enter)
        .value("Escape", loom::Key::Escape).value("Tab",   loom::Key::Tab)
        .value("Shift",  loom::Key::Shift) .value("Ctrl",  loom::Key::Ctrl)
        .value("F1",     loom::Key::F1)    .value("F5",    loom::Key::F5)
        .value("F12",    loom::Key::F12)
        .value("Backspace", loom::Key::Backspace) .value("Delete", loom::Key::Delete)
        .value("Home",      loom::Key::Home)      .value("End",    loom::Key::End)
        .export_values();

    py::enum_<loom::MouseButton>(m, "MouseButton")
        .value("Left",   loom::MouseButton::Left)
        .value("Middle", loom::MouseButton::Middle)
        .value("Right",  loom::MouseButton::Right)
        .export_values();

    // ── GamepadButton / GamepadAxis ───────────────────────────────────────────
    py::enum_<loom::GamepadButton>(m, "GamepadButton")
        .value("South",         loom::GamepadButton::South)
        .value("East",          loom::GamepadButton::East)
        .value("West",          loom::GamepadButton::West)
        .value("North",         loom::GamepadButton::North)
        .value("Back",          loom::GamepadButton::Back)
        .value("Guide",         loom::GamepadButton::Guide)
        .value("Start",         loom::GamepadButton::Start)
        .value("LeftStick",     loom::GamepadButton::LeftStick)
        .value("RightStick",    loom::GamepadButton::RightStick)
        .value("LeftShoulder",  loom::GamepadButton::LeftShoulder)
        .value("RightShoulder", loom::GamepadButton::RightShoulder)
        .value("DpadUp",        loom::GamepadButton::DpadUp)
        .value("DpadDown",      loom::GamepadButton::DpadDown)
        .value("DpadLeft",      loom::GamepadButton::DpadLeft)
        .value("DpadRight",     loom::GamepadButton::DpadRight)
        .export_values();

    py::enum_<loom::GamepadAxis>(m, "GamepadAxis")
        .value("LeftX",        loom::GamepadAxis::LeftX)
        .value("LeftY",        loom::GamepadAxis::LeftY)
        .value("RightX",       loom::GamepadAxis::RightX)
        .value("RightY",       loom::GamepadAxis::RightY)
        .value("TriggerLeft",  loom::GamepadAxis::TriggerLeft)
        .value("TriggerRight", loom::GamepadAxis::TriggerRight)
        .export_values();

    // ── TouchPoint ────────────────────────────────────────────────────────────
    py::class_<loom::TouchPoint>(m, "TouchPoint")
        .def_readonly("id",       &loom::TouchPoint::id)
        .def_readonly("position", &loom::TouchPoint::position)
        .def_readonly("pressure", &loom::TouchPoint::pressure)
        .def("__repr__", [](const loom::TouchPoint& t){
            return "TouchPoint(id=" + std::to_string(t.id) + ", pos=("
                 + std::to_string(t.position.x) + ", "
                 + std::to_string(t.position.y) + "))";
        });

    // ── Input (static class) ─────────────────────────────────────────────────
    py::class_<loom::Input>(m, "Input")
        // Frame lifecycle (run() drives this; exposed for headless tests)
        .def_static("new_frame", &loom::Input::new_frame)
        // Keyboard
        .def_static("key_down",     &loom::Input::key_down)
        .def_static("key_pressed",  &loom::Input::key_pressed)
        .def_static("key_released", &loom::Input::key_released)
        // Mouse
        .def_static("mouse_position", &loom::Input::mouse_position)
        .def_static("mouse_down",     &loom::Input::mouse_down)
        .def_static("mouse_pressed",  &loom::Input::mouse_pressed)
        .def_static("mouse_released", &loom::Input::mouse_released)
        .def_static("mouse_wheel",    &loom::Input::mouse_wheel)
        // Gamepad
        .def_static("gamepad_count",     &loom::Input::gamepad_count)
        .def_static("gamepad_connected", &loom::Input::gamepad_connected,
                    py::arg("index")=0)
        .def_static("gamepad_down",      &loom::Input::gamepad_down,
                    py::arg("button"), py::arg("index")=0)
        .def_static("gamepad_pressed",   &loom::Input::gamepad_pressed,
                    py::arg("button"), py::arg("index")=0)
        .def_static("gamepad_released",  &loom::Input::gamepad_released,
                    py::arg("button"), py::arg("index")=0)
        .def_static("gamepad_axis",      &loom::Input::gamepad_axis,
                    py::arg("axis"), py::arg("index")=0)
        .def_static("set_gamepad_deadzone", &loom::Input::set_gamepad_deadzone)
        .def_static("gamepad_deadzone",     &loom::Input::gamepad_deadzone)
        .def_static("gamepad_rumble",    &loom::Input::gamepad_rumble,
                    py::arg("low"), py::arg("high"), py::arg("duration_ms"),
                    py::arg("index")=0)
        // Touch
        .def_static("touch_count",   &loom::Input::touch_count)
        .def_static("touches",       &loom::Input::touches)
        .def_static("touches_began", &loom::Input::touches_began)
        .def_static("touches_ended", &loom::Input::touches_ended)
        // Text input
        .def_static("start_text_input",  &loom::Input::start_text_input)
        .def_static("stop_text_input",   &loom::Input::stop_text_input)
        .def_static("text_input_active", &loom::Input::text_input_active)
        .def_static("text_input",        &loom::Input::text_input)
        // Test injection
        .def_static("inject_key_down",  &loom::Input::inject_key_down)
        .def_static("inject_key_up",    &loom::Input::inject_key_up)
        .def_static("inject_mouse_wheel", &loom::Input::inject_mouse_wheel)
        .def_static("inject_text_input",  &loom::Input::inject_text_input)
        .def_static("inject_gamepad_add",    &loom::Input::inject_gamepad_add)
        .def_static("inject_gamepad_remove", &loom::Input::inject_gamepad_remove)
        .def_static("inject_gamepad_button", &loom::Input::inject_gamepad_button,
                    py::arg("index"), py::arg("button"), py::arg("down"))
        .def_static("inject_gamepad_axis",   &loom::Input::inject_gamepad_axis,
                    py::arg("index"), py::arg("axis"), py::arg("value"))
        .def_static("inject_touch",          &loom::Input::inject_touch,
                    py::arg("id"), py::arg("logical_pos"), py::arg("pressure")=1.f)
        .def_static("inject_touch_release",  &loom::Input::inject_touch_release);

    // ── BodyType ──────────────────────────────────────────────────────────────
    py::enum_<loom::BodyType>(m, "BodyType")
        .value("Static",    loom::BodyType::Static)
        .value("Kinematic", loom::BodyType::Kinematic)
        .value("Dynamic",   loom::BodyType::Dynamic)
        .export_values();

    // ── Physics events ──────────────────────────────────────────────────────────
    py::class_<loom::ContactPair>(m, "ContactPair")
        .def_readonly("body_a", &loom::ContactPair::body_a,
                      py::return_value_policy::reference)
        .def_readonly("body_b", &loom::ContactPair::body_b,
                      py::return_value_policy::reference);

    py::class_<loom::SensorPair>(m, "SensorPair")
        .def_readonly("sensor",  &loom::SensorPair::sensor,
                      py::return_value_policy::reference)
        .def_readonly("visitor", &loom::SensorPair::visitor,
                      py::return_value_policy::reference);

    py::class_<loom::RaycastHit>(m, "RaycastHit")
        .def_readonly("hit",      &loom::RaycastHit::hit)
        .def_readonly("body",     &loom::RaycastHit::body,
                      py::return_value_policy::reference)
        .def_readonly("point",    &loom::RaycastHit::point)
        .def_readonly("normal",   &loom::RaycastHit::normal)
        .def_readonly("fraction", &loom::RaycastHit::fraction)
        .def("__bool__", [](const loom::RaycastHit& h){ return h.hit; });

    // ── PhysicsBody ───────────────────────────────────────────────────────────
    py::class_<loom::PhysicsBody>(m, "PhysicsBody")
        .def("add_box",    &loom::PhysicsBody::add_box,
             py::arg("half_w"), py::arg("half_h"),
             py::arg("density")=1.f, py::arg("friction")=0.3f,
             py::arg("restitution")=0.f, py::arg("is_sensor")=false)
        .def("add_circle", &loom::PhysicsBody::add_circle,
             py::arg("radius"),
             py::arg("density")=1.f, py::arg("friction")=0.3f,
             py::arg("restitution")=0.f, py::arg("is_sensor")=false)
        .def_property("tag", &loom::PhysicsBody::tag, &loom::PhysicsBody::set_tag)
        .def_property_readonly("position",        &loom::PhysicsBody::position)
        .def_property_readonly("rotation",        &loom::PhysicsBody::rotation)
        .def_property_readonly("linear_velocity", &loom::PhysicsBody::linear_velocity)
        .def("set_position",        &loom::PhysicsBody::set_position)
        .def("set_linear_velocity", &loom::PhysicsBody::set_linear_velocity)
        .def("apply_impulse",       &loom::PhysicsBody::apply_impulse)
        .def("apply_force",         &loom::PhysicsBody::apply_force);

    // ── PhysicsWorld ──────────────────────────────────────────────────────────
    py::class_<loom::PhysicsWorld>(m, "PhysicsWorld")
        .def(py::init<float, float>(),
             py::arg("gravity_x")=0.f, py::arg("gravity_y")=980.f)
        .def("step",         &loom::PhysicsWorld::step,
             py::arg("dt"), py::arg("sub_steps")=4)
        .def("create_body",  &loom::PhysicsWorld::create_body,
             py::arg("type"), py::arg("position"),
             py::return_value_policy::reference)
        .def("destroy_body", &loom::PhysicsWorld::destroy_body)
        .def("raycast",      &loom::PhysicsWorld::raycast,
             py::arg("p1"), py::arg("p2"))
        .def_property_readonly("contact_begins", &loom::PhysicsWorld::contact_begins)
        .def_property_readonly("contact_ends",   &loom::PhysicsWorld::contact_ends)
        .def_property_readonly("sensor_begins",  &loom::PhysicsWorld::sensor_begins)
        .def_property_readonly("sensor_ends",    &loom::PhysicsWorld::sensor_ends)
        .def_readwrite("on_contact_begin", &loom::PhysicsWorld::on_contact_begin)
        .def_readwrite("on_contact_end",   &loom::PhysicsWorld::on_contact_end)
        .def_readwrite("on_sensor_begin",  &loom::PhysicsWorld::on_sensor_begin)
        .def_readwrite("on_sensor_end",    &loom::PhysicsWorld::on_sensor_end);

    // ── SoundHandle ───────────────────────────────────────────────────────────
    // Returned by play_sound(). Optional to keep: the engine holds the sound
    // alive until it finishes on its own.
    py::class_<loom::SoundHandle>(m, "SoundHandle")
        .def_property_readonly("playing", &loom::SoundHandle::playing)
        .def("stop",       &loom::SoundHandle::stop)
        .def("set_volume", &loom::SoundHandle::set_volume, py::arg("volume"));

    // ── AudioEngine ───────────────────────────────────────────────────────────
    py::class_<loom::AudioEngine>(m, "AudioEngine")
        .def(py::init<>())
        .def_property_readonly("initialized", &loom::AudioEngine::initialized)
        .def("play_sound",        &loom::AudioEngine::play_sound,
             py::arg("path"), py::arg("volume")=1.f)
        .def("play_music",        &loom::AudioEngine::play_music,
             py::arg("path"), py::arg("volume")=1.f, py::arg("loop")=true)
        .def("stop_music",        &loom::AudioEngine::stop_music)
        .def("set_music_volume",  &loom::AudioEngine::set_music_volume)
        .def("music_playing",     &loom::AudioEngine::music_playing)
        .def("set_master_volume", &loom::AudioEngine::set_master_volume);

    // ── AssetManager ──────────────────────────────────────────────────────────
    py::class_<loom::AssetManager>(m, "AssetManager")
        .def(py::init<>())
        .def("texture", &loom::AssetManager::texture)
        .def("clear",   &loom::AssetManager::clear);

    // ── Game ──────────────────────────────────────────────────────────────────
    py::class_<Game, PyGame>(m, "Game")
        .def(py::init<>())
        .def_readwrite("clear_color",  &Game::clear_color)
        // The active scene. Read-only: to change scenes go through `scenes`,
        // which handles lifecycle hooks and transitions.
        .def_property_readonly("scene",  &Game::scene_ptr)
        .def_property_readonly("scenes", [](Game& g) -> loom::SceneManager& { return g.scenes; },
                               py::return_value_policy::reference_internal)
        .def_readwrite("auto_physics", &Game::auto_physics)
        .def_readwrite("auto_scene",   &Game::auto_scene)
        .def_readwrite("auto_timers",  &Game::auto_timers)
        .def_readwrite("auto_tweens",  &Game::auto_tweens)
        .def_readwrite("running",      &Game::running)
        .def_readwrite("logical_width",  &Game::logical_width)
        .def_readwrite("logical_height", &Game::logical_height)
        .def_readwrite("scale_mode",     &Game::scale_mode)
        .def_property_readonly("screen_width",  [](Game& g) { return g.screen_width;  })
        .def_property_readonly("screen_height", [](Game& g) { return g.screen_height; })
        .def_property_readonly("last_draw_calls", [](Game& g) { return g.last_draw_calls; })
        .def_property_readonly("ui",      [](Game& g) -> loom::UICanvas& { return g.ui; },
                               py::return_value_policy::reference_internal)
        .def_property_readonly("timers",  [](Game& g) -> loom::Timers& { return g.timers; },
                               py::return_value_policy::reference_internal)
        .def_property_readonly("tweens",  [](Game& g) -> loom::TweenManager& { return g.tweens; },
                               py::return_value_policy::reference_internal)
        .def_property_readonly("physics", [](Game& g) -> loom::PhysicsWorld& { return g.physics; },
                               py::return_value_policy::reference_internal)
        .def_property_readonly("audio",   [](Game& g) -> loom::AudioEngine&  { return g.audio;   },
                               py::return_value_policy::reference_internal)
        .def_property_readonly("assets",  [](Game& g) -> loom::AssetManager& { return g.assets;  },
                               py::return_value_policy::reference_internal)
        .def("on_start",  &Game::on_start)
        .def("on_update", &Game::on_update, py::arg("dt"))
        .def("on_draw",   &Game::on_draw)
        .def("on_stop",   &Game::on_stop)
        .def("on_resize", &Game::on_resize, py::arg("w"), py::arg("h"));

    // ── run() ─────────────────────────────────────────────────────────────────
    m.def("run", &loom::run_game,
          py::arg("game"),
          py::arg("title")  = "loom2d",
          py::arg("width")  = 800,
          py::arg("height") = 600,
          "Start the game loop. Blocks until the window closes.");
}
