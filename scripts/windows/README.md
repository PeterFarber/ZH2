# Windows build scripts

Clone the repo, open PowerShell at the repo root, and run:

```powershell
.\scripts\windows\setup.ps1
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\scripts\windows\test.ps1
```

That is the full first-time flow. The scripts handle Git, Visual Studio, CMake, Ninja, and vcpkg for you.

**First run notes**

- `setup.ps1` may ask for administrator access (UAC) to install Git and Visual Studio 2022.
- The first Visual Studio install and first `configure_debug.ps1` can take a while. Later runs are much faster.
- Re-running `setup.ps1` is safe — it skips anything already installed.

## After the first build

Rebuild and test:

```powershell
.\scripts\windows\build_debug.ps1
.\scripts\windows\test.ps1
```

Run the apps:

```powershell
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_client.ps1
.\scripts\windows\run_server.ps1
```

Release build:

```powershell
.\scripts\windows\configure_release.ps1
.\scripts\windows\build_release.ps1
```

## What each script does

- `setup.ps1` — install tools and prepare the build environment
- `configure_debug.ps1` / `configure_release.ps1` — generate the build files
- `build_debug.ps1` / `build_release.ps1` — compile
- `test.ps1` — run all tests
- `run_client.ps1` — game client
- `run_editor.ps1` — map editor
- `run_server.ps1` — headless dedicated server

Build output goes in `build/windows-debug/` or `build/windows-release/`.

## If something goes wrong

- **Declined the UAC prompt** — run `setup.ps1` again and approve it.
- **Setup still fails** — open a new terminal and run `setup.ps1` again.
- **`winget` not found** — install [App Installer](https://apps.microsoft.com/detail/9nblggh4nns1) from the Microsoft Store, then retry.
- **Moved the repo or configure looks wrong** — run `configure_debug.ps1` again.

For Linux, see `scripts/linux/`.
