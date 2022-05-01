
import sys, os
import logging as log

def main():
    redirectOutput()
    initLogging()
    initPipPackage()


def initSettings():
    "Set the launchy.settings object"
    #Based on http://lists.kde.org/?l=pykde&m=108947844203156&w=2
    try:
        log.info("launchy_util::initSettings, called")
        from PySide6 import QtCore
        from shiboken6 import wrapInstance
        log.debug("launchy_util::initSettings, succeed to import pyside and shiboken")
        import launchy
        log.debug("launchy_util::initSettings, launchy dir: %s" % dir(launchy))
        launchy.settings = wrapInstance(launchy.__settings, QtCore.QSettings)
        log.debug("launchy_util::initSettings, launchy.settings: %s" % launchy.settings)
    except ImportError as err:
        log.warning("launchy_util::initSettings, ImportError, %s" % err)
    except NameError as err:
        log.warning("launchy_util::initSettings, NameError, %s" % err)
    except Exception as err:
        log.warning("launchy_util::initSettings, Exception, %s" % err)


def initLogging():
    try:
        import launchy
        log_file_name = os.path.join(launchy.getAppTempPath(), 'launchypy.log')
    except:
        import pathlib
        log_file_name = os.path.join(pathlib.Path(__file__).parent.resolve(), 'launchypy.log')
    finally:
        print('launchy_util::initLogging, log file name:', log_file_name)
        log.basicConfig(filename=log_file_name,
                        filemode='w',
                        format='%(asctime)s.%(msecs)03d [%(levelname).1s] %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S',
                        level=log.DEBUG)
        log.info('launchy_util::initLogging, log init')


def redirectOutput():
    # If I write code such as sys.stdout = open('stdout.txt', 'w'),
    # I have to flush the file after writing to it (using "print" for example).
    # The following class saves the need to flush the file each time.
    class FlushedFile:
        """ Provides an output file that is immediately flushed after write """
        def __init__(self, filepath):
            try:
                self.terminal = sys.stdout
                self.file = open(filepath, 'w')
            except Exception:
                self.file = None

        def write(self, str):
            if self.file == None:
                return
            self.file.write(str)
            self.file.flush()

        def __getattr__(self, attr):
            try:
                self.file.flush()
                return getattr(self.terminal, attr)
            except Exception as err:
                print("launchy_util::redirectOutput, FlashedFile.__getattr__,", err)

    # Redirect stdout and stderr
    try:
        import launchy
        sys.stdout = FlushedFile(os.path.join(launchy.getAppTempPath(), 'py_stdout.log'))
        sys.stderr = FlushedFile(os.path.join(launchy.getAppTempPath(), 'py_stderr.log'))
    except:
        sys.stdout = FlushedFile('python/py_stdout.log')
        sys.stderr = FlushedFile('python/py_stderr.log')
    print("launchy_util::redirectOutput, redirect output finished")


def initPipPackage():
    log.debug("launchy_util::initPipPackage, sys.path: %s" % sys.path)
    log.debug("launchy_util::initPipPackage, env.path: %s" % os.environ.get('PATH', ''))
    log.info('launchy_util::initPipPackage, sys.prefix: %s' % sys.prefix)

    # print ("launchy_util::initPipPackage, env.path:", os.environ.get('PATH', ''))
    # print ("launchy_util::initPipPackage, sys.path:", sys.path)

    path = os.environ.get('PATH', '')

    log.debug('launchy_util::initPipPackage, path type: %s' % type(path))
    pathsplit = path.split(';')
    log.debug('launchy_util::initPipPackage, path split1: %s' % pathsplit)
    if not sys.prefix in pathsplit:
        pathsplit.insert(0, sys.prefix)
    log.debug('launchy_util::initPipPackage, path split2: %s' % pathsplit)
    pathjoin = ';'.join(str(s) for s in pathsplit)
    log.debug('launchy_util::initPipPackage, pathjoin: %s' % pathjoin)

    os.environ['PATH'] = pathjoin

    if sys.prefix not in sys.path:
        sys.path.insert(0, sys.prefix)
    # sys.path.insert(0, ".")

    xlib = os.path.join(sys.prefix, 'Lib')

    if os.path.exists(xlib):
        log.info('launchy_util::initPipPackage, Lib path found, init site')
        if xlib not in sys.path:
            sys.path.insert(0, xlib)
        import site
        site.main()
        os.chdir(sys.prefix)
    else:
        log.info('launchy_util::initPipPackage, Lib path not found, skip init site')

    log.info("launchy_util::initPipPackage, sys.path: %s" % sys.path)
    log.info("launchy_util::initPipPackage, env.path: %s" % os.environ.get('PATH', ''))

