#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
PluginPy: the python-visible contract of launchy.pyd.

PluginPy is a python extension module, so its surface only exists inside an
interpreter -- no C++ test can reach it. This runs the interpreter found at
configure time (the same 3.6.8 the app embeds must be used, the .pyd is built
against it) from the application tree, which is where launchy.pyd and the Qt
runtime DLLs it links against live:

    TestPluginPy.py [app-dir]        # app-dir defaults to the current directory

ctest sets the working directory to the application tree, so the bare form is
what runs there. <app-dir>/python is put on sys.path explicitly rather than
relying on the cwd: Qt5*.dll resolves through the cwd, the module through the
path.

What this covers, and why each part is worth locking:

  * the module surface plugins call (CatItem, getAppTempPath) -- a renamed
    binding breaks every plugin at once, and only at runtime;
  * the Plugin trampolines, in both directions: an overridden method must reach
    python, and the notifications a plugin is allowed to omit (launchyShow,
    launchyHide, endDialog) must not raise. Making those mandatory is exactly
    the regression that threw "Tried to call pure virtual function" on every
    show/hide and closed dialogs;
  * launchy.settings existing before any plugin module is imported -- CalcyPy
    and WebSearchPy do "from launchy import settings" at module scope, so this
    is the difference between a plugin that loads and one that does not;
  * every shipped python plugin still importing and instantiating, which is the
    smoke test that used to be run by hand;
  * launchy_util.main() in a child process: it replaces sys.stdout/stderr for
    the rest of the process, so testing it in-process would swallow the test
    report.
