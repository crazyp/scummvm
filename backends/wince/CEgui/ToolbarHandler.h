/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001-2004 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#ifndef CEGUI_TOOLBARHANDLER
#define CEGUI_TOOLBARHANDLER

#include "common/stdafx.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/str.h"
#include "common/map.h"

#include "Toolbar.h"

using Common::String;
using Common::Map;

namespace CEGUI {

	class ToolbarHandler {
	public:
		ToolbarHandler();
		bool add(const String &name, const Toolbar &toolbar);
		bool setActive(const String &name);
		bool action(int x, int y, bool pushed);
		void setVisible(bool visible);
		bool visible();
		String activeName();
		void forceRedraw();
		bool draw(SDL_Surface *surface, SDL_Rect *rect);
		bool drawn();
		Toolbar *active();
		virtual ~ToolbarHandler();
	private:

		struct IgnoreCaseComparator {
          int operator()(const String& x, const String& y) const { 
			  return scumm_stricmp(x.c_str(), y.c_str()); }
        };

		Map<String, Toolbar*, IgnoreCaseComparator> _toolbarMap;
		String _current;
		Toolbar *_active;
	};
}

#endif