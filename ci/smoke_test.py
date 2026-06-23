"""Wheel smoke test used by cibuildwheel (CIBW_TEST_COMMAND).

Prints packaging diagnostics *before* importing loom2d (so we still see them if
the import fails to find the bundled SDL), then exercises GPU-free tilemap logic.
"""
import os
import sys
import glob
import subprocess
import importlib.util

spec = importlib.util.find_spec("loom2d")
pkg_dir = os.path.dirname(spec.origin)
print("loom2d package dir:", pkg_dir)
print("contents:", sorted(os.listdir(pkg_dir)))

native = glob.glob(os.path.join(pkg_dir, "loom2d_native*"))
print("native module:", native)

if native and sys.platform.startswith("linux"):
    for tool in (["readelf", "-d"], ["ldd"]):
        try:
            r = subprocess.run(tool + [native[0]], capture_output=True, text=True)
            print(f"--- {' '.join(tool)} ---\n{r.stdout}{r.stderr}")
        except Exception as e:  # noqa: BLE001
            print(f"{' '.join(tool)} failed: {e}")

# The real smoke test — importing loads the native module + bundled SDL.
import loom2d as loom  # noqa: E402

m = loom.Tilemap(32, 32, 10, 10)
m.set_solid(3, 3, True)
assert m.rect_overlaps_solid(loom.Rect(100, 100, 8, 8))
assert not m.rect_overlaps_solid(loom.Rect(0, 0, 8, 8))
print("loom2d wheel OK:", loom.__file__)
