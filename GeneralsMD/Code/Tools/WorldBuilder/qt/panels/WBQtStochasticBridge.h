// WBQtStochasticBridge.h -- opaque facade for the Qt Stochastic Terrain panel.
//
// Carries only int (no Qt or MFC types), so StochasticTool can drive the panel and the panel can
// drive StochasticTool without either side including the other's headers.
#ifndef WB_QT_STOCHASTIC_BRIDGE_H
#define WB_QT_STOCHASTIC_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// --- Forward: tool -> Qt widget (implemented Qt-side, WBQtStochasticPanel.cpp) ---------------
void WBQtStochastic_PushWidth(int v);
void WBQtStochastic_PushFeather(int v);
void WBQtStochastic_PushSeed(int v);
void WBQtStochastic_PushRate(int v);

// --- Reverse: Qt widget -> tool (implemented MFC-side, WBQtStochasticBridge.cpp) -------------
void WBQtStochastic_SetWidth(int v);
void WBQtStochastic_SetFeather(int v);
void WBQtStochastic_SetSeed(int v);
void WBQtStochastic_SetRate(int v);
void WBQtStochastic_RandomizeSeed(void);
int  WBQtStochastic_GetWidth(void);
int  WBQtStochastic_GetFeather(void);
int  WBQtStochastic_GetSeed(void);
int  WBQtStochastic_GetRate(void);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_STOCHASTIC_BRIDGE_H
