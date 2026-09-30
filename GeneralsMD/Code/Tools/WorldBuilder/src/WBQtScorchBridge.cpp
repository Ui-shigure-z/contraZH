// WBQtScorchBridge.cpp -- the MFC side of the Qt Scorch-panel seam. See WBQtFenceBridge.cpp /
// WBQtBrushBridge.cpp for the pattern. Plain MFC TU (no Qt include); reverse callbacks
// resolved against the exe at the final link. Whole body guarded by RTS_HAS_QT so the OFF
// build compiles it to an empty object.
//
// The MFC ScorchOptions is still Create()d as the hidden OFF fallback and owns the selection
// statics (m_scorchtype / m_scorchsize). ScorchOptions::qtSetScorchType/qtSetScorchSize run
// changeScorch()/changeSize(), which build a DictItemUndoable against every selected scorch
// MapObject (TheKey_scorchType / TheKey_objectRadius), exactly like the MFC handlers. This
// bridge lets the Qt Scorch panel read + drive those statics.
#include "StdAfx.h"
#include "ScorchOptions.h"
#include "qt/panels/WBQtScorchBridge.h"

#ifdef RTS_HAS_QT

// The scorch-type combo entries, baked into the IDD_SCORCH_OPTIONS combo in WorldBuilder.rc
// (the MFC panel loads them from the .rc, not from the Scorches enum). The selected index maps
// 1:1 to the Scorches value ScorchOptions stores; the Qt panel mirrors this exact list.
static const char *kScorchTypeNames[] =
{
	"Scorch 1",
	"Scorch 2",
	"Scorch 3",
	"Scorch 4",
	"Round Shadow",
	"Crack Marks"
};
static const int kScorchTypeCount = (int)(sizeof(kScorchTypeNames) / sizeof(kScorchTypeNames[0]));

extern "C" {

int WBQtScorch_GetTypeCount(void)
{
	return kScorchTypeCount;
}

int WBQtScorch_GetTypeName(int index, char *nameOut, int cap)
{
	if (nameOut == NULL || cap <= 0)
	{
		return 0;
	}
	if (index < 0 || index >= kScorchTypeCount)
	{
		nameOut[0] = 0;
		return 0;
	}
	strncpy(nameOut, kScorchTypeNames[index], cap - 1);
	nameOut[cap - 1] = 0;
	return 1;
}

int WBQtScorch_GetType(void)
{
	return ScorchOptions::qtGetScorchType();
}

void WBQtScorch_SetType(int type)
{
	ScorchOptions::qtSetScorchType(type);
}

double WBQtScorch_GetSize(void)
{
	return ScorchOptions::qtGetScorchSize();
}

void WBQtScorch_SetSize(double size)
{
	ScorchOptions::qtSetScorchSize(size);
}

}
#endif
