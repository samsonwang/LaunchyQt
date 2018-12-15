
#include "Directory.h"

namespace launchy {
Directory::Directory()
    : indexDirs(false),
      indexExe(false),
      depth(10) {
}

Directory::Directory(const QString& n)
    : indexDirs(false),
      indexExe(false),
      name(n),
      depth(10) {
}

Directory::Directory(const QString& n, const QStringList& t, bool d, bool e, int dep)
    : indexDirs(d),
      indexExe(e),
      name(n),
      types(t),
      depth(dep) {
}
}
