/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// StochasticTool.cpp
// Paints the hex-cell texture breakup the D3D9 terrain shader draws to hide tiling.

#include "StdAfx.h"
#include "resource.h"

#include "StochasticTool.h"
#include "CUndoable.h"
#include "MainFrm.h"
#include "WHeightMapEdit.h"
#include "WorldBuilderDoc.h"
#include "WorldBuilderView.h"
#include "DrawObject.h"
#include "WorldBuilder.h"
#include "wbview3d.h"
#include "qt/panels/WBQtStochasticBridge.h"

Int StochasticTool::m_width = 8;
Int StochasticTool::m_feather = 4;
Int StochasticTool::m_seed = 0;
Int StochasticTool::m_rate = 50;
StochasticTool *StochasticTool::m_staticThis = nullptr;
Coord3D StochasticTool::m_cursor;
Bool StochasticTool::m_cursorValid = false;

StochasticTool::StochasticTool() :
	Tool(ID_STOCHASTIC_TOOL, IDC_BRUSH_CROSS)
{
	m_htMapEditCopy = nullptr;
	m_strokeSeed = 0;
	m_staticThis = this;
}

StochasticTool::~StochasticTool()
{
	REF_PTR_RELEASE(m_htMapEditCopy);
	if (m_staticThis == this)
	{
		m_staticThis = nullptr;
	}
}

// Painting raises a point to the stroke's amount and stamps its look, including where the point already sits at that
// amount, such as full paint under the brush's core.
Bool StochasticTool::takesStroke(Int amount, Int strength)
{
	return amount > strength || (amount == strength && amount > 0);
}

// The overlay follows the width and feather, so a change redraws it without waiting for the mouse.
void StochasticTool::redrawOverlay()
{
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	WbView3d *pView = (pDoc != nullptr) ? pDoc->Get3DView() : nullptr;
	if (pView != nullptr)
	{
		pView->Invalidate(false);
	}
}

Bool StochasticTool::getBrushOverlay(Coord3D &center, Real &coreRadius, Real &outerRadius)
{
	// deactivate() is not reliably called, so the overlay asks whether this is still the selected tool.
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (!m_cursorValid || m_staticThis == nullptr || pDoc == nullptr || pDoc->GetHeightMap() == nullptr ||
		WbApp() == nullptr || WbApp()->getSelTool() != (Tool *)m_staticThis)
	{
		return false;
	}

	// The centre and radii calcRoundBlendFactor paints with: odd widths centre on a cell, even ones on a point.
	CPoint ndx;
	getCenterIndex(&m_cursor, m_width, &ndx, pDoc);
	const Real offset = (m_width & 1) ? 0.5f : 0.0f;
	const Int border = pDoc->GetHeightMap()->getBorderSize();
	center.x = (ndx.x + offset - border) * MAP_XY_FACTOR;
	center.y = (ndx.y + offset - border) * MAP_XY_FACTOR;
	center.z = 0.0f;
	coreRadius = (m_width / 2.0f) * MAP_XY_FACTOR;
	outerRadius = (m_width / 2.0f + m_feather) * MAP_XY_FACTOR;
	return true;
}

Bool StochasticTool::overlayStampsSeedAt(Int xIndex, Int yIndex)
{
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (!m_cursorValid || m_staticThis == nullptr || pDoc == nullptr || (0x8000 & ::GetAsyncKeyState(VK_SHIFT)) != 0)
	{
		return false;
	}
	// Mid-stroke the tool's own copy holds what the stroke has painted so far.
	WorldHeightMapEdit *pMap = (m_staticThis->m_htMapEditCopy != nullptr) ? m_staticThis->m_htMapEditCopy : pDoc->GetHeightMap();
	if (pMap == nullptr || xIndex < 0 || yIndex < 0 || xIndex >= pMap->getXExtent() || yIndex >= pMap->getYExtent())
	{
		return false;
	}
	CPoint ndx;
	getCenterIndex(&m_cursor, m_width, &ndx, pDoc);
	const Real blend = calcRoundBlendFactor(ndx, xIndex, yIndex, m_width, m_feather);
	if (blend <= 0.0f)
	{
		return false;
	}
	UnsignedByte strength, seed, rate;
	pMap->getStochastic(xIndex, yIndex, strength, seed, rate);
	return takesStroke(REAL_TO_INT_FLOOR(blend * 255.0f + 0.5f), strength);
}

void StochasticTool::setWidth(Int width)
{
	if (m_width != width)
	{
		m_width = width;
#ifdef RTS_HAS_QT
		WBQtStochastic_PushWidth(width);
#endif
		DrawObject::setBrushFeedbackParms(false, m_width, m_feather);
		redrawOverlay();
	}
}

void StochasticTool::setFeather(Int feather)
{
	if (m_feather != feather)
	{
		m_feather = feather;
#ifdef RTS_HAS_QT
		WBQtStochastic_PushFeather(feather);
#endif
		DrawObject::setBrushFeedbackParms(false, m_width, m_feather);
		redrawOverlay();
	}
}

