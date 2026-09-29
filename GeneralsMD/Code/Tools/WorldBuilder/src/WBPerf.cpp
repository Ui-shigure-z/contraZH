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

#include "StdAfx.h"
#include "WBPerf.h"

#include <stdio.h>
#include <string.h>

WBPerf::Section WBPerf::s_sections[WBPerf::MAX_SECTIONS];
Int WBPerf::s_numSections = 0;
Int WBPerf::s_frames = 0;
Bool WBPerf::s_enabled = false;

void WBPerf::setEnabled(Bool on)
{
	s_enabled = on;
	s_numSections = 0;
	s_frames = 0;
	if (on) {
		DEBUG_LOG(("WBPerf: enabled, summary every %d frames", (int)REPORT_FRAMES));
	}
}

double WBPerf::nowMs()
{
	static LARGE_INTEGER freq = { 0 };
	if (freq.QuadPart == 0) {
		::QueryPerformanceFrequency(&freq);
	}
	LARGE_INTEGER now;
	::QueryPerformanceCounter(&now);
	return (double)now.QuadPart * 1000.0 / (double)freq.QuadPart;
}

void WBPerf::addSection(const char *name, double ms)
{
	for (Int i = 0; i < s_numSections; ++i) {
		if (strcmp(s_sections[i].name, name) == 0) {
			s_sections[i].frameMs += ms;
			return;
		}
	}
	if (s_numSections < MAX_SECTIONS) {
		Section &s = s_sections[s_numSections++];
		s.name = name;
		s.frameMs = ms;
		s.totalMs = 0.0;
		s.maxMs = 0.0;
	}
}

void WBPerf::frameEnd()
{
	if (!s_enabled) {
		return;
	}
	for (Int i = 0; i < s_numSections; ++i) {
		Section &s = s_sections[i];
		s.totalMs += s.frameMs;
		if (s.frameMs > s.maxMs) {
			s.maxMs = s.frameMs;
		}
		s.frameMs = 0.0;
	}
	if (++s_frames < REPORT_FRAMES) {
		return;
	}

	char line[1024];
	int len = _snprintf(line, sizeof(line), "WBPerf: %d frames", (int)s_frames);
	for (Int i = 0; i < s_numSections && len > 0 && len < (int)sizeof(line); ++i) {
		Section &s = s_sections[i];
		len += _snprintf(line + len, sizeof(line) - len, " | %s avg %.2f max %.2f",
			s.name, s.totalMs / s_frames, s.maxMs);
		s.totalMs = 0.0;
		s.maxMs = 0.0;
	}
	line[sizeof(line) - 1] = 0;
	DEBUG_LOG(("%s", line));
	s_frames = 0;
}

void WBPerf::logEvent(const char *name, double ms)
{
	if (!s_enabled) {
		return;
	}
	DEBUG_LOG(("WBPerf: %s %.2f ms", name, ms));
}
