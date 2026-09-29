#pragma once

#include <QString>

namespace launchy {
namespace memprof {

// Enable/disable actual memory recording. When disabled, record() and
// ScopedMem skip takeSnapshot() and produce no qInfo() output. Defaults to
// disabled so a normal run has zero overhead.
void setEnabled(bool enabled);

// Log one memory sample tagged with `stage`.
// Returns the delta of Private Bytes (KB) versus the previous sample, or -1
// if there was no previous sample / counters are unavailable / sampling is
// disabled.
qlonglong record(const QString& stage);

// RAII helper: records Private Bytes on construction and again on destruction,
// logging the delta. Both constructor and destructor are no-ops when sampling
// is disabled.
class ScopedMem {
public:
    explicit ScopedMem(const QString& label);
    ~ScopedMem();
private:
    QString m_label;
    qlonglong m_startPrivateKB = -1;
};

} // namespace memprof
} // namespace launchy