"""Save files: JSON in the directory the operating system set aside for them.

A game does not get to choose where its saves live. Next to the executable is
wrong (the install directory is often read-only), the working directory is wrong
(it's wherever the player launched from), and the home directory is wrong
everywhere but Linux. `loom2d.save_dir()` asks the OS, so the same code puts
saves under %APPDATA% on Windows, ~/Library/Application Support on macOS,
$XDG_DATA_HOME on Linux, and inside the app sandbox on Android and iOS.

    save = SaveFile("Acme", "Coin Quest")
    data = save.load({"level": 1, "coins": 0})
    data["coins"] += 10
    save.save(data)

Writes are atomic: the new file is written alongside and then moved into place,
so a crash (or a pulled power cable) mid-save leaves the previous save intact
rather than a half-written one.
"""
import json as _json
import os as _os
import tempfile as _tempfile

from loom2d_native import save_dir

__all__ = ["SaveFile", "save_dir"]


class SaveFile:
    """One JSON save file.

    org/app name the directory the OS gives the game; `name` is the file inside
    it, so several SaveFiles ("slot1.json", "settings.json") can share a game's
    directory. Pass `directory` to override the location outright — mostly useful
    for tests and for portable installs.
    """

    def __init__(self, org, app, name="save.json", directory=None):
        self.directory = directory if directory is not None else save_dir(org, app)
        if not self.directory:
            raise OSError(f"no writable save directory for {org}/{app}")
        self.path = _os.path.join(self.directory, name)

    @property
    def exists(self):
        return _os.path.isfile(self.path)

    def load(self, default=None):
        """Read the save, or return `default` if there isn't a usable one.

        A missing file and a corrupt one are the same thing to a game: there is
        nothing to resume from, so start fresh rather than crash on the player.
        Pass the new-game state as `default` and the caller needs no branch.
        """
        try:
            with open(self.path, "r", encoding="utf-8") as f:
                return _json.load(f)
        except (OSError, ValueError):
            return default

    def save(self, data):
        """Write `data` as JSON, atomically."""
        _os.makedirs(self.directory, exist_ok=True)
        # Write into the destination directory (not the system temp dir) so the
        # replace below is a rename within one filesystem, which is what makes it
        # atomic; a cross-device move would copy, and could be seen half-done.
        fd, tmp = _tempfile.mkstemp(dir=self.directory, suffix=".tmp")
        try:
            with _os.fdopen(fd, "w", encoding="utf-8") as f:
                _json.dump(data, f, indent=2)
                f.flush()
                _os.fsync(f.fileno())
            _os.replace(tmp, self.path)
        except BaseException:
            try:
                _os.unlink(tmp)
            except OSError:
                pass
            raise

    def delete(self):
        """Remove the save file. Returns False if there wasn't one."""
        try:
            _os.remove(self.path)
            return True
        except OSError:
            return False
