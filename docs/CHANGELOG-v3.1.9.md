# LaunchyQt v3.1.9 Changelog

**Release date**: 2026-10-03
**Previous release**: v3.1.8 (2025-07-05)
**Scope**: 41 commits, 151 files, +10079 / -9385

---

## New Features

### Search

- **Each word in the search input is matched independently** (`improve: match each word in search input`)
  Multi-word queries no longer require the whole string to match contiguously. Results are
  more accurate and better ranked when several query words hit the same item.

- **Faster catalog search**
  `CatalogFast` gained a trigram inverted index. A search first narrows the candidate set by
  3-character substrings, then runs the full subsequence test on the candidates, instead of
  scanning every item. The catalog is now split into `CatalogFast` and `CatalogSlow`:
  large catalogs trade memory for speed, small ones trade speed for memory.

- **Catalog rebuild is triggered by system idle time (10 minutes)**
  Scheduled rebuilds now wait until the system has been idle long enough. While the system is
  in use the check is repeated every minute. Manual requests (F5, tray menu, options dialog,
  rescan at startup) still rebuild immediately and are unaffected.

### Options

- **Resolve symbol link target** (new option, enabled by default)
  Settings → General → "Resolve symbol link". When enabled, `.lnk` shortcuts and symbolic
  links are resolved to their real target before the icon is fetched.
- **Enable debug logging** (new option, disabled by default)
  Settings → System → "Enable debug logging". Replaces the log-level switch that used to live
  only in the source code (`GenOps/logLevel`).

### Default catalog directories

- **The Public Desktop is now indexed by default**, fixing shortcuts on the shared desktop
  not showing up in results.
- **User directories are validated**: stale entries in the catalog directory list no longer
  slow down every scan.

### Skins

- New **Flat Modern Dark** skin.
- New **Flat Modern Light** skin.

---

## Fixes

### Crashes

- **Fixed crashes on startup** (`fix: crashes upon startup`) — timing issues in main widget
  initialisation, the character list widget, and the Windows / Linux platform branches.
- **Fixed runtime crashes** (`fix: crash problems`) — races and null dereferences in the
  catalog thread, command history, and the plugin handler.
- **Fixed recursive catalog deadlock** (`fix catalog recursive deadlock`) — the catalog mutex
  is now recursive, so a rebuild no longer blocks on itself.

### Catalog

