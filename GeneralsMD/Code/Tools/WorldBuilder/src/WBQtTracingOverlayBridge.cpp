// WBQtTracingOverlayBridge.cpp -- the MFC side of the Qt Tracing Overlay settings seam.
//
// Plain MFC translation unit (no Qt include). Implements the data half of
// qt/panels/WBQtTracingOverlayBridge.h over the TracingOverlayOptions statics (the
// persisted [Appearance] settings) and DrawObject (the live overlay state), plus the
// qtSetFromUi member static declared (guarded) in TracingOverlayOptions.h -- member
// statics may be defined in any TU, which lets the bridge write the protected statics
// without touching TracingOverlayOptions.cpp.
//
// The whole body is guarded by RTS_HAS_QT, so the default (Qt-off) build compiles this
// to an empty object.
#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "TracingOverlayOptions.h"
#include "DrawObject.h"
#include "qt/panels/WBQtTracingOverlayBridge.h"

#ifdef RTS_HAS_QT

// Mirrors the file-local defines in TracingOverlayOptions.cpp.
#define APPEARANCE_SECTION			"Appearance"
#define KEY_TRACE_OPACITY			"TraceOverlayOpacity"
#define KEY_TRACE_FILTER			"TraceOverlayFilter"

// Push the Qt UI's values into the statics + DrawObject (== readControlsToStatics +
// applyToDrawObject); persist == the MFC persistToProfile, called only on commit points
// (slider release, combo change, OK/close), never per drag tick.
void TracingOverlayOptions::qtSetFromUi(Int opacityPct, Int filter, Bool persist)
{
	if (opacityPct < OPACITY_MIN)
	{
		opacityPct = OPACITY_MIN;
	}
	if (opacityPct > OPACITY_MAX)
	{
		opacityPct = OPACITY_MAX;
	}
	if (filter != FILTER_NEAREST)
	{
		filter = FILTER_DEFAULT;
	}
	m_opacityPct = opacityPct;
	m_filter = filter;

	// Opacity is 0..100 in the ui; DrawObject wants 0..255 alpha.
	Int alpha = (m_opacityPct * 255) / 100;
	DrawObject::setTracingOverlayOpacity(alpha);
	DrawObject::setTracingOverlayFilter(m_filter);

	if (persist)
	{
		::AfxGetApp()->WriteProfileInt(APPEARANCE_SECTION, KEY_TRACE_OPACITY, m_opacityPct);
		::AfxGetApp()->WriteProfileInt(APPEARANCE_SECTION, KEY_TRACE_FILTER, m_filter);
	}
}

extern "C" int WBQtTracingOverlayData_GetOpacityPct(void)
{
	return TracingOverlayOptions::getOpacityPct();
}

extern "C" int WBQtTracingOverlayData_GetFilter(void)
{
	return TracingOverlayOptions::getFilter();
}

extern "C" void WBQtTracingOverlayData_SetFromUi(int opacityPct, int filter, int persist)
{
	TracingOverlayOptions::qtSetFromUi(opacityPct, filter, persist != 0);
}

#endif // RTS_HAS_QT
