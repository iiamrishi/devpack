
# devpack

devpack is an open-source, C-based tool for installing **complete developer environments** using simple config files.

Instead of installing tools one-by-one (`winget install node`, `apt install git`, etc.), devpack lets you define a **stack** (like `web-basic`, `cpp-dev`, or `gamedev`) and installs + verifies everything in one go.

devpack is **not** a new package manager. It sits on top of:
- `winget` / `choco` on Windows
- `apt` (and later other managers) on Linux

The goal is:

> **"From bare machine to working dev environment with one command, and a verification that it actually works."**




## Goals

- 🧱 Declarative stacks: define environments in JSON (`stacks/web-basic.json`, etc.)
- 🧪 Verified installs: every tool has a `verify_cmd` (`node -v`, `code --version`, etc.)
- 🖥 Cross-platform: Windows + Linux (macOS later)
- 🧰 Use existing package managers: winget, choco, apt (no custom repo hosting)
- 🧵 Simple C implementation: no heavy runtime, no magic

## Non-goals (for now)

- ❌ Not a replacement for winget/apt
- ❌ No full snapshot/rollback system (maybe later)
- ❌ No GUI in v0.x (CLI-first, GUI can wrap it later)
- ❌ No hosting binaries or custom package format




