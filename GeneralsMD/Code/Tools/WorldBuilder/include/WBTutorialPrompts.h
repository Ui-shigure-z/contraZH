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

// WBTutorialPrompts.h -- the "tutorial prompts" switch (one-time hint toasts, the Ctrl+A
// whole-map confirm). It lives in WorldBuilder.ini, not in Qt, so the classic MFC build reads
// it too; the Qt Object panel's checkbox is just one more client of the same pair. C linkage
// like the rest of the bridge seam (qt/WBQtPanelBridge.h declares the same two).

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void WBQtObject_SetTutorialPrompts(int on);
int  WBQtObject_GetTutorialPrompts(void);

#ifdef __cplusplus
}
#endif
