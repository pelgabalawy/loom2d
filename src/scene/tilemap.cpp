#include "scene/tilemap.hpp"
#include "graphics/renderer.hpp"
#include "graphics/sprite_batcher.hpp"
#include "graphics/camera.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace loom {

// ── Tileset ──────────────────────────────────────────────────────────────────

Tileset::Tileset(std::shared_ptr<Texture> tex, int tw, int th, int fg)
    : texture(std::move(tex)), tile_w(tw), tile_h(th), first_gid(fg) {}

int Tileset::columns_resolved() const {
    if (columns > 0) return columns;
    if (texture && tile_w > 0)
        return (texture->width() - 2 * margin + spacing) / (tile_w + spacing);
    return 0;
}

int Tileset::tile_count() const {
    int cols = columns_resolved();
    if (cols <= 0 || !texture || tile_h <= 0) return cols > 0 ? cols : 0;
    int rows = (texture->height() - 2 * margin + spacing) / (tile_h + spacing);
    return cols * rows;
}

bool Tileset::contains(int gid) const {
    return gid >= first_gid && gid <= last_gid();
}

Rect Tileset::source_for(int gid) const {
    int cols = columns_resolved();
    if (cols <= 0)
        return {0.f, 0.f, static_cast<float>(tile_w), static_cast<float>(tile_h)};
    int local = gid - first_gid;
    if (local < 0) local = 0;
    int col = local % cols;
    int row = local / cols;
    float x = static_cast<float>(margin + col * (tile_w + spacing));
    float y = static_cast<float>(margin + row * (tile_h + spacing));
    return {x, y, static_cast<float>(tile_w), static_cast<float>(tile_h)};
}

// ── TileLayer ────────────────────────────────────────────────────────────────

TileLayer::TileLayer(std::string n, int w, int h)
    : name(std::move(n)), width(w), height(h), gids(static_cast<size_t>(w) * h, 0) {}

int TileLayer::at(int x, int y) const {
    if (!in_bounds(x, y)) return 0;
    return gids[static_cast<size_t>(y) * width + x];
}

void TileLayer::set(int x, int y, int gid) {
    if (in_bounds(x, y)) gids[static_cast<size_t>(y) * width + x] = gid;
}

void TileLayer::fill(int gid) {
    std::fill(gids.begin(), gids.end(), gid);
}

// ── Tilemap ──────────────────────────────────────────────────────────────────

Tilemap::Tilemap(int tw, int th, int w, int h)
    : tile_w(tw), tile_h(th), width(w), height(h),
      m_solid(static_cast<size_t>(w) * h, 0) {}

std::shared_ptr<Tileset> Tilemap::add_tileset(std::shared_ptr<Texture> tex,
                                              int tw, int th, int first_gid) {
    auto ts = std::make_shared<Tileset>(std::move(tex), tw, th, first_gid);
    m_tilesets.push_back(ts);
    return ts;
}

void Tilemap::add_tileset(std::shared_ptr<Tileset> ts) {
    m_tilesets.push_back(std::move(ts));
}

Tileset* Tilemap::tileset_for_gid(int gid) const {
    // The matching tileset is the one with the greatest first_gid <= gid.
    Tileset* best = nullptr;
    for (const auto& ts : m_tilesets) {
        if (gid >= ts->first_gid && (!best || ts->first_gid > best->first_gid))
            best = ts.get();
    }
    return best;
}

std::shared_ptr<TileLayer> Tilemap::add_layer(const std::string& name) {
    auto l = std::make_shared<TileLayer>(name, width, height);
    m_layers.push_back(l);
    return l;
}

void Tilemap::add_layer(std::shared_ptr<TileLayer> layer) {
    m_layers.push_back(std::move(layer));
}

std::shared_ptr<TileLayer> Tilemap::layer(int index) const {
    if (index < 0 || index >= static_cast<int>(m_layers.size())) return nullptr;
    return m_layers[index];
}

std::shared_ptr<TileLayer> Tilemap::layer_by_name(const std::string& name) const {
    for (const auto& l : m_layers)
        if (l->name == name) return l;
    return nullptr;
}

