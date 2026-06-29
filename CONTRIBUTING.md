# Contributing to loom2d

Thanks for wanting to help! 🌱 loom2d is built to grow with its community. This
guide covers how changes land and how releases are cut — both are automated, so
the rules are short.

## The workflow at a glance

```
branch  →  pull request  →  required checks + review  →  merge to main  →  automatic release
```

1. **Branch.** Never commit to `main` directly. Start a branch named for the work,
   e.g. `feat/ui-toolkit`, `fix/gamepad-deadzone`.
2. **Open a pull request** against `main`. Give it a **Conventional-Commit title**
   (see below) — because we **squash-merge**, the PR title *becomes* the commit
   message on `main`, and that is what drives versioning.
3. **Pass the gates.** Every PR must (a) pass the required test checks
   (`Tests · ubuntu-latest` and `Tests · windows-2022`) and (b) get **one
   approving review from a maintainer**. GitHub does not let you approve your own
   PR, so external contributions are always reviewed by someone else.
4. **Merge.** Once green and approved, the PR is squash-merged.
5. **Release happens on its own** — see [How releases work](#how-releases-work).

> **Who can merge?** The repository owner (sole admin) may review and merge their
> own PRs. Everyone else must have a maintainer review and merge theirs — that's
> the whole point of the review gate. Keep this in mind if you're added as a
> maintainer: you still can't self-approve, and only the owner can bypass the
> review requirement.

## Conventional Commits (PR titles)

We follow [Conventional Commits](https://www.conventionalcommits.org/). The prefix
on your **PR title** decides the next version automatically:

| PR title prefix | Version effect | Example |
|---|---|---|
| `feat: …` | **minor** bump — `0.3.0 → 0.4.0` | `feat: UI toolkit` |
| `fix: …` | **patch** bump — `0.3.0 → 0.3.1` | `fix: gamepad dead-zone off-by-one` |
| `feat!: …` *or* a `BREAKING CHANGE:` footer | **major** bump — `0.x → 1.0.0` | `feat!: rename the Input API` |
| `docs:` · `chore:` · `ci:` · `test:` · `refactor:` · `style:` · `perf:` | **no release** — these merge but don't bump a version | `docs: clarify camera guide` |

Pick the prefix that matches the *user-visible* effect of the change. New
capability → `feat`. Bug fix → `fix`. Docs/tooling/internal-only → the
no-release prefixes.

## How releases work

Releases are automated with
[release-please](https://github.com/googleapis/release-please). You don't tag
versions or write changelogs by hand — release-please does both from the
Conventional-Commit history.

1. When a `feat`/`fix` (or breaking) change lands on `main`, release-please opens
   (or updates) a rolling **release PR** titled like `chore(main): release 0.4.0`.
   It bumps the version in `pyproject.toml` and updates `CHANGELOG.md`.
2. That release PR is **approved and auto-merged automatically** once its checks
   pass — so merging a `feat`/`fix` ships a release with no extra clicks
   (*continuous release*).
3. Merging the release PR creates the **`vX.Y.Z` git tag** and **GitHub Release**,
   then builds the **wheels** (Windows / macOS / Linux × CPython 3.11–3.13) and
   the sdist and attaches them to that release.

`docs:`/`chore:`/`ci:` merges intentionally produce **no** release — there's no
version change to ship. That's expected, not a bug.

### If a release fails

- **Tests fail on the release PR** → it simply doesn't merge; nothing is published.
  Fix `main`, and the release PR goes green and merges on its own.
- **Wheel build fails after the tag/Release exist** → the version is "spent" but
  some wheels are missing. Fix the build and re-run the release workflow (or the
  reusable `release.yml`) with the same tag; it re-attaches the wheels without
  bumping the version.

## Developing locally

You'll need **CMake ≥ 3.21**, a **C++17 compiler**, and **Python ≥ 3.11**. All C++
dependencies are fetched by CMake — nothing to install by hand. See
[BUILDING.md](BUILDING.md) for full per-platform detail.

**Windows (PowerShell):**
```powershell
.\build.ps1 Debug
.\run_tests.ps1
```

**macOS / Linux (bash):**
```bash
chmod +x build.sh run_tests.sh
./build.sh Debug
./run_tests.sh
```

The suite is headless (it runs in CI under SDL's `offscreen`/`dummy` drivers).
**New features should come with tests** — C++ unit tests in `tests/cpp/`, Python
tests in `tests/python/`. Add an example under `examples/` for anything visual
(the `--frames N` self-terminating smoke-test pattern doubles as a render check).

## Checklist before opening a PR

- [ ] Work is on a branch, not `main`.
- [ ] PR title is a Conventional Commit (`feat:` / `fix:` / `docs:` / …).
- [ ] `run_tests` is green locally.
- [ ] New behavior has tests (and an example, if it's visual).
- [ ] Docs updated if the change is user-facing (guides in `docs/`, plus the
      `README` for notable features).

For larger changes, open an issue first to discuss the approach. Thanks for
contributing! 💛
