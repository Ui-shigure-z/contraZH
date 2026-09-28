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

#pragma once

// BlendMaterial.h : header file
//

#include "WBPopupSlider.h"
#include "TerrainSwatches.h"
#include "OptionsPanel.h"
class WorldHeightMapEdit;
/////////////////////////////////////////////////////////////////////////////
// BlendMaterial dialog

class BlendMaterial : public COptionsPanel
{
// Construction
public:
	BlendMaterial(CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(BlendMaterial)
	enum { IDD = IDD_BLEND_MATERIAL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(BlendMaterial)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual void OnOK() override {return;};  ///< Modeless dialogs don't OK, so eat this for modeless.
	virtual void OnCancel() override {return;}; ///< Modeless dialogs don't close on ESC, so eat this for modeless.
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
	//}}AFX_VIRTUAL

// Implementation
protected:
	enum {MIN_TILE_SIZE=2, MAX_TILE_SIZE = 100};
	// Generated message map functions
	//{{AFX_MSG(BlendMaterial)
	virtual BOOL OnInitDialog() override;
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


protected:
	static BlendMaterial	*m_staticThis;
	Bool										m_updating;
	static Int							m_currentBlendTexture;
	CTreeCtrl								m_terrainTreeView;
	static Bool  m_hvgap; //horizontal+vertical gap
	static Bool  m_dgap;  //diagonal gap
	static Bool  m_revalblends;

protected:
	void updateTextures();
	void addTerrain(const char *pPath, Int terrainNdx, HTREEITEM parent);
	HTREEITEM findOrAdd(HTREEITEM parent, const char *pLabel);

	afx_msg void OnReevaluateBlends();
	afx_msg void OnHorizontalAndVerticalGap();
	afx_msg void OnDiagonalGap();

	afx_msg void OnToggleMirror();
	afx_msg void OnToggleMirrorX();
	afx_msg void OnToggleMirrorY();
	afx_msg void OnToggleMirrorXY();
	

public:
	static void updateBlendPointerToolTip();
	static Bool isHorizVertGap(void) {return m_hvgap;}
	static Bool isDiagGap(void) {return m_dgap;}
	static Bool isRevalBlends(void) {return m_revalblends;}

	static Int getBlendTexClass(void) {return m_currentBlendTexture;}
	static void setBlendTexClass(Int texClass);

#ifdef RTS_HAS_QT
	// Qt panel support (WBQtBlendMaterialBridge): let the Qt Blend Material panel drive the
	// gap statics the same way the MFC On* handlers do (set the flag, refresh the tooltip).
	// Defined in src/WBQtBlendMaterialBridge.cpp; static so they can reach the protected state.
	static void qtSetHorizVertGap(Bool on);
	static void qtSetDiagGap(Bool on);
	static void qtSetRevalBlends(Bool on);
#endif

public:
	Bool setTerrainTreeViewSelection(HTREEITEM parent, Int selection);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
