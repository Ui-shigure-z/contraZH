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

// StochasticTool.h
// Paints the hex-cell texture breakup the D3D9 terrain shader draws to hide tiling.

#pragma once

#include "Tool.h"
class WorldHeightMapEdit;

/*************************************************************************/
/**                             StochasticTool
	 Paints stochastic terrain: strength with a feathered edge, and the
	 stroke's seed and blending rate where the strength rises. Shift erases.
***************************************************************************/
class StochasticTool : public Tool
{
protected:
	WorldHeightMapEdit *m_htMapEditCopy; ///< ref counted.

	static Int m_width;
	static Int m_feather;
	static Int m_seed;
	static Int m_rate;

public:
	enum { MAX_SEED = 255, MAX_RATE = 100 };

	StochasticTool();
	virtual ~StochasticTool() override;

public:
	static Int getWidth() {return m_width;}
	static Int getFeather() {return m_feather;}
	static Int getSeed() {return m_seed;}
	static Int getRate() {return m_rate;}
	static void setWidth(Int width);
	static void setFeather(Int feather);
	static void setSeed(Int seed);
	static void setRate(Int rate);
	static void randomizeSeed();

public:
	virtual void mouseDown(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc) override;
	virtual void mouseUp(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc) override;
	virtual void mouseMoved(TTrackingMode m, CPoint viewPt, WbView* pView, CWorldBuilderDoc *pDoc) override;
	virtual WorldHeightMapEdit *getHeightMap(void) override {return m_htMapEditCopy;}
	virtual void activate() override;
	virtual void abandonStroke(void) override;
	virtual Bool followsTerrain(void) override {return false;}
};
