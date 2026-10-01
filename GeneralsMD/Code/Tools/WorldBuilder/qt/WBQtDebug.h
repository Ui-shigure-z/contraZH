// WBQtDebug.h -- opt-in debug logging for the WorldBuilder Qt inversion.
//
// WBQT_DBGLOG(fmt, ...) writes a printf-formatted line to the Win32 debug channel
// (OutputDebugString -- read it with DebugView, cdb, or the VS Output window). It is compiled to
// NOTHING unless WB_QT_KEYDEBUG is defined, so release/normal builds pay zero cost and print no
// spam. Enable it for a debugging session by configuring with -DWB_QT_KEYDEBUG=ON (the qt lib's
// CMakeLists forwards that to a compile define); works with any build (e.g. the win32-internal
// preset). Every line is prefixed with [WBDBG] so it is easy to filter.
//
// It was added after the Qt-inversion keyboard-routing bugs (Delete not deleting, Ctrl+C/V not
// reaching Object Properties text fields), where an OutputDebugString trace of the focus HWNDs was
// what actually pinned the cause. Keep using it for the next such bug rather than re-adding ad-hoc
// prints.
#ifndef WB_QT_DEBUG_H
#define WB_QT_DEBUG_H

#ifdef WB_QT_KEYDEBUG

#include <qt_windows.h>
#include <stdio.h>

// Format into a stack buffer and emit via OutputDebugStringA. Wrapped in do/while so it is a single
// statement (safe after a bare if). The [WBDBG] prefix + trailing newline are added here.
#define WBQT_DBGLOG(...) \
	do { \
		char wbqt_dbg_buf_[512]; \
		int wbqt_dbg_n_ = _snprintf(wbqt_dbg_buf_, sizeof(wbqt_dbg_buf_) - 2, "[WBDBG] " __VA_ARGS__); \
		if (wbqt_dbg_n_ < 0 || wbqt_dbg_n_ > (int)sizeof(wbqt_dbg_buf_) - 2) \
		{ \
			wbqt_dbg_n_ = (int)sizeof(wbqt_dbg_buf_) - 2; \
		} \
		wbqt_dbg_buf_[wbqt_dbg_n_] = '\n'; \
		wbqt_dbg_buf_[wbqt_dbg_n_ + 1] = 0; \
		::OutputDebugStringA(wbqt_dbg_buf_); \
	} while (0)

#else

#define WBQT_DBGLOG(...) do { } while (0)

#endif // WB_QT_KEYDEBUG

#endif // WB_QT_DEBUG_H
