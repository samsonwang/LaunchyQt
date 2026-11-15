# LaunchyQt v3.1.10 Changelog

**Release date**: 2026-11-15
**Previous release**: v3.1.9 (2026-10-03)
**Scope**: 56 commits, 317 files, +18302 / -54879

This release is mostly an engineering release: the whole application was split into
layers (`LaunchyCore` / `LaunchyPlatform` / `PluginInterface`) and the test suite went
from zero to 22 suites. What a user notices is a new search mode switch, high DPI that
actually works above 100%, several launch-time crash fixes, and a large round of plugin
work (WebSearchPy, Tasky, DiryPy, UWPApp, CalcyPy).

---

## Highlights

- **High DPI screens are honoured.** 125% / 150% scaling no longer collapses to 100%.
- **Fast / Slow search mode is now a user setting** — the catalog implementation used to
  be hardcoded to `CatalogFast`.
- **Fixed three launch-time failures**: launching a plugin item could crash, a history
  row could not be launched at all, and Python plugins failed to load whenever Launchy
  was started from a working directory other than its own.
- **WebSearchPy can open a URL you type directly**, and every engine shows its own
  site icon in the results.
- **The catalog index and the command history are now JSON files** (`catalog.json`,
  `history.json`) instead of binary blobs.

---

## New Features

### Search

- **Search mode option: Fast (Accurate) / Slow (Fuzzy)** (`Add user-facing option for search mode - fast/slow`, #145)
  Settings → General → "Search mode". Fast uses the trigram index (`CatalogFast`), Slow
  scans the whole catalog (`CatalogSlow`). The catalog type is chosen once at startup,
  so the option dialog tells you a restart is needed.
- **"Add default directories" button** in Settings → Catalog
  (`improve: refill default catalog directories manually`). Re-adds the built-in
  catalog directories and repairs paths that point at another user's profile (an
  account rename or a settings folder copied from another machine). It only runs when
  you click it — it is deliberately never applied at startup.
- **Faster catalog indexing** (`improve: Optimize catalog indexing performance`)
  The worker's duty cycle went from 10 ms of work / 1 ms sleep to 100 ms / 5 ms, the
  directory listing is throttled between batches as well, and the `.lnk` directory test
  no longer needs a substring search per entry.

### Options dialog

- **"Open config folder"** button (Settings → System) — opens the folder that holds
  `launchy.ini`, following the current portable / installed mode
  (`feature: add a button to open the config directory in the option dialog`).
- **"Open log folder"** button (Settings → System) — opens `%TEMP%\Launchy`, where the
  log and the crash dumps live (`feature: add buttion to open temp folder in option dialog`).
- **About page shows the build**: version, `git <hash>` and the build timestamp in one
  selectable label (`improve: add git hash and build time to version info`).

### Skin and UI

- The options dialog grew a few controls and had its pages re-laid out: the About page
  became a version block, the General page gained the search-mode combo box next to the
  screen selection, and the System page gained the config / log folder buttons.

---

## Fixes

### Crashes and launching

- **Fixed a crash when launching an item** (`fix: crash when launching item`)
  A plugin's `launchItem` (Verby runs `ShellExecuteExW`) or the platform's
  `runProgram()` can pump the message loop and re-enter `processInput()`, which replaces
  `m_inputData` — the code then read through a dangling reference. The query is now
  snapshotted before anything that can pump messages, and the item, the arguments and
  the history entry all come from that snapshot.
- **Fixed items with no path opening the Launchy folder** (`fix: improve tab with history input`)
  Launching an item with an empty `fullPath` fell back to Launchy's working directory and
  opened it in Explorer. It is now logged and skipped.
- **Fixed a plugin library load failure** (`fix: fail to load plugin library`)
  CPython loads extension modules with `LOAD_WITH_ALTERED_SEARCH_PATH`, which drops the
  application directory from the DLL search order, so `Qt5Qml.dll` / `python3.dll` next
  to `Launchy.exe` were found only when the process working directory happened to be
  that directory. `PlatformWin` now calls `SetDllDirectoryW` on the application
  directory before any plugin is loaded, so Python plugins also load when Launchy is
  started from a script, a shortcut with a "Start in" folder, or an autostart entry.

### History

