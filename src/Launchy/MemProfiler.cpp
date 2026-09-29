/*
  LaunchyQt memory profiler implementation (see MemProfiler.h).

  The snapshot struct, the sampler and the line formatter are internal to this
  translation unit and are kept in an anonymous namespace so they do not leak
  into the public header.
*/

#include "MemProfiler.h"

#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <psapi.h>
// Link the process status API on MSVC without touching CMakeLists.
#  pragma comment(lib, "psapi.lib")
#elif defined(Q_OS_LINUX)
#  include <QFile>
#  include <QTextStream>
#elif defined(Q_OS_DARWIN)
#  include <mach/mach.h>
#  include <mach/task_info.h>
#endif

namespace launchy {
namespace memprof {

// One captured sample of the process memory counters.
struct MemSnapshot {
    qulonglong workingSetKB = 0;     // RAM held (shared counts) - noisy
    qulonglong peakWorkingSetKB = 0; // peak RAM held since process start
    qulonglong privateKB = 0;        // Private Bytes - the real footprint
    qulonglong commitKB = 0;         // commit charge (PagefileUsage)
    qulonglong peakCommitKB = 0;     // peak commit charge
};

#if defined(Q_OS_LINUX)
// Reads one "Key: value kB" line from /proc/self/status and returns the value
// in kB (0 if the key is absent or the file cannot be read).
static qulonglong readProcStatusKb(const char* key) {
    QFile f(QStringLiteral("/proc/self/status"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    QTextStream in(&f);
    const QString prefix = QString::fromLatin1(key) + ':';
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.startsWith(prefix)) {
            QString value = line.mid(prefix.size()).trimmed();
            value.remove(QStringLiteral(" kB"));
            return value.toULongLong();
        }
    }
    return 0;
}
#endif

// Capture the current process memory counters. Returns false if the platform
// does not support it.
static bool takeSnapshot(MemSnapshot& out) {
#if defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS_EX counters = {};
    counters.cb = sizeof(counters);
    HANDLE h = GetCurrentProcess();
    if (!GetProcessMemoryInfo(h,
                              reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                              sizeof(counters))) {
        return false;
    }
    out.workingSetKB     = counters.WorkingSetSize / 1024;
    out.peakWorkingSetKB = counters.PeakWorkingSetSize / 1024;
    out.privateKB        = counters.PrivateUsage / 1024;
    out.commitKB         = counters.PagefileUsage / 1024;
    out.peakCommitKB     = counters.PeakPagefileUsage / 1024;
    return true;
#elif defined(Q_OS_LINUX)
    // Map the closest /proc/self/status analogues to the Windows fields:
    //   Working Set       -> VmRSS   (resident pages)
    //   Peak Working Set  -> VmHWM   (high-water resident)
    //   Commit charge     -> VmSize  (virtual size; Linux has no commit counter)
    //   Peak commit       -> VmPeak
    //   Private Bytes     -> RssAnon + RssShmem (memory not backed by a shared
    //                         file); falls back to VmRSS on kernels < 4.5 that
    //                         predate the Rss* lines.
    out.workingSetKB     = readProcStatusKb("VmRSS");
    out.peakWorkingSetKB = readProcStatusKb("VmHWM");
    out.commitKB         = readProcStatusKb("VmSize");
    out.peakCommitKB     = readProcStatusKb("VmPeak");
    const qulonglong priv = readProcStatusKb("RssAnon") + readProcStatusKb("RssShmem");
    out.privateKB        = (priv != 0) ? priv : out.workingSetKB;
    return out.workingSetKB != 0;
#elif defined(Q_OS_DARWIN)
    mach_task_basic_info_data_t info = {};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
        return false;
    }
    out.workingSetKB     = info.resident_size / 1024;
    out.peakWorkingSetKB = info.resident_size_max / 1024;
    out.commitKB         = info.virtual_size / 1024;
    // No separate peak-virtual counter is reported; reuse the current value.
    out.peakCommitKB     = info.virtual_size / 1024;

    // "Internal" memory is the portion not shared with other processes, the
    // closest Mach analogue to Windows Private Bytes.
    task_vm_info_data_t vm = {};
    mach_msg_type_number_t vcount = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO,
                  reinterpret_cast<task_info_t>(&vm), &vcount) == KERN_SUCCESS) {
        out.privateKB = vm.internal / 1024;
    }
    else {
        out.privateKB = out.workingSetKB;
    }
    return true;
#else
    Q_UNUSED(out);
    return false;
#endif
}

static QString formatLine(const QString& stage, const MemSnapshot& s, qlonglong delta) {
    return QString("[MEMPROF] %1 | WS=%2 peakWS=%3 private=%4 commit=%5 peakCommit=%6 | dPrivate=%7")
        .arg(stage)
        .arg(s.workingSetKB).arg(s.peakWorkingSetKB)
        .arg(s.privateKB).arg(s.commitKB).arg(s.peakCommitKB)
        .arg(delta);
}

// Shared state guarded by s_mutex. The catalog build runs on a worker thread
// while the GUI thread may also sample, so protect the delta bookkeeping. The
// enabled flag is read lock-free on the hot path (record/ScopedMem are called
// many times during startup); toggling it is benign under a benign race.
static QMutex s_mutex;
static volatile bool s_enabled = false;
static qlonglong s_lastPrivateKB = -1;

void setEnabled(bool enabled) {
    s_enabled = enabled;
    if (enabled) {
        // Reset the delta baseline so the first sample after re-enable is not
        // contaminated by a huge dPrivate accumulated while sampling was off.
        QMutexLocker locker(&s_mutex);
        s_lastPrivateKB = -1;
    }
}

qlonglong record(const QString& stage) {
    // Fast no-op when sampling is disabled; keep this branch cheap because
    // record() is called many times during catalog building.
    if (!s_enabled) {
        return -1;
    }

    MemSnapshot s;
    const bool ok = takeSnapshot(s);
    qlonglong delta = -1;
    QString logLine;

    QMutexLocker locker(&s_mutex);
    if (ok) {
        delta = (s_lastPrivateKB >= 0)
                    ? static_cast<qlonglong>(s.privateKB) - s_lastPrivateKB
                    : -1;
        s_lastPrivateKB = static_cast<qlonglong>(s.privateKB);
        logLine = formatLine(stage, s, delta);
    }
    else {
        logLine = QString("[MEMPROF] %1 (memory counters unavailable on this platform)")
                      .arg(stage);
    }

    qInfo().noquote() << logLine;
    return delta;
}

ScopedMem::ScopedMem(const QString& label)
    : m_label(label) {
    if (!s_enabled) {
        return;
    }
    MemSnapshot start;
    takeSnapshot(start);
    m_startPrivateKB = static_cast<qlonglong>(start.privateKB);
    qInfo().noquote() << QString("[MEMPROF] BEGIN %1 (private=%2KB)")
                         .arg(m_label).arg(m_startPrivateKB);
}

ScopedMem::~ScopedMem() {
    if (!s_enabled) {
        return;
    }
    MemSnapshot end;
    takeSnapshot(end);
    const qlonglong delta = static_cast<qlonglong>(end.privateKB) - m_startPrivateKB;
    record(QString("%1 [END]").arg(m_label));
    qInfo().noquote() << QString("[MEMPROF] END %1 dPrivate=%2KB")
                         .arg(m_label).arg(delta);
}

} // namespace memprof
} // namespace launchy