"""

import os
import subprocess
import sys
import unittest

# Optional positional argument: the application tree. Popped so that unittest
# does not read it as a test name.
if len(sys.argv) > 1 and not sys.argv[1].startswith("-"):
    APP_DIR = os.path.abspath(sys.argv.pop(1))
else:
    APP_DIR = os.getcwd()

APP_DIR = os.path.normpath(APP_DIR)
PYTHON_DIR = os.path.join(APP_DIR, "python")
PLUGIN_DIR = os.path.join(APP_DIR, "plugins")

# Before the import, not after: launchy.pyd links the Qt DLLs that sit next to
# Launchy.exe, and the Windows loader resolves them from the working directory.
# Doing it here also means the script can be started from anywhere.
os.chdir(APP_DIR)
sys.path.insert(0, PYTHON_DIR)

try:
    import launchy
    import launchy_util
except ImportError as err:
    # 77 tells ctest to report the case as skipped instead of failed: the
    # application tree is not runnable yet (launchy.pyd is built by
    # PluginPy.Deploy, and the Qt DLLs it links against only reach that tree
    # through windeployqt, which CI runs after ctest). A renamed binding or a
    # broken helper still fails loudly below -- this only covers "not built".
    sys.stderr.write(
        "TestPluginPy: cannot import launchy from %s (%s).\n"
        "Run it from the application tree, or pass that directory as the first "
        "argument. If the tree is otherwise built, its Qt runtime has not been "
        "deployed next to Launchy.exe yet.\n"
        % (PYTHON_DIR, err))
    raise SystemExit(77)

# Has to happen before any plugin module is imported: several plugins do
# "from launchy import settings" at module scope, and the real object is only
# installed later by initSettings().
launchy_util.initSettingsPlaceholder()


class ModuleSurface(unittest.TestCase):
    """The module functions and the import itself."""

    def testLoadsFromTheApplicationTree(self):
        self.assertEqual(
            os.path.normcase(os.path.dirname(os.path.abspath(launchy.__file__))),
            os.path.normcase(PYTHON_DIR),
            "launchy was imported from somewhere else: %s" % launchy.__file__)

    def testAppTempPathIsCreatedAndForwardSlashed(self):
        path = launchy.getAppTempPath(False)
        self.assertTrue(os.path.isdir(path), path)
        self.assertEqual(os.path.basename(path), "Launchy")
        # Plugins concatenate their own sub paths onto this string
        self.assertNotIn("\\", path)

    def testAppTempPathNativeKeepsPlatformSeparators(self):
        native = launchy.getAppTempPath(True)
        self.assertTrue(os.path.isdir(native), native)
        if os.sep == "\\":
            self.assertIn("\\", native)
            self.assertNotIn("/", native)

    def testSetNeedRebuildCatalogIsCallable(self):
        # Reports a settings change; the catalog picks the flag up on its next
        # pass. Nothing to read back, the contract is that it does not raise.
        launchy.setNeedRebuildCatalog()


class CatItemBinding(unittest.TestCase):
    """CatItem is the only way a plugin can hand an entry to the catalog."""

    def testFieldsCarryThrough(self):
        item = launchy.CatItem("C:/tools/a.exe", "a", "MyPlugin", "C:/i.ico")
        self.assertEqual(item.fullPath(), "C:/tools/a.exe")
        self.assertEqual(item.shortName(), "a")
        self.assertEqual(item.iconPath(), "C:/i.ico")

    def testEmptyIconIsAllowed(self):
        item = launchy.CatItem("C:/tools/a.exe", "a", "MyPlugin", "")
        self.assertEqual(item.iconPath(), "")

    def testFourArgumentsAreRequired(self):
        # The third argument is the plugin name, not an icon: a plugin written
        # against the old three argument form must fail loudly instead of
        # registering an item with no owner.
        with self.assertRaises(TypeError):
            launchy.CatItem("C:/tools/a.exe", "a", "MyPlugin")
        with self.assertRaises(TypeError):
            launchy.CatItem("C:/tools/a.exe")

    def testSetUsageAcceptsAnInt(self):
        item = launchy.CatItem("C:/tools/a.exe", "a", "MyPlugin", "")
        item.setUsage(50000)
        item.setUsage(-1)


class PluginContract(unittest.TestCase):
    """The trampolines in ExportPyPlugin.h, from the python side."""

    class Full(launchy.Plugin):
        def __init__(self):
            # Every shipped plugin does this, so a fixture without it would not
            # represent one.
            launchy.Plugin.__init__(self)
            self.calls = []
            self.parents = []

        def init(self):
            self.calls.append("init")

        def getName(self):
            return "full"

        def setPath(self, path):
            self.calls.append(("setPath", path))

        def doDialog(self, parent):
            self.calls.append(("doDialog", parent))
            self.parents.append(parent)
            return 1234

        def endDialog(self, accept):
            self.calls.append(("endDialog", accept))

        def launchyShow(self):
            self.calls.append("launchyShow")

        def launchyHide(self):
            self.calls.append("launchyHide")

    class Bare(launchy.Plugin):
        """Implements nothing: most plugins leave the notifications out."""

    def testOverridesAreCalled(self):
        plugin = self.Full()
        self.assertEqual(plugin.getName(), "full")

        plugin.init()
        plugin.setPath("C:/plugins/full")
        plugin.launchyShow()
        plugin.launchyHide()
        plugin.endDialog(True)

        self.assertEqual(plugin.calls,
                         ["init", ("setPath", "C:/plugins/full"),
                          "launchyShow", "launchyHide", ("endDialog", True)])

    def testMandatoryMethodsRaiseWhenMissing(self):
        # getName and init are what the loader calls first: a plugin without
        # them has to fail here rather than register under an empty name.
        plugin = self.Bare()
        for call in (plugin.init, plugin.getName):
            with self.assertRaises(RuntimeError) as ctx:
                call()
            self.assertIn("pure virtual", str(ctx.exception))

    def testOptionalNotificationsAreSilent(self):
        # launchyShow/launchyHide/endDialog are looked up without failing, and
        # doDialog reports "no dialog" as a null handle. Going through
        # PYBIND11_OVERLOAD_PURE here took the whole launcher down.
        plugin = self.Bare()
        plugin.launchyShow()
        plugin.launchyHide()
        plugin.endDialog(True)
        self.assertIsNone(plugin.doDialog(None))

    def testDialogHandleRoundTrips(self):
        # The dialog handle travels as an opaque value: the app passes a parent
        # widget pointer, the plugin returns the handle of the page to show, and
        # a null handle means "this plugin has no page". Confirmed from the
        # python side: None comes back as None, an int as itself, and the
        # returned int reaches the caller unchanged.
        plugin = self.Full()
        self.assertEqual(plugin.doDialog(None), 1234)
        self.assertEqual(plugin.doDialog(0), 1234)
        self.assertEqual(plugin.doDialog(5), 1234)
        self.assertEqual(plugin.parents, [None, 0, 5])

        # A plugin that does not override doDialog reports "no page" too
        self.assertIsNone(self.Bare().doDialog(None))

    def testSubclassIsStillAPlugin(self):
        # PluginLoader decides "is this a python plugin" with
        # py::isinstance<exportpy::Plugin>, so a subclass has to satisfy it.
        self.assertIsInstance(self.Full(), launchy.Plugin)
        self.assertIsInstance(self.Bare(), launchy.Plugin)


class SettingsPlaceholder(unittest.TestCase):
    """launchy.settings before the real object exists."""

    def testImportStatementUsedByPluginsWorks(self):
        # CalcyPy and WebSearchPy do exactly this at module scope. Without the
        # placeholder the import raises ImportError and the plugin is skipped.
        namespace = {}
        exec("from launchy import settings", namespace)
        self.assertIsNone(namespace["settings"])

    def testAttributeIsPresent(self):
        self.assertTrue(hasattr(launchy, "settings"))
        self.assertIsNone(launchy.settings)


class PluginPackages(unittest.TestCase):
    """Every shipped python plugin still loads and instantiates."""

    @classmethod
    def setUpClass(cls):
        try:
            import PySide2  # noqa: F401
        except ImportError:
            raise unittest.SkipTest("PySide2 not deployed next to launchy.pyd")
        if not os.path.isdir(PLUGIN_DIR):
            raise unittest.SkipTest("no plugins directory in %s" % APP_DIR)

        cls.names = sorted(
            name for name in os.listdir(PLUGIN_DIR)
            if os.path.isfile(os.path.join(PLUGIN_DIR, name, name + ".py")))
        if not cls.names:
            raise unittest.SkipTest("no python plugin packages in %s" % PLUGIN_DIR)

    def testEveryPluginLoadsAndInstantiates(self):
        for name in self.names:
            with self.subTest(plugin=name):
                plugin_dir = os.path.join(PLUGIN_DIR, name)
                sys.path.insert(0, plugin_dir)
                try:
                    module = __import__(name)
                except Exception as err:
                    self.fail("%s failed to import: %s: %s"
                              % (name, type(err).__name__, err))

                self.assertTrue(hasattr(module, "getPlugin"),
                                "%s has no getPlugin()" % name)
                instance = module.getPlugin()()
                self.assertIsInstance(instance, launchy.Plugin, name)
                self.assertTrue(isinstance(instance.getName(), str)
                                and instance.getName().strip(),
                                "%s returned an empty name" % name)


class UtilMain(unittest.TestCase):
    """launchy_util.main(): output redirection plus the python log."""

    def testMainPreparesOutputAndLogging(self):
        child = (
            "import launchy_util\n"
            "launchy_util.main()\n"
            "print('pluginpy-child-marker')\n"
        )
        env = dict(os.environ)
        env["PYTHONPATH"] = PYTHON_DIR

        # A child process: main() replaces sys.stdout/stderr for the rest of its
        # process, which would swallow this test's own report.
        process = subprocess.Popen(
            [sys.executable, "-c", child],
            cwd=APP_DIR, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = process.communicate(timeout=120)

        self.assertEqual(process.returncode, 0,
                         "child failed: %r\n%s" % (out, err))

        stdout_log = os.path.join(launchy.getAppTempPath(False), "py_stdout.log")
        self.assertTrue(os.path.isfile(stdout_log), stdout_log)
        with open(stdout_log, "r") as handle:
            content = handle.read()
        self.assertIn("pluginpy-child-marker", content)
        self.assertIn("redirect output finished", content)

        log_file = os.path.join(launchy.getAppTempPath(False), "launchypy.log")
        self.assertTrue(os.path.isfile(log_file), log_file)
        with open(log_file, "r") as handle:
            log = handle.read()
        self.assertIn("launchy_util::initLogging, log init", log)
        # main() also installs the settings placeholder, and logging is at debug
        # level, so the placeholder's own debug line has to be there too.
        self.assertIn("initSettingsPlaceholder", log)


if __name__ == "__main__":
    unittest.main(verbosity=2)
