# Scene Graph

loom2d organizes everything you draw into a **scene graph**: a tree of `Node`
objects. Each node has a transform (position, rotation, scale) that is **relative to
its parent**, so moving a parent moves all of its children with it.

## Nodes

A `Node` is the base building block. On its own it draws nothing — it is a transform
and a container — but `SpriteNode`, `TextNode`, and `Tilemap` all inherit from it.

```python
import loom2d as loom

node = loom.Node("hero")          # optional name
node.position = loom.Vec2(100, 50)
node.rotation = 0.5               # radians
node.scale    = loom.Vec2(2, 2)   # 2× in both axes
node.visible  = True              # skip drawing (and children) when False
```

### Transform properties

| Property | Type | Notes |
|----------|------|-------|
| `position` | `Vec2` | local position relative to the parent |
| `x`, `y` | `float` | shortcuts for `position.x` / `position.y` |
| `rotation` | `float` | radians, clockwise |
| `scale` | `Vec2` | per-axis scale |
| `visible` | `bool` | when `False`, the node and its children are not drawn |
| `name` | `str` | optional label for lookup/debugging |

!!! tip "Use `x`/`y` in hot loops"
    `node.position = loom.Vec2(x, y)` allocates a new `Vec2` each call. In tight
    per-object loops (hundreds of moving nodes), assign `node.x` / `node.y` directly
    to avoid the allocation.

## Parent / child hierarchy

Build a hierarchy with `add_child`. Children inherit their parent's transform.

```python
ship   = loom.SpriteNode(ship_tex)
turret = loom.SpriteNode(turret_tex)
turret.position = loom.Vec2(0, -20)   # 20px above the ship's origin

ship.add_child(turret)                # turret now follows the ship
ship.position = loom.Vec2(400, 300)   # moves both
ship.rotation = 0.3                   # rotates the turret around the ship too
```

To read a node's absolute transform (after walking up the parent chain), use the
world helpers:

```python
turret.world_position()   # Vec2 in screen/world space
turret.world_rotation()   # float, radians
turret.world_scale()      # Vec2
```

Remove a node from its parent with:

```python
turret.remove_from_parent()
```

And inspect the tree:

```python
for child in ship.children():
    print(child.name)
```

## The Scene

Your `Game` already owns a `Scene` as `self.scene`. The scene holds a root node and
a [`Camera`](camera.md), and it is updated and drawn for you every frame.

```python
class Game(loom.Game):
    def on_start(self):
        self.scene.add(self.player)        # add to the root
        self.scene.add(self.background)

    # ...later...
    def reset_level(self):
        self.scene.clear()                 # remove everything
```

| Method / property | Description |
|-------------------|-------------|
| `scene.add(node)` | add a node to the root |
| `scene.remove(node)` | remove a node from the root |
| `scene.clear()` | remove all nodes |
| `scene.camera` | the scene's [`Camera`](camera.md) |
| `scene.root()` | the root `Node` (add deep hierarchies here) |

## Draw order

Nodes are drawn in the order they are added — later nodes appear **on top**. Add your
background first, gameplay sprites next, and the HUD last.

```python
self.scene.add(self.background)   # drawn first  (back)
self.scene.add(self.player)       # drawn second (middle)
self.scene.add(self.hud)          # drawn last   (front)
```

## See also

- [Sprites & Textures](sprites.md) — the most common drawable node
- [Camera](camera.md) — pan, zoom, and convert between screen and world space
- [Text & Fonts](text.md) — `TextNode` for HUDs and labels
