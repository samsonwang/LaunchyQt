
#include "Directory.h"

namespace launchy {

Directory::Directory()
    : indexDirs(false),
      indexExe(false),
      depth(5) {
}

Directory::Directory(const QString& n)
    : name(n),
      indexDirs(false),
      indexExe(true),
      depth(5) {
}

Directory::Directory(const QString& n, const QStringList& t,
                     bool d, bool e, int dep)
    : name(n),
      types(t),
      indexDirs(d),
      indexExe(e),
      depth(dep) {
}

} // namespace launchy
