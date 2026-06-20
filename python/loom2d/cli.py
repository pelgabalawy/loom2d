"""loom2d CLI — new / run / package commands."""
import argparse
import sys


def cmd_new(args):
    import pathlib
    dest = pathlib.Path(args.name)
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "main.py").write_text(
        f'import loom2d as loom\n\n'
        f'class {args.name.title().replace("_", "")}(loom.Game):\n'
        f'    def on_start(self):\n'
        f'        pass\n\n'
        f'    def on_update(self, dt):\n'
        f'        pass\n\n'
        f'loom.run({args.name.title().replace("_", "")}(), title="{args.name}")\n',
        encoding="utf-8",
    )
    (dest / "loom2d.toml").write_text(
        f'[project]\nname = "{args.name}"\nversion = "0.1.0"\nbundle_id = "com.example.{args.name}"\n',
        encoding="utf-8",
    )
    print(f"Created project: {dest}/")
    print(f"Run it with:  python {dest}/main.py")


def cmd_run(args):
    import subprocess
    subprocess.run([sys.executable, "main.py"], check=True)


def cmd_package(args):
    print(f"[loom2d] Packaging for target: {args.target}")
    print("Mobile packaging coming in Phase 7-8.")


def main():
    parser = argparse.ArgumentParser(prog="loom2d")
    sub = parser.add_subparsers(dest="command")

    p_new = sub.add_parser("new", help="Create a new game project")
    p_new.add_argument("name", help="Project name")

    sub.add_parser("run", help="Run the game in the current directory")

    p_pkg = sub.add_parser("package", help="Package the game for a platform")
    p_pkg.add_argument("target", choices=["win", "mac", "lin", "apk", "ipa"])

    args = parser.parse_args()
    if args.command == "new":
        cmd_new(args)
    elif args.command == "run":
        cmd_run(args)
    elif args.command == "package":
        cmd_package(args)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