- **Fixed root directories not being indexed**: an input ending in a separator (for example
  `C:\`) left `matchWords` with an empty word list, which discarded the whole directory
  listing. The directory contents are now returned correctly. A new `looksLikePath()` helper
  recognises bare drive letters, `~`, UNC prefixes and URLs, so something like `http://` is
  no longer mistaken for a path.
- **Fixed an icon extractor thread synchronisation issue** (`fix: icon extractor thread sync issue`).

### Window placement

- **Fixed widget relocation when the display configuration changes** (`fix: widget relocation when display changes`)
  The window position is re-validated on monitor add/remove and primary display change, and
  falls back to the screen under the window when the configured screen index no longer covers it.

### Icons

- **Network path icon extraction is dramatically faster** (`improve: avoid network path icon extraction, it is very slow`)
  A `.lnk` pointing at a network target (UNC path or mapped remote drive) is no longer resolved
  even when "Resolve symbol link" is on — it was measured blocking for roughly 22 seconds. The
  icon is now taken from the `.lnk` itself via its type icon. The target string is read with a
  dedicated `IShellLink` + `SLGP_RAWPATH` call instead of Qt's `symLinkTarget()`, which uses
  `SLGP_UNCPRIORITY` and may probe the remote host.

### Plugins

- **UWP apps**: improved icon resolution (shell property → `AppxManifest.xml` logo →
  qualified assets → ranked enumeration → extra folders → `IShellItemImageFactory`), with results
  cached in `plugins/UWPApp/cache/uwp_<aumid>.png`. The old cache name embedded the package
  version, so icons went blank after an app update.
- **CalcyPy**: fixed decimal point and group separator handling — it now uses the symbols from
  the user's settings instead of the system locale defaults.

### Update checking

- **The latest version is now fetched from the GitHub API**, and the OpenSSL runtime
  (`libssl-1_1-x64.dll` / `libcrypto-1_1-x64.dll`) is shipped alongside it. HTTPS update
  checks previously failed with a `[warn]` line in the log.

---

## Plugins

### Added

| Plugin | Description |
|---|---|
| **Birdie** | Reads Edge bookmarks, generates search entries per bookmark folder, and can launch a selected script with one keystroke |
| **Chrono** | Counts time between two points in time and formats the result for JIRA (ported to the 3.1.8 plugin interface) |
| **Shortie** | Maps strings to key-combination sequences and simulates the keystrokes, giving quick access to Launchy shortcuts and PowerToys functions |

### Renamed

- `PydiryPy` → **DiryPy** (icon and internal modules renamed as well)
- `WeekNum` → **Week Number**

### Updated

- **Camel** and **Edge** adapted to the new plugin interface.

### Removed

- Deleted the obsolete `Runner.pro` and `Verby.pro` build files.

---

## Interface and Localisation

- The options dialog was reorganised: layout and control order changed, modifier-key and key
  combo boxes were added, and the tab structure was reworked.
- Updated the About page links (GitHub issue tracker / project page).
- Translations: **all 10 languages are now complete** (de / es / fr / it / ja / nl / pt / ru /
  zh_CN / zh_TW). Every new and corrected string has been translated.

---

## Build and Engineering

- **Added a full GitHub Actions CI pipeline** (`.github/workflows/build.yml`): v141 toolchain →
  Qt → Python 3.6.8 → configure → build → deploy the Qt runtime → prune PySide2 → package
  Python and skins → verify the artifacts (PE import table scan).
- **Added `misc/deploy_win.ps1`**: local release packaging, relocating the Qt runtime to
  `qtlibs/` via `misc/qt.conf`.
- **Added `misc/update_run_dir.ps1`**: updates the build artifacts into the run directory
  (stop processes → copy only this project's artifacts → verify with SHA256 → request UAC when
  needed), leaving third-party files untouched.
- Windows PDB debug info is generated and compiler warnings cleaned up.
- Removed dead code, unused plugin imports, and the old documentation directory (`docs/docs_old/`).

### Dependency baseline changes

| Dependency | v3.1.8 | v3.1.9 |
|---|---|---|
| Qt | 6.2.4 | **5.12.10** |
| Python | 3.9.7 | **3.6.8** |

> ⚠️ These are **major build baseline changes** and directly affect third-party plugin
> compatibility (PySide2 must be 5.12.6, Python syntax must be 3.6 compatible). Take this
> into account when updating existing plugins.

### New internal tool

- **Memory profiler** (`MemProfiler`): a `ScopedMem` RAII probe that samples on demand and
  records private memory deltas. It is part of the build and was used to track memory usage
  during catalog rebuilds.

---

## Upgrade Notes

1. **Plugin authors**: Qt went from 6.2.4 down to 5.12.10 and Python from 3.9.7 down to 3.6.8.
   Plugins using Python 3.7+ syntax or PySide6 need to move to PySide2 and stay Python 3.6
   compatible.
2. **Icons for `.lnk` files on network paths**: even with "Resolve symbol link" enabled,
   shortcuts whose target is on the network now use the icon of the `.lnk` itself. This is an
   intentional change to avoid a ~22 second stall, but the icon may differ from the target
   program's real icon.
3. **The log level setting moved**: the old `GenOps/logLevel` entry has been removed in favour
   of Settings → System → "Enable debug logging".
4. **Scheduled catalog rebuilds changed timing**: they no longer run at a fixed time, and are
   instead triggered once the system has been idle long enough. Manual triggers are unaffected.
