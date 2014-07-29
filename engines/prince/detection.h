/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef PRINCE_DETECTION_H
#define PRINCE_DETECTION_H

#include "prince/prince.h"
#include "engines/advancedDetector.h"

namespace Prince {

struct PrinceGameDescription {
	ADGameDescription desc;
	int gameType;
};

static const PlainGameDescriptor princeGames[] = {
	{"prince", "Prince Game"},
	{0, 0}
};

static const PrinceGameDescription gameDescriptions[] = {
	{
		{
			"prince",
			"Galador",
			AD_ENTRY1s("databank.ptc", "5fa03833177331214ec1354761b1d2ee", 3565031),
			Common::DE_DEU,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NONE)
		},
		0
	},
	{
		{
			"prince",
			"Ksiaze i Tchorz",
			AD_ENTRY1s("databank.ptc", "48ec9806bda9d152acbea8ce31c93c49", 3435298),
			Common::PL_POL,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GUIO_NONE)
		},
		1
	},
	{ AD_TABLE_END_MARKER, 0 }
};

// we match from data too, to stop detection from a non-top-level directory
const static char *directoryGlobs[] = {
	"all",
	0
};

class PrinceMetaEngine : public AdvancedMetaEngine {
public:
	PrinceMetaEngine() : AdvancedMetaEngine(Prince::gameDescriptions, sizeof(Prince::PrinceGameDescription), princeGames) {
		_singleid = "prince";
		_maxScanDepth = 2;
		_directoryGlobs = directoryGlobs;
	}

	virtual const char *getName() const {
		return "Prince Engine";
	}

	virtual const char *getOriginalCopyright() const {
		return "Copyright (C)";
	}

	virtual bool createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const;
	virtual bool hasFeature(MetaEngineFeature f) const;
	virtual int getMaximumSaveSlot() const;
	virtual SaveStateList listSaves(const char *target) const;
	SaveStateDescriptor querySaveMetaInfos(const char *target, int slot) const;
	virtual void removeSaveState(const char *target, int slot) const;
};

} // End of namespace Prince

#endif
