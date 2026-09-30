// WBQtStochasticBridge.cpp -- the MFC side of the Qt Stochastic Terrain panel seam. See
// WBQtBrushBridge.cpp for the pattern. Guarded by RTS_HAS_QT so the non-Qt build compiles it empty.
#include "StdAfx.h"
#include "StochasticTool.h"
#include "qt/panels/WBQtStochasticBridge.h"

#ifdef RTS_HAS_QT
extern "C" {

void WBQtStochastic_SetWidth(int v)      { StochasticTool::setWidth(v); }
void WBQtStochastic_SetFeather(int v)    { StochasticTool::setFeather(v); }
void WBQtStochastic_SetSeed(int v)       { StochasticTool::setSeed(v); }
void WBQtStochastic_SetRate(int v)       { StochasticTool::setRate(v); }
void WBQtStochastic_RandomizeSeed(void)  { StochasticTool::randomizeSeed(); }

int WBQtStochastic_GetWidth(void)    { return StochasticTool::getWidth(); }
int WBQtStochastic_GetFeather(void)  { return StochasticTool::getFeather(); }
int WBQtStochastic_GetSeed(void)     { return StochasticTool::getSeed(); }
int WBQtStochastic_GetRate(void)     { return StochasticTool::getRate(); }

}
#endif
