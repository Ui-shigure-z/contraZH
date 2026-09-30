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
#include "qt/panels/WBQtStochasticBridge.h"

Int StochasticTool::m_width = 8;
Int StochasticTool::m_feather = 4;
Int StochasticTool::m_seed = 0;
Int StochasticTool::m_rate = 50;

StochasticTool::StochasticTool() :
	Tool(ID_STOCHASTIC_TOOL, IDC_BRUSH_CROSS)
{
	m_htMapEditCopy = nullptr;
}

StochasticTool::~StochasticTool()
{
	REF_PTR_RELEASE(m_htMapEditCopy);
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
	setSeed(rand() % (MAX_SEED + 1));
}

void StochasticTool::activate()
{
	CMainFrame::GetMainFrame()->showOptionsDialog(IDD_STOCHASTIC_OPTIONS);
	DrawObject::setDoBrushFeedback(true);
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

			// Painting only raises a cell, which then takes this stroke's look; erasing only lowers it.
			const Int amount = REAL_TO_INT_FLOOR(blend * 255.0f + 0.5f);
			if (erase)
			{
				const Int lowered = min((Int)strength, 255 - amount);
				if (lowered < strength)
				{
					m_htMapEditCopy->setStochastic(i, j, (UnsignedByte)lowered, seed, oldRate);
				}
			}
			else if (amount > strength)
			{
				m_htMapEditCopy->setStochastic(i, j, (UnsignedByte)amount, (UnsignedByte)m_seed, rate);
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