- **Fixed history rows not being launchable** (`fix: unable to resolve history item`)
  A row in the alternatives list is a placeholder item (`pluginName == "History"`,
  `data` = history index); launching it handed the raw `key ► query` line to the shell.
  It is now expanded back into the original input segments, so the launch goes through
  the original plugin (Verby, WebSearchPy, ...) with the real arguments.
- **Tab no longer completes from a history row** (`fix: improve tab with history input`)
  Completion picks the first non-history match; if only history matched, the typed text
  is kept instead of being expanded into a second separator.

### Input and keyboard

- **Fixed the keyboard debounce** (`fix: keyboard debounce problems`)
  `keyPressEvent` only fires a search when `QKeyEvent::text()` is non-empty, so
  backspace, delete, cursor movement and paste never triggered one. `textEdited` is now
  the fallback trigger, and the handlers that rewrite the input box themselves (history
  preview, tab completion, back tab) block its signals so the debounced search cannot
  re-enter and rebuild the list under the user's cursor.
- **Enter always launches the final query**: a pending debounced search is executed
  instead of being cancelled, so a fast typist no longer launches the previous query.

### High DPI

- **Fixed high DPI support** (`fix: high dpi support`)
  `AA_EnableHighDpiScaling` and `AA_UseHighDpiPixmaps` are set before the application
  object is created, and Launchy is compiled with the `PassThrough` rounding policy, so
  125% stays 125% instead of being rounded back to 100% (Qt 5.12 defaults to `Round` and
  has no runtime setter for it, hence the compile definition).

### Window placement

- **Fixed the screen index and the widget position** (`fix: screen index and widget position problems`)
  The window is placed from the stored position onto the screen named by "Screen number",
  and is re-centred when that position is not on the selected screen. The old fractional
  "placement ratio" bookkeeping is gone: the saved point is used as-is instead of being
  recomputed from the previous screen geometry on every start. "Rescan screen" refreshes
  the list.

### Ranking

- **Fixed plugin results not reaching the top** (`fix: CalcyPy result not showing in the top`)
  When neither item contains the whole query as a contiguous substring, `searchCatalogs`'
  truncation comparator checked the word count before usage while `CatItemComparePtr`
  checked usage first — so a plugin result could be cut off by the result cap before it
  was ever ranked. Both now agree: usage first, word count second.
- `CatalogSlow` lowercases the query once per search instead of per item
  (`improve: slow search input text is already lowercased`).

---

## Plugins

### WebSearchPy

- **Type a URL and it opens** (`feature: open web url with WebSearchPy`)
  A single-segment input that looks like a website (`http://`, `https://`, `ftp://`,
  `www.`, common TLDs) produces an "Open <url>" result and opens the browser. Tab-separated
  engine queries are untouched.
- **Every engine shows its own site icon** (`improve: WebSearchPy plugin show every link favicon in results`)
  A new `FaviconCache` + `LaunchyProxy` pair downloads the site icon of each configured
  engine, honours Launchy's proxy setting, and falls back to the plugin icon until the
  download finishes.
- **Icon downloads are throttled to once per 12 hours** (`improve: WebSearchPy plugin throttle favicon request`)
  The last attempt is stored in a state file, so restarting Launchy inside the window
  does not re-run it; failed hosts get a longer back-off than successful ones.
- **A failed re-fetch keeps the previous icon** (`improve: keep old favicon when fail to fetch new`)
  The "refresh icons" button is now a soft refresh: it re-downloads the listed sites but
  keeps the old `.png` until a new one arrives.
- **Icons are cached in the temp folder** (`refactor: WebSearchPy save favicon to temp folder`)
  instead of next to the plugin, so a read-only install directory is no longer a problem.
- **Robust engine configuration** (`improve: WebSearchPy plugin`)
  A hand-edited ini can no longer break the option page: missing / unreadable values are
  repaired per row and highlighted in the table, duplicate or empty keys are rejected
  with a log line, and the engine editor validates the key characters (a key containing
  the tab separator, a space or outer whitespace can never be typed).
- **Updated default URLs**: Google search, Google Translate, Baidu Translate, the
  Arch Linux man page and the 64-bit cygwin package search.

### Tasky

- **Cloaked windows are filtered out** (`improve: Tasky and DiryPy experience`)
  UWP and other modern apps keep several top-level windows per instance; all but the
  visible one are "cloaked" by the DWM, yet every Win32 visibility flag says otherwise.
  They no longer fill the result list with entries no switch could show.
