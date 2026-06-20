"""
loom2d platformer example — demonstrates:
  - Scene graph (Node)
  - Input (keyboard WASD + Space)
  - Physics (dynamic player body, static floor)
  - Camera follow
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom

W, H = 800, 600
PLAYER_SPEED = 200.0  # px/s
JUMP_IMPULSE = 350.0  # px/s upward


class Platformer(loom.Game):
    def on_start(self):
        self.clear_color = loom.Color(0.15, 0.15, 0.25)
        print("Platformer started!")
        print("  WASD / arrow keys to move, Space to jump, Escape to quit")

        # Static floor
        self.floor = self.physics.create_body(loom.BodyType.Static,
                                              loom.Vec2(W / 2, H - 20))
        self.floor.add_box(W / 2, 20)

        # Dynamic player
        self.player = self.physics.create_body(loom.BodyType.Dynamic,
                                               loom.Vec2(W / 2, H / 2))
        self.player.add_box(16, 24, density=1.0, restitution=0.0)

        # Visual node (shows where the physics body is)
        self.player_node = loom.Node("player")
        self.scene.add(self.player_node)

    def on_update(self, dt):
        # Horizontal movement
        vx = 0.0
        if loom.Input.key_down(loom.Key.Left)  or loom.Input.key_down(loom.Key.A):
            vx = -PLAYER_SPEED
        if loom.Input.key_down(loom.Key.Right) or loom.Input.key_down(loom.Key.D):
            vx = +PLAYER_SPEED

        cur_vel = self.player.linear_velocity
        self.player.set_linear_velocity(loom.Vec2(vx, cur_vel.y))

        # Jump when nearly on the ground
        if loom.Input.key_pressed(loom.Key.Space):
            if abs(cur_vel.y) < 32.0:
                self.player.apply_impulse(loom.Vec2(0, -JUMP_IMPULSE))

        # Sync visual node to physics body
        pos = self.player.position
        self.player_node.position = loom.Vec2(pos.x, pos.y)

        # Camera loosely follows the player
        self.scene.camera.position = loom.Vec2(pos.x - W / 2, pos.y - H / 2)

    def on_stop(self):
        print("Thanks for playing!")


loom.run(Platformer(), title="loom2d Platformer", width=W, height=H)
