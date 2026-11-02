
#pragma once

#include <QFileIconProvider>

namespace launchy {

class IconProviderBase : public QFileIconProvider {
public:
    IconProviderBase();
    virtual ~IconProviderBase();

public:
    void setPreferredIconSize(int size);

protected:
    int m_preferredSize;
};

// This build's provider, heap allocated and owned by the caller.
//
// Defined next to the implementation it returns (IconProviderWin.cpp,
// IconProviderLinux.cpp) so that the one caller -- IconExtractor -- never has to
// know which of them this tree builds. Only one of those two files is in
// SOURCES_PLATFORM, so there is exactly one definition per build.
IconProviderBase* createIconProvider();

} // namespace launchy