void StochasticTool::setSeed(Int seed)
{
	seed = max(0, min(seed, (Int)MAX_SEED));
	if (m_seed != seed)
	{
		m_seed = seed;
#ifdef RTS_HAS_QT
		WBQtStochastic_PushSeed(seed);
#endif
	}
}

void StochasticTool::setRate(Int rate)
{
	rate = max(0, min(rate, (Int)MAX_RATE));
	if (m_rate != rate)
	{
		m_rate = rate;
#ifdef RTS_HAS_QT
		WBQtStochastic_PushRate(rate);
#endif
	}
}

void StochasticTool::randomizeSeed()
{
	// Never picks RANDOM_SEED, which would switch to a new seed per stroke.
	setSeed(1 + rand() % MAX_SEED);
}

void StochasticTool::activate()
{
	CMainFrame::GetMainFrame()->showOptionsDialog(IDD_STOCHASTIC_OPTIONS);
	// The tool draws its own overlay, see DrawObject::drawStochasticBrushFeedback, so the shared brush grid stays off.
	DrawObject::setDoBrushFeedback(false);
	DrawObject::setBrushFeedbackParms(false, m_width, m_feather);
}

void StochasticTool::abandonStroke(void)
{
	if (m_htMapEditCopy == nullptr)
	{
		return;
	}
	REF_PTR_RELEASE(m_htMapEditCopy);
	revertAbandonedPreview();
}

void StochasticTool::mouseDown(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc)
{
	if (m != TRACK_L)
	{
		return;
	}
	REF_PTR_RELEASE(m_htMapEditCopy);
	m_htMapEditCopy = pDoc->GetHeightMap()->duplicate();
	// Seed 0 is what unpainted cells read, so a random stroke picks from the others.
	m_strokeSeed = (m_seed == RANDOM_SEED) ? 1 + rand() % MAX_SEED : m_seed;
	m_prevXIndex = -1;
	m_prevYIndex = -1;
	mouseMoved(m, viewPt, pView, pDoc);
}

void StochasticTool::mouseUp(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc)
{
	if (m != TRACK_L || m_htMapEditCopy == nullptr)
	{
		return;
	}
	WBDocUndoable *pUndo = new WBDocUndoable(pDoc, m_htMapEditCopy);
	pDoc->AddAndDoUndoable(pUndo);
	REF_PTR_RELEASE(pUndo);
	REF_PTR_RELEASE(m_htMapEditCopy);
}

void StochasticTool::mouseMoved(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc)
{
	Coord3D cpt;
	pView->viewToDocCoords(viewPt, &cpt);
	DrawObject::setFeedbackPos(cpt);
	m_cursor = cpt;
	m_cursorValid = true;
	pView->Invalidate();
	pDoc->updateAllViews();
	if (m != TRACK_L || m_htMapEditCopy == nullptr)
	{
		return;
	}

	CPoint ndx;
	getCenterIndex(&cpt, m_width, &ndx, pDoc);
	if (m_prevXIndex == ndx.x && m_prevYIndex == ndx.y)
	{
		return;
	}
	m_prevXIndex = ndx.x;
	m_prevYIndex = ndx.y;

	const Bool erase = (0x8000 & ::GetAsyncKeyState(VK_SHIFT)) != 0;
	const UnsignedByte rate = (UnsignedByte)((m_rate * 255 + MAX_RATE / 2) / MAX_RATE);
	const Int reach = m_width + 2 * m_feather + 2;
	const Int sub = reach / 2;
	const Int add = reach - sub;

	for (Int i = ndx.x - sub; i < ndx.x + add; i++)
	{
		for (Int j = ndx.y - sub; j < ndx.y + add; j++)
		{
			if (i < 0 || j < 0 || i >= m_htMapEditCopy->getXExtent() || j >= m_htMapEditCopy->getYExtent())
			{
				continue;
			}
			const Real blend = calcRoundBlendFactor(ndx, i, j, m_width, m_feather);
			if (blend <= 0.0f)
			{
				continue;
			}
			UnsignedByte strength, seed, oldRate;
			m_htMapEditCopy->getStochastic(i, j, strength, seed, oldRate);

			// Painting only raises a cell, which then takes this stroke's look (see takesStroke); erasing only lowers it.
			const Int amount = REAL_TO_INT_FLOOR(blend * 255.0f + 0.5f);
			if (erase)
			{
				const Int lowered = min((Int)strength, 255 - amount);
				if (lowered < strength)
				{
					m_htMapEditCopy->setStochastic(i, j, (UnsignedByte)lowered, seed, oldRate);
				}
			}
			else if (takesStroke(amount, strength) && (amount != strength || seed != m_strokeSeed || oldRate != rate))
			{
				m_htMapEditCopy->setStochastic(i, j, (UnsignedByte)amount, (UnsignedByte)m_strokeSeed, rate);
			}
		}
	}

	IRegion2D partialRange;
	partialRange.lo.x = ndx.x - reach;
	partialRange.hi.x = ndx.x + reach;
	partialRange.lo.y = ndx.y - reach;
	partialRange.hi.y = ndx.y + reach;
	pDoc->updateHeightMap(m_htMapEditCopy, true, partialRange);
}