Vec2 Tilemap::tile_to_world(int tx, int ty) const {
    Vec2 wp = world_position();
    Vec2 ws = world_scale();
    return {wp.x + tx * tile_w * ws.x, wp.y + ty * tile_h * ws.y};
}

Vec2 Tilemap::world_to_tile(Vec2 world) const {
    Vec2 wp = world_position();
    Vec2 ws = world_scale();
    float lx = (world.x - wp.x) / (tile_w * ws.x);
    float ly = (world.y - wp.y) / (tile_h * ws.y);
    return {std::floor(lx), std::floor(ly)};
}

void Tilemap::set_solid(int x, int y, bool solid) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    m_solid[static_cast<size_t>(y) * width + x] = solid ? 1 : 0;
}

bool Tilemap::is_solid(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) return false;
    return m_solid[static_cast<size_t>(y) * width + x] != 0;
}

void Tilemap::set_collision_from_layer(int layer_index) {
    auto l = layer(layer_index);
    if (!l) return;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (l->at(x, y) != 0) set_solid(x, y, true);
}

void Tilemap::clear_collision() {
    std::fill(m_solid.begin(), m_solid.end(), static_cast<std::uint8_t>(0));
}

bool Tilemap::rect_overlaps_solid(const Rect& wr) const {
    // Nudge the far edge inward so a rect that merely touches a tile boundary
    // does not report a collision with the next (empty-area) tile.
    const float eps = 0.0001f;
    Vec2 tl = world_to_tile({wr.left(), wr.top()});
    Vec2 br = world_to_tile({wr.right() - eps, wr.bottom() - eps});
    int tx0 = static_cast<int>(tl.x), ty0 = static_cast<int>(tl.y);
    int tx1 = static_cast<int>(br.x), ty1 = static_cast<int>(br.y);
    for (int ty = ty0; ty <= ty1; ++ty)
        for (int tx = tx0; tx <= tx1; ++tx)
            if (is_solid(tx, ty)) return true;
    return false;
}

void Tilemap::draw(Renderer& renderer, const Camera& camera) {
    if (!visible) { Node::draw(renderer, camera); return; }
    m_tiles_drawn = 0;

    Vec2 wp = world_position();
    Vec2 ws = world_scale();

    // Visible rect in the map's local (pre-scale) space, clamped to the grid.
    Rect vis = camera.visible_rect();
    float lx0 = (vis.left()  - wp.x) / ws.x;
    float lx1 = (vis.right() - wp.x) / ws.x;
    float ly0 = (vis.top()    - wp.y) / ws.y;
    float ly1 = (vis.bottom() - wp.y) / ws.y;
    if (lx0 > lx1) std::swap(lx0, lx1);
    if (ly0 > ly1) std::swap(ly0, ly1);

    int tx0 = std::max(0, static_cast<int>(std::floor(lx0 / tile_w)));
    int ty0 = std::max(0, static_cast<int>(std::floor(ly0 / tile_h)));
    int tx1 = std::min(width  - 1, static_cast<int>(std::floor(lx1 / tile_w)));
    int ty1 = std::min(height - 1, static_cast<int>(std::floor(ly1 / tile_h)));

    for (const auto& layer : m_layers) {
        if (!layer->visible) continue;
        Color tint = Color::white();
        tint.a = layer->opacity;
        for (int ty = ty0; ty <= ty1; ++ty) {
            for (int tx = tx0; tx <= tx1; ++tx) {
                int gid = layer->at(tx, ty);
                if (gid == 0) continue;
                Tileset* ts = tileset_for_gid(gid);
                if (!ts || !ts->texture) continue;
                Vec2 pos{wp.x + tx * tile_w * ws.x, wp.y + ty * tile_h * ws.y};
                SpriteQuad q = build_sprite_quad(
                    pos, 0.f, ws, {0.f, 0.f}, ts->source_for(gid),
                    ts->texture->width(), ts->texture->height(), false, false);
                renderer.batcher().submit(*ts->texture, q, tint);
                ++m_tiles_drawn;
            }
        }
    }

    Node::draw(renderer, camera);
}

