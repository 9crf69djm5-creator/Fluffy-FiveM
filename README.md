# Fluffy-FiveM

## Releases

Download builds from [Releases](https://github.com/9crf69djm5-creator/Fluffy-FiveM/releases). Each release tag is `v` + the value in `APP_VERSION` at the repo root (for example `1.0.9` → **v1.0.9**).

## Version alignment (important)

Keep these in sync when you ship a build:

- **`APP_VERSION`** — used by GitHub Actions to name the release (`v<APP_VERSION>`) and to publish assets.
- **Embedded client version inside `Fluffy-FiveM.exe`** — the running app’s updater compares this to the server / latest release. If it does not match `APP_VERSION`, the updater may download repeatedly or loop.

Before you bump `APP_VERSION` and push, rebuild the exe so its internal version string matches, or republish a binary whose embedded version equals `APP_VERSION`.

## What gets published

On push to `main` (when `APP_VERSION` or release binaries change), the workflow publishes `Fluffy-FiveM.exe`, required DLLs, and **`update.zip`** (exe + DLLs) for in-app updates.

## Local tools

Scripts under `tools/`:

- `Update-FluffyFiveM.ps1` — pull latest assets from GitHub Releases into a folder you choose.
- `Create-Desktop-Backup.ps1` — snapshot exe, DLLs, and scripts to a backup folder.
- `Test-UpdaterSetup.ps1` — quick check that `APP_VERSION`, workflow, and updater script paths exist.
