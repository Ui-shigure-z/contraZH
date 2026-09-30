// WBQtTerrainModalBridge.cpp -- the MFC side of the Qt missing-terrain-texture seam
// (Tier 5b). Plain MFC TU; whole body guarded by RTS_HAS_QT so the OFF build compiles it
// to an empty object. Ports TerrainModal's row model (updateTextures/addTerrain grouping,
// including the initial-selection bump past classes the map already uses) and the
// updateLabel UI-name leaf.
#define DEFINE_TERRAIN_TYPE_NAMES		// instantiate terrainTypeNames[] in this TU

#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "TerrainMaterial.h"
#include "WHeightMapEdit.h"
#include "WorldBuilderDoc.h"
#include "Common/TerrainTypes.h"
#include "qt/panels/WBQtTerrainModalBridge.h"

#ifdef RTS_HAS_QT

#include <vector>
#include <string>

static void copyOut(const char *s, char *buf, int cap)
{
	if (buf == NULL || cap <= 0)
	{
		return;
	}
	strncpy(buf, (s != NULL) ? s : "", cap - 1);
	buf[cap - 1] = 0;
}

struct WBQtTmRow
{
	std::string group;		// '/'-joined folder chain
	std::string leaf;
	int texClass;
};

static std::vector<WBQtTmRow> s_qtTmRows;
static int s_qtTmInitialSel = 0;

extern "C" int WBQtTerrainModalData_Build(void *heightMapEdit)
{
	WorldHeightMapEdit *pMap = static_cast<WorldHeightMapEdit *>(heightMapEdit);
	s_qtTmRows.clear();
	s_qtTmInitialSel = 0;

	for (Int i = 0; i < WorldHeightMapEdit::getNumTexClasses(); i++)
	{
		// == updateTextures: bump the initial selection past classes the map uses.
		if (pMap != NULL && pMap->isTexClassUsed(i))
		{
			if (s_qtTmInitialSel == i)
			{
				s_qtTmInitialSel++;
			}
		}

		// == addTerrain's grouping.
		AsciiString className = WorldHeightMapEdit::getTexClassName(i);
		TerrainType *terrain = TheTerrainTypes->findTerrain(className);
		WBQtTmRow row;
		row.texClass = (int)i;
		if (terrain)
		{
			row.group = "Unknown";
			for (TerrainClass tc = TERRAIN_NONE; tc < TERRAIN_NUM_CLASSES; tc = (TerrainClass)(tc + 1))
			{
				if (terrain->getClass() == tc)
				{
					row.group = terrainTypeNames[tc];
					break;
				}
			}
			row.leaf = terrain->getName().str();
		}
		else
		{
			// Legacy GDF entries: nested folders from the path components (skipping the
			// "." directory), leaf = the last component. Verbatim walk from addTerrain.
			row.group = "**LegacyGDF";
			char buffer[_MAX_PATH];
			const char *pPath = className.str();
			Bool doAdd = false;
			Int ci = 0;
			while (pPath[ci] && ci < (Int)sizeof(buffer))
			{
				if (pPath[ci] == '\\' || pPath[ci] == '/')
				{
					if (ci > 0 && (ci > 1 || buffer[0] != '.'))
					{
						buffer[ci] = 0;
						row.group += "/";
						row.group += buffer;
					}
					pPath += ci + 1;
					ci = 0;
				}
				buffer[ci] = pPath[ci];
				buffer[ci + 1] = 0;
				ci++;
				doAdd = true;
			}
			if (!doAdd)
			{
				continue;
			}
			row.leaf = buffer;
		}
		s_qtTmRows.push_back(row);
	}
	return (int)s_qtTmRows.size();
}

extern "C" void WBQtTerrainModalData_GetInfo(int i, char *groupPathOut, int groupCap,
	char *leafOut, int leafCap, int *texClassOut)
{
	if (i < 0 || i >= (int)s_qtTmRows.size())
	{
		copyOut("", groupPathOut, groupCap);
		copyOut("", leafOut, leafCap);
		if (texClassOut != NULL)
		{
			*texClassOut = -1;
		}
		return;
	}
	copyOut(s_qtTmRows[i].group.c_str(), groupPathOut, groupCap);
	copyOut(s_qtTmRows[i].leaf.c_str(), leafOut, leafCap);
	if (texClassOut != NULL)
	{
		*texClassOut = s_qtTmRows[i].texClass;
	}
}

extern "C" int WBQtTerrainModalData_GetInitialSelection(void)
{
	return s_qtTmInitialSel;
}

// == TerrainModal::updateLabel: the UI name's last path component.
extern "C" void WBQtTerrainModalData_GetUiNameLeaf(int texClass, char *bufOut, int cap)
{
	copyOut("", bufOut, cap);
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (pDoc == NULL || pDoc->GetHeightMap() == NULL)
	{
		return;
	}
	const char *tName = pDoc->GetHeightMap()->getTexClassUiName(texClass).str();
	if (tName == NULL)
	{
		return;
	}
	const char *leaf = tName;
	while (*tName)
	{
		if ((tName[0] == '\\' || tName[0] == '/') && tName[1])
		{
			leaf = tName + 1;
		}
		tName++;
	}
	copyOut(leaf, bufOut, cap);
}

extern "C" void WBQtTerrainModal_SetFgTexClass(int texClass)
{
	TerrainMaterial::setFgTexClass(texClass);
}

#endif // RTS_HAS_QT
