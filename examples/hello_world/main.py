"""
loom2d — Phase 1 hello world.
Opens a window with a cornflower-blue background.
Press Escape or close the window to quit.
"""
import sys, os
# When running from the repo before installing, find the Python package
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom

class HelloWorld(loom.Game):
    def on_start(self):
        print("loom2d hello world started!")
        print("Press Escape or close the window to quit.")
        self.time = 0.0

    def on_update(self, dt):
        self.time += dt
        # Slowly cycle the clear color to show on_update is running
        import math
        r = (math.sin(self.time * 0.5) + 1) * 0.5 * 0.4
        g = (math.sin(self.time * 0.7 + 2) + 1) * 0.5 * 0.4
        b = 0.5 + (math.sin(self.time * 0.3 + 4) + 1) * 0.25
        self.clear_color = loom.Color(r, g, b, 1.0)

    def on_stop(self):
        print("Goodbye!")

loom.run(HelloWorld(), title="loom2d — Hello World", width=800, height=600)
