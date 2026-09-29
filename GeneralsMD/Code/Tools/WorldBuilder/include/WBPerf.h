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

// Frame-time measurement for the editor, on when WorldBuilder.ini has PerfLog=1
// under [MainFrame]. Scoped sections accumulate per frame and a summary line goes
// to the debug log every 300 frames; one-off events log as they happen.

#ifndef __WB_PERF_H_
#define __WB_PERF_H_

#include "Lib/BaseType.h"

class WBPerf
{
public:
	static void setEnabled(Bool on);
	static Bool isEnabled() { return s_enabled; }

	static double nowMs();
	static void addSection(const char *name, double ms);	///< accumulate into this frame's summary
	static void frameEnd();									///< close the frame; logs every 300th
	static void logEvent(const char *name, double ms);		///< immediate "name took ms" line

private:
	enum { MAX_SECTIONS = 16, REPORT_FRAMES = 300 };
	struct Section
	{
		const char *name;
		double frameMs;		///< this frame so far
		double totalMs;		///< since the last report
		double maxMs;		///< worst frame since the last report
	};
	static Section s_sections[MAX_SECTIONS];
	static Int s_numSections;
	static Int s_frames;
	static Bool s_enabled;
};

// Adds the enclosing scope's time to a section.
class WBPerfScope
{
public:
	WBPerfScope(const char *name) : m_name(name), m_start(WBPerf::isEnabled() ? WBPerf::nowMs() : 0.0) {}
	~WBPerfScope()
	{
		if (WBPerf::isEnabled()) {
			WBPerf::addSection(m_name, WBPerf::nowMs() - m_start);
		}
	}
private:
	const char *m_name;
	double m_start;
};

// Logs the enclosing scope's time as an event.
class WBPerfEvent
{
public:
	WBPerfEvent(const char *name) : m_name(name), m_start(WBPerf::isEnabled() ? WBPerf::nowMs() : 0.0) {}
	~WBPerfEvent()
	{
		if (WBPerf::isEnabled()) {
			WBPerf::logEvent(m_name, WBPerf::nowMs() - m_start);
		}
	}
private:
	const char *m_name;
	double m_start;
};

#endif // __WB_PERF_H_