- **A hung application no longer freezes typing**: window titles are read with
  `SMTO_ABORTIFHUNG` and a 200 ms timeout, so a dead process costs one stalled query
  instead of a locked input box.
- **Matching is an ordered subsequence** of the window title instead of a character
  count, so `abc` no longer matches a title holding the same letters in any order; the
  match position becomes the item's usage, so a contiguous match sorts above a scattered one.
- **Icons are cached per executable**, and launching reactivates one of the windows the
  results actually offered instead of the first window a fresh lookup happens to find.

### DiryPy

- **Renamed from `PyDiry`** to match the plugin name (icon, modules and `.ui` files renamed).
- **Dynamic results outrank catalog matches** (`fix: py plugin bug`): folder entries get
  usage 50001 / file entries 50000, folders are collected first, and the tail is
  truncated at 200 results so a large folder cannot stall a keystroke.
- **Filter matching uses `fnmatchcase`**, and the "alias + filter" form is required
  (`alias ► filter`); a bare alias falls back to the catalog entry.

### UWPApp

- **Better icon selection from the package assets** (`improve: UWPApp plugin filter rules`)
  The `contrast-` family is checked before the unplated markers (a file can carry both,
  e.g. `...contrast-black...altform-unplated.png`, and it is a monochrome variant, not a
  regular logo). Properties the shell publishes as an empty variant are now treated as
  "not set" instead of an empty string, and boolean package properties only count as true
  when they are explicitly set.

### CalcyPy

