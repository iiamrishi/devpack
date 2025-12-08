# devpack

**devpack** is an open-source, C-based CLI tool that installs and verifies complete developer environments using simple JSON configuration files.

Instead of installing tools one at a time (`apt install git`, `winget install nodejs`, etc.), devpack lets you define a *stack* (for example `web-dev`, `cpp-dev`, or `python-dev`) and installs **and verifies** everything in the correct order — including dependencies.

> From a bare machine to a working dev environment with one command — and proof that it actually works.

---

## What devpack is (and is not)

**devpack is not a package manager.**  
It sits on top of existing system package managers and focuses on structure, repeatability, and verification.

Supported package managers:

- **Linux:** pacman, apt, dnf, yum, zypper (auto-detected)
- **Windows:** winget, choco (via `windows_cmd`)
- **macOS:** planned

devpack coordinates installs and verifies results; it does not replace your OS tooling.

---

## Key Features

- 🧱 **Declarative stacks**  
  Define complete environments using JSON (`stacks/web-dev.json`, etc.)

- 🧪 **Verified installs**  
  Every package includes a `verify_cmd` (`node --version`, `git --version`, etc.)

- 🧵 **Dependency-aware**  
  Stacks can depend on other stacks (`web-dev → python-dev`)

- ♻️ **Dry-run support**  
  See exactly what commands will execute before running anything

- 🖥 **Cross-distro Linux support**  
  Automatically detects available package managers

- 🪟 **Windows-friendly design**  
  Supports `windows_cmd` entries (WSL recommended for now)

- ⚡ **Lightweight C implementation**  
  Single binary, no runtime dependencies, no background services

---

## Commands

```bash
devpack list
devpack list --json

devpack stacks
devpack stacks --json

devpack verify web-dev
devpack install web-dev
devpack install web-dev --dry-run

devpack doctor
devpack --version
