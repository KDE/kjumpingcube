/* ****************************************************************************
  Copyright 2012 Ian Wadham <iandw.au@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************** */
#ifndef AI_GLOBALS_H
#define AI_GLOBALS_H

#define AILog 0

enum Player {Nobody, One, Two};

// Used in AI_Main and the inheritors of AI_Base (e.g. AI_Kepler, AI_Newton).
const int HighValue     = 999;
const int VeryHighValue = 9999;
const int WinnerPlus1   = 0x3fffffff;

#endif // AI_GLOBALS_H