- **Fixed the plugin stopping every few minutes** (issue #153, `fix: CalcyPy stops working periodically`)
  The settings dict is pre-filled with defaults, every key is validated, and the whole
  `getResults` body is guarded, so a partially read ini no longer leaves the plugin
  raising `KeyError` on every query with nothing in the log.
- Incomplete expressions (`3+`, `sqrt(`) are logged at debug level instead of being
  silently swallowed.

### Chrono

- **Only `start` and `stop` produce a result** (`improve: Chrono plugin only match "start" and "stop"`),
  and a tab-separated query is ignored instead of being counted as elapsed time.

### Verby

- **Folders open in the registered file manager** (`feature: support for third-party file explorers`)
  The "open containing folder" action uses the shell's default `open` verb instead of
  hardcoded `explorer.exe`, so Explorer alternatives (Files, Directory Opus, ...) are used.

---

## Icons

- **A shortcut's own configured icon now wins** (`improve: resolve user defined icon in sym link`)
  If the user set a custom icon on a `.lnk`, that icon is used instead of the resolved
  target's, and if the configured icon cannot be read the target's icon is used as before.
- **Shortcuts that store only an IDList** (Recycle Bin, This PC, Control Panel, Network)
  no longer fall back to a generic icon: the icon resource of the shell namespace item is
  extracted directly, without the shortcut arrow overlay.

---

## Interface and Localisation

- **Plugin labels are now public API** (`refactor: move plugin labels to PluginInterface`).
  The label and owner-name constants (`LABEL_*`, `NAME_*`) moved from an internal core
  header into `PluginInterface/PluginLabels.h`, which every plugin, every Python plugin
  (through `launchy.pyd`) and every test already links. Plugin-private labels stay inside
  their plugin.
- **`InputData` no longer carries a plugin name** (`refactor: remove plugin name from input data`).
  Ownership is expressed by labels (volatile, one `getLabels`/`getResults` pass) and by
  `CatItem::pluginName` of the top result (durable, what the launch routes on).
- Translations: the new strings ("Fast (Accurate)", "Slow (Fuzzy)", "Open Log Folder",
  "Failed to open the log folder", "The catalog directories are already up to date.") plus
  the reworked Credits page are **not translated yet** — 6 to 16 `<message>` entries per
  language are still `unfinished`, the Chinese ones included.

---

## Build and Engineering

### Layering

- **The source tree was split into libraries** (`refactor: code structure` and follow-ups):
  - `LaunchyCore` (STATIC) — catalog, ranking, history, options, plugin registry; Qt Core only.
  - `LaunchyPlatform` (STATIC) — directories, environment, program launching, icon extraction.
  - `PluginInterface` (SHARED) — the plugin ABI, **renamed from `LaunchyLib`**.
  - `Launchy` (exe) — `QApplication`, main window, plugin loading.
  This is what lets the tests run headless, and it is why `src/Launchy` no longer holds
  the whole program.
- **`PluginRegistry` is the single plugin entry point** (`refactor: catalog builder, logger, plugin registry`):
  `PluginHandler` and the app-side plugin facade are gone; registry, factory, option
  dialog and catalog builder all talk to the same table. `CatalogBuilder` and `Logger`
  moved into `LaunchyCore`.

### Tests

- **Added the unit test suite** (`feature: add unit test code`): 22 suites, roughly 235
  cases under `test/`, built with `-DLAUNCHY_BUILD_TESTS=ON` and run from `build/test`
  with `ctest -C Release`. Coverage includes catalog fast/slow search and ranking, the
  catalog builder, command history, settings, startup options, file search, plugin
  registry, logger, memory profiler, version tag parsing, `InputData` and the platform seam.

### Storage

- **The catalog and the command history are JSON** (`refactor: catalog save as json`,
  `refactor: command history save as json`): `catalog.json` and `history.json` with an
  explicit format version, written through `QSaveFile`. A malformed or truncated file is
  now reported and skipped instead of being silently half-loaded.

### CI and packaging

- **CI splits the symbols from the download** (`feature: split pdb and executable from github ci`):
  `Launchy-v3.2.10-win-x64.7z` (runnable tree) and `Launchy-v3.2.10-win-x64-pdb.7z`
  (symbols only), packed with solid LZMA2 compression. The archive name is read from
  `LaunchyVersion.h`, so a version bump can never mislabel a build.
- **The complete VC141 CRT ships next to `Launchy.exe`** (`improve: github ci pack all vc redist`)
  instead of three hand-picked DLLs, so the archive runs immediately after extraction.
- **CI asserts the build did not write into the checkout** (`fix: github ci git hash dirty`):
  the version string carries the git hash, and `BuildInfo.Generate` asks git for the tree
  state *while* the build runs, so any build rule that rewrites a tracked file used to
  make a pristine clone report `<hash>-dirty`. `qt5_create_translation` was the culprit
  (it rewrote all ten `translations/*.ts` location lines on every build); Qt5 now uses
  `qt5_add_translation` (lrelease only) and refreshing translations is the manual,
  non-`ALL` target `Launchy.UpdateTranslations`.
- **CMake lists every source file explicitly** (`refactor: CMakeLists list all files explicitly`);
  `file(GLOB)` is no longer used for build inputs.

### Removed

- `src/src_old/` — the Qt4-era Calcy / Controly sources, including a 38k-line vendored
  `exprtk.hpp`.
- `dist/win/installer/SETUP.iss` (the Inno Setup script) and the VS2005-era build
  helpers (`Create Visual Studio projects.cmd`, `Microsoft.VC80.CRT.manifest`,
  `buildTranslations.bat`).
- The `Useful Scripts` / `Special Folders` folders under `dist/win` lost their spaces, and
  the old `lpip.cmd` helper under the shipped Python folder was dropped.

---

## Upgrade Notes

1. **The catalog and the history are not migrated.** The binary `launchy.db` and
   `history.db` are intentionally ignored and read nothing; the new files are
   `catalog.json` and `history.json` next to `launchy.ini`. After the first start the
   catalog is empty until a rebuild runs — press F5, use the tray menu → "Rebuild
   catalog", or wait for the scheduled rebuild (it fires once the system has been idle
   for 10 minutes) — and the command history starts over.
2. **C++ plugins must be recompiled.** `LaunchyLib.dll` became `PluginInterface.dll`,
   `InputData::setPlugin()` / `getPlugin()` were removed, and the label constants moved to
   `PluginInterface/PluginLabels.h`. Nothing that links the old DLL will load.
3. **Python plugins**: `endDialog`, `launchyShow` and `launchyHide` are now optional
   overrides (they used to raise "Tried to call pure virtual function" for every plugin
   that did not implement them), `getCatalog` may safely acquire the GIL from the catalog
   thread, and a plugin that called `InputData::setPlugin` must switch to labels.
4. **High DPI changes the window size on scaled displays.** At 125% the UI is now 25%
   larger instead of being rendered at 100%; skins with fixed pixel metrics may look
   different than before.
5. **Search mode needs a restart.** It selects the catalog implementation at startup, so
   the option dialog only stores it and tells you to restart.
6. **Plugin results may move up.** Because usage now precedes the word count in the
   "no contiguous match" branch, results from plugins that set a high usage (DiryPy,
   CalcyPy, ...) sort above plain catalog matches where they previously might not.
