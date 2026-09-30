// WBQtRoadBridge.cpp -- the MFC side of the Qt Road-panel seam. See WBQtFenceBridge.cpp for the
// pattern. Plain MFC TU (no Qt include); reverse callbacks resolved against the exe at the final
// link (extern "C" keeps the names stable). Whole body guarded by RTS_HAS_QT so the OFF build
// compiles it to an empty object and the MFC build is unchanged.
//
// The MFC RoadOptions is still created as the hidden OFF fallback and owns the road-type
// selection statics (m_currentRoadIndex / m_currentRoadName) plus the sticky corner-type / join
// state (m_angleCorners / m_tightCurve / m_doJoin) that RoadTool + the apply path read. The Qt
// Road panel MIRRORS the road/bridge list (enumerated straight from TheTerrainRoads here, in the
// same roads-then-bridges order the MFC tree used) and drives those statics + command handlers
// through RoadOptions::qt* (defined in RoadOptions.cpp), so RoadTool + PointerTool keep working.
#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "RoadOptions.h"
#include "GameClient/TerrainRoads.h"
#include "qt/panels/WBQtRoadBridge.h"

#ifdef RTS_HAS_QT

//----------------------------------------------------------------------------------------
// Helpers: copy a string into a caller buffer, and walk the road-then-bridge enumeration to
// the entry at a given running index (mirrors RoadOptions::OnInitDialog's addRoad loop order).
//----------------------------------------------------------------------------------------
namespace
{
	void copyString(char *out, int cap, const char *src)
	{
		if (out == NULL || cap <= 0)
		{
			return;
		}
		if (src == NULL)
		{
			out[0] = 0;
			return;
		}
		strncpy(out, src, cap - 1);
		out[cap - 1] = 0;
	}

	// Resolve the entry at running index (roads first, then bridges). isBridgeOut (optional) is
	// set to whether the entry came from the bridge list. Returns NULL when out of range.
	TerrainRoadType *roadAtIndex(int index, Bool *isBridgeOut)
	{
		int count = 0;
		TerrainRoadType *road;
		for (road = TheTerrainRoads->firstRoad(); road; road = TheTerrainRoads->nextRoad(road))
		{
			if (count == index)
			{
				if (isBridgeOut != NULL)
				{
					*isBridgeOut = FALSE;
				}
				return road;
			}
			count++;
		}
		TerrainRoadType *bridge;
		for (bridge = TheTerrainRoads->firstBridge(); bridge; bridge = TheTerrainRoads->nextBridge(bridge))
		{
			if (count == index)
			{
				if (isBridgeOut != NULL)
				{
					*isBridgeOut = TRUE;
				}
				return bridge;
			}
			count++;
		}
		return NULL;
	}
}

extern "C" {

//----------------------------------------------------------------------------------------
// Road / bridge list enumeration. The index is the running position (roads then bridges) --
// exactly the terrainNdx the MFC addRoad() stored as the tree lParam.
//----------------------------------------------------------------------------------------
int WBQtRoad_GetCount(void)
{
	int count = 0;
	TerrainRoadType *road;
	for (road = TheTerrainRoads->firstRoad(); road; road = TheTerrainRoads->nextRoad(road))
	{
		count++;
	}
	TerrainRoadType *bridge;
	for (bridge = TheTerrainRoads->firstBridge(); bridge; bridge = TheTerrainRoads->nextBridge(bridge))
	{
		count++;
	}
	return count;
}

int WBQtRoad_GetEntry(int index, char *groupOut, char *leafOut, int cap)
{
	Bool isBridge = FALSE;
	TerrainRoadType *road = roadAtIndex(index, &isBridge);
	if (road == NULL)
	{
		return 0;
	}
	// roads go under a "Roads" node, bridges under "Bridges", mirroring RoadOptions::addRoad().
	copyString(groupOut, cap, isBridge ? "Bridges" : "Roads");
	copyString(leafOut, cap, road->getName().str());
	return 1;
}

int WBQtRoad_GetFullName(int index, char *nameOut, int cap)
{
	TerrainRoadType *road = roadAtIndex(index, NULL);
	if (road == NULL)
	{
		return 0;
	}
	copyString(nameOut, cap, road->getName().str());
	return 1;
}

//----------------------------------------------------------------------------------------
// Selection.
//----------------------------------------------------------------------------------------
void WBQtRoad_SelectIndex(int index)
{
	TerrainRoadType *road = roadAtIndex(index, NULL);
	if (road != NULL)
	{
		RoadOptions::qtSelectIndex(index, road->getName().str());
	}
}

int WBQtRoad_GetSelectedIndex(void)
{
	return RoadOptions::qtGetCurrentIndex();
}

int WBQtRoad_GetCurrentName(char *nameOut, int cap)
{
	return RoadOptions::qtGetCurrentName(nameOut, cap);
}

//----------------------------------------------------------------------------------------
// Corner type + join + apply.
//----------------------------------------------------------------------------------------
int WBQtRoad_GetCornerType(void)
{
	return RoadOptions::qtGetCornerType();
}

void WBQtRoad_SetCornerType(int cornerType)
{
	RoadOptions::qtSetCornerType(cornerType);
}

int WBQtRoad_GetJoin(void)
{
	return RoadOptions::qtGetJoin();
}

void WBQtRoad_SetJoin(int on)
{
	RoadOptions::qtSetJoin(on);
}

void WBQtRoad_ApplyRoadType(void)
{
	RoadOptions::qtApplyRoadType();
}

//----------------------------------------------------------------------------------------
// Road snap distance. Persisted in the registry under the same section/key the MFC panel uses,
// clamped to [0.2, 5.0] exactly like RoadOptions::OnEditSnapPoint.
//----------------------------------------------------------------------------------------
double WBQtRoad_GetSnapDistance(void)
{
	CString value = ::AfxGetApp()->GetProfileString("RoadOptionPanel", "RoadSnappingDistance", "1.0");
	return atof(value);
}

void WBQtRoad_SetSnapDistance(double distance)
{
	if (distance > 5.0)
	{
		distance = 5.0;
	}
	if (distance < 0.2)
	{
		distance = 0.2;
	}
	CString strVal;
	strVal.Format("%.2f", distance);
	::AfxGetApp()->WriteProfileString("RoadOptionPanel", "RoadSnappingDistance", strVal);
}

//----------------------------------------------------------------------------------------
// Selection-derived checkbox state (mirrors RoadOptions::updateSelection()).
//----------------------------------------------------------------------------------------
void WBQtRoad_GetSelectionState(int *cornerTypeOut, int *joinOut, int *mixedOut,
	char *roadNameOut, int cap)
{
	RoadOptions::qtGetSelectionState(cornerTypeOut, joinOut, mixedOut, roadNameOut, cap);
}

}
#endif
