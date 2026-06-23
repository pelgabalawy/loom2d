"""
loom2d physics-events demo — a tiny, console-only tour of Phase 2.65:

  - contact events  (a falling ball lands on the ground)
  - sensor events   (the ball rolls through a trigger zone without being blocked)
  - raycast         (find what a ray hits, and where)
  - body tags       (to tell who-hit-what)

No window or GPU needed — physics runs headlessly, so this just prints.
Run:  python examples/physics_events/main.py
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom


def main():
    world = loom.PhysicsWorld(gravity_x=0, gravity_y=980)

    # Ground: a static floor.
    ground = world.create_body(loom.BodyType.Static, loom.Vec2(0, 300))
    ground.add_box(400, 10)
    ground.tag = "ground"

    # A trigger zone partway down: a sensor reports overlap but never blocks,
    # so the ball passes straight through it (enter then exit) on its way down.
    zone = world.create_body(loom.BodyType.Static, loom.Vec2(0, 150))
    zone.add_box(20, 20, is_sensor=True)
    zone.tag = "zone"

    # The ball: starts high and falls under gravity onto the ground.
    ball = world.create_body(loom.BodyType.Dynamic, loom.Vec2(0, 0))
    ball.add_circle(12, restitution=0.0)
    ball.tag = "ball"

    # --- React to events as they happen, via callbacks ---
    def on_contact_begin(a, b):
        tags = {a.tag, b.tag}
        if tags == {"ball", "ground"}:
            print(f"[contact] ball landed on the ground (y={ball.position.y:.0f})")

    def on_sensor_begin(sensor, visitor):
        print(f"[sensor]  {visitor.tag} entered the {sensor.tag}")

    def on_sensor_end(sensor, visitor):
        print(f"[sensor]  {visitor.tag} left the {sensor.tag}")

    world.on_contact_begin = on_contact_begin
    world.on_sensor_begin  = on_sensor_begin
    world.on_sensor_end    = on_sensor_end

    # --- Simulate two seconds at 60 Hz ---
    print("Simulating a falling ball...\n")
    for _ in range(120):
        world.step(1 / 60)

    print(f"\nBall came to rest at y={ball.position.y:.0f} "
          f"(ground surface is ~278).")

    # --- Raycast: shoot a ray straight down (offset from the sensor) ---
    hit = world.raycast(loom.Vec2(100, -100), loom.Vec2(100, 400))
    if hit:
        print(f"\n[raycast] straight down hit '{hit.body.tag}' "
              f"at y={hit.point.y:.0f} (fraction={hit.fraction:.2f})")
    else:
        print("\n[raycast] hit nothing")

    # A ray that misses everything.
    miss = world.raycast(loom.Vec2(-500, -500), loom.Vec2(-500, 500))
    print(f"[raycast] off to the side hit something? {bool(miss)}")


if __name__ == "__main__":
    main()
