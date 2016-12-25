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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef GOB_PREGOB_ONCEUPON_PALETTES_H
#define GOB_PREGOB_ONCEUPON_PALETTES_H

static const  int kPaletteSize  = 16;
static const uint kPaletteCount = 20;

static const byte kCopyProtectionPalette[3 * kPaletteSize] = {
	0x00, 0x00, 0x00,
	0x19, 0x00, 0x19,
	0x00, 0x3F, 0x00,
	0x00, 0x2A, 0x2A,
	0x2A, 0x00, 0x00,
	0x2A, 0x00, 0x2A,
	0x2A, 0x15, 0x00,
	0x00, 0x19, 0x12,
	0x00, 0x00, 0x00,
	0x15, 0x15, 0x3F,
	0x15, 0x3F, 0x15,
	0x00, 0x20, 0x3F,
	0x3F, 0x00, 0x00,
	0x3F, 0x00, 0x20,
	0x3F, 0x3F, 0x00,
	0x3F, 0x3F, 0x3F
};

static const byte kGamePalettes[kPaletteCount][3 * kPaletteSize] = {
	{
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x10,
		0x00, 0x00, 0x18,
		0x00, 0x00, 0x3C,
		0x1C, 0x28, 0x00,
		0x10, 0x18, 0x00,
		0x1C, 0x1C, 0x20,
		0x14, 0x14, 0x14,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x14, 0x20, 0x04,
		0x3C, 0x2C, 0x00,
		0x02, 0x00, 0x18,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x38, 0x20, 0x3C,
		0x2C, 0x10, 0x30,
		0x20, 0x08, 0x28,
		0x14, 0x00, 0x1C,
		0x20, 0x20, 0x38,
		0x18, 0x18, 0x2C,
		0x10, 0x10, 0x24,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x3C, 0x20, 0x20,
		0x24, 0x14, 0x14,
		0x1C, 0x10, 0x10,
		0x14, 0x0C, 0x0C,
		0x1C, 0x1C, 0x1C,
		0x18, 0x18, 0x18,
		0x10, 0x10, 0x10,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x10, 0x28, 0x1C,
		0x10, 0x1C, 0x10,
		0x10, 0x14, 0x0C,
		0x1C, 0x1C, 0x3C,
		0x24, 0x24, 0x3C,
		0x18, 0x18, 0x24,
		0x10, 0x10, 0x18,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x3F, 0x26, 0x3F,
		0x36, 0x1C, 0x36,
		0x2C, 0x12, 0x2A,
		0x27, 0x0C, 0x24,
		0x22, 0x07, 0x1E,
		0x1D, 0x03, 0x18,
		0x16, 0x00, 0x10,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3A,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x3F, 0x39, 0x26,
		0x38, 0x34, 0x1C,
		0x30, 0x2F, 0x13,
		0x27, 0x29, 0x0C,
		0x1D, 0x22, 0x07,
		0x14, 0x1B, 0x03,
		0x0C, 0x14, 0x00,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3A,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x24, 0x3C, 0x3C,
		0x1C, 0x34, 0x38,
		0x14, 0x2C, 0x30,
		0x0C, 0x20, 0x2C,
		0x08, 0x18, 0x28,
		0x04, 0x10, 0x20,
		0x00, 0x08, 0x1C,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x38,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x24,
		0x38, 0x24, 0x1C,
		0x30, 0x1C, 0x14,
		0x28, 0x18, 0x0C,
		0x20, 0x10, 0x04,
		0x1C, 0x0C, 0x00,
		0x14, 0x08, 0x00,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x38,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x3C, 0x34, 0x24,
		0x38, 0x2C, 0x1C,
		0x30, 0x24, 0x14,
		0x2C, 0x1C, 0x10,
		0x30, 0x30, 0x3C,
		0x1C, 0x1C, 0x38,
		0x0C, 0x0C, 0x38,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x0C,
		0x02, 0x03, 0x14,
		0x07, 0x07, 0x1D,
		0x0E, 0x0E, 0x25,
		0x17, 0x17, 0x2E,
		0x21, 0x22, 0x36,
		0x2F, 0x2F, 0x3F,
		0x3F, 0x3F, 0x3F,
		0x3F, 0x3B, 0x0D,
		0x3A, 0x31, 0x0A,
		0x35, 0x28, 0x07,
		0x30, 0x21, 0x04,
		0x2B, 0x19, 0x02,
		0x26, 0x12, 0x01,
		0x16, 0x0B, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x18, 0x00, 0x00,
		0x21, 0x01, 0x00,
		0x2A, 0x02, 0x00,
		0x33, 0x03, 0x00,
		0x3D, 0x06, 0x00,
		0x2A, 0x19, 0x05,
		0x15, 0x14, 0x14,
		0x22, 0x1F, 0x1E,
		0x2F, 0x2C, 0x28,
		0x3F, 0x3C, 0x29,
		0x3F, 0x38, 0x0B,
		0x3B, 0x30, 0x0A,
		0x37, 0x29, 0x08,
		0x33, 0x23, 0x07,
		0x2F, 0x1D, 0x06
	},
	{
		0x00, 0x00, 0x00,
		0x00, 0x1C, 0x38,
		0x34, 0x30, 0x28,
		0x2C, 0x24, 0x1C,
		0x24, 0x18, 0x10,
		0x1C, 0x10, 0x08,
		0x14, 0x04, 0x04,
		0x10, 0x00, 0x00,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x38,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x00, 0x1C, 0x38,
		0x34, 0x30, 0x28,
		0x2C, 0x24, 0x1C,
		0x3F, 0x3F, 0x3F,
		0x3F, 0x3F, 0x3F,
		0x3F, 0x3F, 0x3F,
		0x3F, 0x3F, 0x3F,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x38,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x1A, 0x30, 0x37,
		0x14, 0x28, 0x31,
		0x10, 0x20, 0x2C,
		0x0C, 0x19, 0x27,
		0x08, 0x12, 0x21,
		0x05, 0x0C, 0x1C,
		0x03, 0x07, 0x16,
		0x01, 0x03, 0x11,
		0x00, 0x00, 0x0C,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x34, 0x30, 0x34,
		0x30, 0x24, 0x30,
		0x28, 0x1C, 0x28,
		0x24, 0x14, 0x24,
		0x1C, 0x0C, 0x1C,
		0x18, 0x08, 0x18,
		0x14, 0x04, 0x14,
		0x0C, 0x04, 0x0C,
		0x08, 0x00, 0x08,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x2C, 0x24, 0x0C,
		0x34, 0x34, 0x28,
		0x2C, 0x2C, 0x1C,
		0x24, 0x24, 0x10,
		0x1C, 0x18, 0x08,
		0x14, 0x14, 0x08,
		0x10, 0x10, 0x04,
		0x0C, 0x0C, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x38,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
		0x14, 0x28, 0x31,
		0x10, 0x20, 0x2C,
		0x0C, 0x19, 0x27,
		0x08, 0x12, 0x21,
		0x05, 0x0C, 0x1C,
		0x03, 0x07, 0x16,
		0x01, 0x03, 0x11,
		0x00, 0x3C, 0x00,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x10, 0x28, 0x1C,
		0x10, 0x1C, 0x10,
		0x10, 0x14, 0x0C,
		0x1C, 0x1C, 0x3C,
		0x24, 0x24, 0x3C,
		0x18, 0x18, 0x24,
		0x10, 0x10, 0x18,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	},
	{
		0x00, 0x00, 0x00,
		0x10, 0x28, 0x1C,
		0x10, 0x1C, 0x10,
		0x10, 0x14, 0x0C,
		0x1C, 0x1C, 0x3C,
		0x24, 0x24, 0x3C,
		0x18, 0x18, 0x24,
		0x10, 0x10, 0x18,
		0x14, 0x20, 0x04,
		0x00, 0x00, 0x24,
		0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00,
		0x3C, 0x2C, 0x00,
		0x3C, 0x18, 0x00,
		0x3C, 0x04, 0x00,
		0x1C, 0x00, 0x00
	}
};

#endif // GOB_PREGOB_ONCEUPON_PALETTES_H
