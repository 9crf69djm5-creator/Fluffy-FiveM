# Auto-update releases to GitHub

**Repo:** [9crf69djm5-creator/Fluffy-FiveM](https://github.com/9crf69djm5-creator/Fluffy-FiveM)

**Before any release:** Set **`APP_VERSION`** in the repo root (e.g. `1.6.0`) — the workflow uses this for the GitHub tag. Keep it in sync with `FLUFFY_APP_VERSION_STRING` in your local `src/Globals.hpp`. The workflow uploads to **`v` + APP_VERSION** and creates the release if needed.

Two ways to get a new build onto GitHub:

## 1. Push a version tag (CI builds and publishes)

1. Bump `FLUFFY_APP_VERSION_STRING` in `src/Globals.hpp`, commit, and push to your repo.
2. Create and push a tag:
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```
3. The **Build and Release** workflow runs: it builds the project on GitHub and creates a Release with `fluffyFiveM.exe` attached.

Use a new tag for each release (e.g. `v1.0.1`, `v1.0.2`).

## 2. Build locally and upload the exe

1. Build the project in Visual Studio (Release | x64). Close the exe if it’s running.
2. Install [GitHub CLI](https://cli.github.com/) and log in: `gh auth login`
3. From the repo root, run:
   ```powershell
   .\scripts\upload-release.ps1 -Tag v1.6.0
   ```
   Change the tag to match your release. The script finds `x64\Release\fluffyFiveM.exe` or `src\x64\Release\fluffyFiveM.exe` and uploads it (creates the release if it doesn’t exist).

## 3. Refresh the “latest” release after a local build (exe in repo)

If you commit `x64/Release/fluffyFiveM.exe`, pushing **main** runs **Update latest release**. After building, run:

```powershell
.\scripts\sync-exe-for-github.ps1
```

Then commit and push `x64/Release/fluffyFiveM.exe`.

---

---

**First time?** If the repo is empty, push your code first:
```bash
git remote add origin https://github.com/9crf69djm5-creator/Fluffy-FiveM.git
git branch -M main
git push -u origin main
```
Then use the tag or script steps above.

---

**Note:** The project uses `$(ExtraSdkInclude)` for an optional include path. If you need the old SDK path locally, add a `Directory.Build.props` in the solution folder with:

```xml
<Project>
  <PropertyGroup>
    <ExtraSdkInclude>C:\Users\Pietro\Downloads\Cheat\SDK\SDK\Include</ExtraSdkInclude>
  </PropertyGroup>
</Project>
```

(Adjust the path as needed.) CI leaves `ExtraSdkInclude` empty so the repo builds without that folder.
