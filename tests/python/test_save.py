"""Save files: JSON, written atomically, in the directory the OS set aside."""
import json
import os

import loom2d as loom


def test_save_dir_is_created_and_writable():
    directory = loom.save_dir("loom2d", "pytest")
    assert directory
    assert directory[-1] in ("/", "\\")   # callers join a filename onto it
    assert os.path.isdir(directory)       # asking for it creates it

    probe = os.path.join(directory, "probe.txt")
    with open(probe, "w", encoding="utf-8") as f:
        f.write("ok")
    assert os.path.isfile(probe)
    os.remove(probe)


def test_save_and_load_round_trip(tmp_path):
    save = loom.SaveFile("loom2d", "pytest", directory=str(tmp_path))
    assert not save.exists

    save.save({"level": 3, "coins": 120, "unlocked": ["forest", "cave"]})
    assert save.exists

    data = save.load()
    assert data == {"level": 3, "coins": 120, "unlocked": ["forest", "cave"]}


def test_load_returns_the_default_when_there_is_no_save(tmp_path):
    save = loom.SaveFile("loom2d", "pytest", directory=str(tmp_path))
    assert save.load({"level": 1}) == {"level": 1}
    assert save.load() is None


def test_a_corrupt_save_reads_as_no_save(tmp_path):
    # A half-written or hand-edited file must not crash the game on startup:
    # there is nothing to resume from, which is the same situation as a new game.
    save = loom.SaveFile("loom2d", "pytest", directory=str(tmp_path))
    with open(save.path, "w", encoding="utf-8") as f:
        f.write("{not json at all")

    assert save.load({"level": 1}) == {"level": 1}


def test_saving_leaves_no_temporary_files_behind(tmp_path):
    # The write goes to a temp file in the same directory and is then moved into
    # place — atomically, so a crash mid-save keeps the previous save. Check the
    # scratch file is not left lying around.
    save = loom.SaveFile("loom2d", "pytest", directory=str(tmp_path))
    save.save({"a": 1})
    save.save({"a": 2})

    assert sorted(p.name for p in tmp_path.iterdir()) == ["save.json"]
    assert save.load() == {"a": 2}


def test_overwriting_replaces_the_previous_save(tmp_path):
    save = loom.SaveFile("loom2d", "pytest", directory=str(tmp_path))
    save.save({"coins": 1})
    save.save({"coins": 2})

    with open(save.path, encoding="utf-8") as f:
        assert json.load(f) == {"coins": 2}


def test_several_files_share_one_game_directory(tmp_path):
    settings = loom.SaveFile("loom2d", "pytest", "settings.json", directory=str(tmp_path))
    slot1 = loom.SaveFile("loom2d", "pytest", "slot1.json", directory=str(tmp_path))

    settings.save({"music": False})
    slot1.save({"level": 7})

    assert settings.load() == {"music": False}
    assert slot1.load() == {"level": 7}


def test_delete_removes_the_save(tmp_path):
    save = loom.SaveFile("loom2d", "pytest", directory=str(tmp_path))
    save.save({"level": 9})

    assert save.delete()
    assert not save.exists
    assert not save.delete()      # nothing left to remove