// ── XML helpers (tiny, format-specific — not a general parser) ────────────────

namespace {

// Value of attribute `name` in a tag substring. Requires the attribute to be
// preceded by whitespace so e.g. looking up "width" never matches "tilewidth".
std::string attr(const std::string& tag, const std::string& name) {
    std::string needle = name + "=";
    size_t pos = 0;
    while ((pos = tag.find(needle, pos)) != std::string::npos) {
        bool boundary = (pos == 0) ||
                        std::isspace(static_cast<unsigned char>(tag[pos - 1]));
        size_t q = pos + needle.size();
        if (boundary && q < tag.size() && (tag[q] == '"' || tag[q] == '\'')) {
            char quote = tag[q];
            size_t end = tag.find(quote, q + 1);
            if (end != std::string::npos) return tag.substr(q + 1, end - (q + 1));
        }
        pos += needle.size();
    }
    return "";
}

int attr_int(const std::string& tag, const std::string& name, int def = 0) {
    std::string v = attr(tag, name);
    if (v.empty()) return def;
    try { return std::stoi(v); } catch (...) { return def; }
}

float attr_float(const std::string& tag, const std::string& name, float def = 0.f) {
    std::string v = attr(tag, name);
    if (v.empty()) return def;
    try { return std::stof(v); } catch (...) { return def; }
}

// The opening tag "<elem ...>" beginning at `start` (returns up to and
// including '>'). Empty if not found.
std::string opening_tag(const std::string& xml, size_t start) {
    size_t end = xml.find('>', start);
    if (end == std::string::npos) return "";
    return xml.substr(start, end - start + 1);
}

// Parse a CSV-encoded <data> body into gids, masking Tiled's flip flags.
std::vector<int> parse_csv(const std::string& body) {
    std::vector<int> out;
    std::string num;
    auto flush = [&]() {
        if (num.empty()) return;
        try {
            unsigned long g = std::stoul(num);
            out.push_back(static_cast<int>(g & 0x1FFFFFFFul)); // strip flip bits
        } catch (...) { /* skip junk */ }
        num.clear();
    };
    for (char c : body) {
        if (c == ',') flush();
        else if (!std::isspace(static_cast<unsigned char>(c))) num += c;
    }
    flush();
    return out;
}

std::string dir_of(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? "" : path.substr(0, slash + 1);
}

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Tilemap: cannot open " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace

// ── TMX / TSX parsing ────────────────────────────────────────────────────────

TmxTileset parse_tsx(const std::string& xml) {
    size_t ts = xml.find("<tileset");
    if (ts == std::string::npos)
        throw std::runtime_error("parse_tsx: no <tileset> element");
    std::string tag = opening_tag(xml, ts);

    TmxTileset out;
    out.tile_w     = attr_int(tag, "tilewidth");
    out.tile_h     = attr_int(tag, "tileheight");
    out.columns    = attr_int(tag, "columns");
    out.tile_count = attr_int(tag, "tilecount");
    out.margin     = attr_int(tag, "margin");
    out.spacing    = attr_int(tag, "spacing");

    size_t img = xml.find("<image", ts);
    if (img != std::string::npos) {
        std::string itag = opening_tag(xml, img);
        out.image   = attr(itag, "source");
        out.image_w = attr_int(itag, "width");
        out.image_h = attr_int(itag, "height");
    }
    return out;
}

TmxMap parse_tmx(const std::string& xml) {
    size_t mp = xml.find("<map");
    if (mp == std::string::npos)
        throw std::runtime_error("parse_tmx: no <map> element");
    std::string maptag = opening_tag(xml, mp);

    TmxMap out;
    out.width  = attr_int(maptag, "width");
    out.height = attr_int(maptag, "height");
    out.tile_w = attr_int(maptag, "tilewidth");
    out.tile_h = attr_int(maptag, "tileheight");

    // Tilesets (embedded or external reference).
    size_t pos = 0;
    while ((pos = xml.find("<tileset", pos)) != std::string::npos) {
        std::string tag = opening_tag(xml, pos);
        TmxTileset t;
        t.first_gid = attr_int(tag, "firstgid", 1);
        t.source    = attr(tag, "source");
        if (t.source.empty()) {
            // Embedded: read attrs + the <image> child.
            t.tile_w     = attr_int(tag, "tilewidth");
            t.tile_h     = attr_int(tag, "tileheight");
            t.columns    = attr_int(tag, "columns");
            t.tile_count = attr_int(tag, "tilecount");
            t.margin     = attr_int(tag, "margin");
            t.spacing    = attr_int(tag, "spacing");
            size_t img = xml.find("<image", pos);
            size_t close = xml.find("</tileset", pos);
            if (img != std::string::npos &&
                (close == std::string::npos || img < close)) {
                std::string itag = opening_tag(xml, img);
                t.image   = attr(itag, "source");
                t.image_w = attr_int(itag, "width");
                t.image_h = attr_int(itag, "height");
            }
        }
        out.tilesets.push_back(t);
        pos += 8; // past "<tileset"
    }

    // Layers.
    pos = 0;
    while ((pos = xml.find("<layer", pos)) != std::string::npos) {
        std::string tag = opening_tag(xml, pos);
        TmxLayer l;
        l.name    = attr(tag, "name");
        l.width   = attr_int(tag, "width");
        l.height  = attr_int(tag, "height");
        l.opacity = attr_float(tag, "opacity", 1.f);
        l.visible = attr_int(tag, "visible", 1) != 0;

        size_t data = xml.find("<data", pos);
        size_t layer_end = xml.find("</layer", pos);
        if (data != std::string::npos &&
            (layer_end == std::string::npos || data < layer_end)) {
            std::string dtag = opening_tag(xml, data);
            std::string enc = attr(dtag, "encoding");
            if (!enc.empty() && enc != "csv")
                throw std::runtime_error(
                    "parse_tmx: only CSV layer encoding is supported (got '" +
                    enc + "')");
            size_t body_start = xml.find('>', data);
            size_t body_end   = xml.find("</data", data);
            if (body_start != std::string::npos && body_end != std::string::npos)
                l.gids = parse_csv(xml.substr(body_start + 1,
                                              body_end - body_start - 1));
        }
        out.layers.push_back(std::move(l));
        pos += 6; // past "<layer"
    }

    return out;
}

std::shared_ptr<Tilemap> Tilemap::load(const std::string& path) {
    std::string base = dir_of(path);
    TmxMap m = parse_tmx(read_file(path));

    auto map = std::make_shared<Tilemap>(m.tile_w, m.tile_h, m.width, m.height);

    for (TmxTileset t : m.tilesets) {
        std::string image_path;
        int cols = t.columns, margin = t.margin, spacing = t.spacing;
        int tw = t.tile_w, th = t.tile_h;
        if (!t.source.empty()) {
            // External .tsx: parse it, image path is relative to the .tsx file.
            std::string tsx_path = base + t.source;
            TmxTileset ext = parse_tsx(read_file(tsx_path));
            image_path = dir_of(tsx_path) + ext.image;
            tw = ext.tile_w; th = ext.tile_h;
            cols = ext.columns; margin = ext.margin; spacing = ext.spacing;
        } else {
            image_path = base + t.image;
        }
        auto tex = Texture::load(image_path);
        auto ts = std::make_shared<Tileset>(tex, tw, th, t.first_gid);
        ts->columns = cols;
        ts->margin  = margin;
        ts->spacing = spacing;
        map->add_tileset(ts);
    }

    for (TmxLayer& l : m.layers) {
        auto layer = std::make_shared<TileLayer>(l.name, l.width, l.height);
        layer->visible = l.visible;
        layer->opacity = l.opacity;
        if (l.gids.size() == layer->gids.size())
            layer->gids = std::move(l.gids);
        map->add_layer(layer);
    }

    return map;
}

} // namespace loom
