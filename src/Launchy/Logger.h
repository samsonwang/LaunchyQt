
#pragma once

#include <QtGlobal>

namespace launchy {
namespace log {

// Uninstall the custom message handler and close the log file.
void stopLogging();

// Enable or disable debug logging:
// true -> QtDebugMsg (log everything), false -> QtWarningMsg and above.
void setDebugLogEnabled(bool enabled);

} // namespace log
} // namespace launchy
