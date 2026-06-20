"""
Pytest configuration for loom2d Python tests.
Adds the built package to sys.path so tests can import loom2d_native
from the build directory without a full `pip install`.
"""
import sys
import os
import pathlib
import glob

def _find_native_module():
    """Search common build-output locations for loom2d_native.pyd / .so."""
    root = pathlib.Path(__file__).parent.parent.parent  # tests/python/ -> tests/ -> project root
    # Check python/loom2d/ first (populated by build.ps1)
    pkg_dir = root / "python" / "loom2d"
    if any(pkg_dir.glob("loom2d_native*")):
        return str(pkg_dir)

    # Check build trees
    for pattern in [
        "build/**/loom2d_native*",
        "build/Debug/loom2d_native*",
        "build/Release/loom2d_native*",
    ]:
        hits = list(root.glob(pattern))
        if hits:
            return str(hits[0].parent)

    return None


# Prepend python/ to path for the pure Python package
sys.path.insert(0, str(pathlib.Path(__file__).parent.parent.parent / "python"))

native_dir = _find_native_module()
if native_dir:
    sys.path.insert(0, native_dir)
