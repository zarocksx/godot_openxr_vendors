/**************************************************************************/
/*  util.cpp                                                              */
/**************************************************************************/
/*                       This file is part of:                            */
/*                              GODOT XR                                  */
/*                      https://godotengine.org                           */
/**************************************************************************/
/* Copyright (c) 2022-present Godot XR contributors (see CONTRIBUTORS.md) */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "util.h"

#include <openxr/internal/xr_linear.h>
#include <openxr/openxr.h>
#include <stdio.h>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/projection.hpp>

using namespace godot;

namespace {
bool lowercase_hexchar_to_uint8(uint8_t &out, char32_t hexchar) {
	if (U'0' <= hexchar && hexchar <= U'9') {
		out = hexchar - U'0';
		return true;
	}

	if (U'a' <= hexchar && hexchar <= U'f') {
		out = 10 + (hexchar - U'a');
		return true;
	}

	return false;
}

bool hexchars_to_uint8(uint8_t &ret, char32_t hexchar0, char32_t hexchar1) {
	// ab
	// ||
	// |hexchar1
	// hexchar0
	uint8_t d0;
	uint8_t d1;
	if (!lowercase_hexchar_to_uint8(d0, hexchar0) || !lowercase_hexchar_to_uint8(d1, hexchar1)) {
		return false;
	}

	ret = (d0 * 16) + d1;
	return true;
}

bool uuid_chunks_to_xr_uuid(const PackedStringArray &p_chunks, XrUuid &r_uuid) {
	uint8_t *data = &r_uuid.data[0];
	int data_index = 0;

	for (int chunk_index = 0; chunk_index < p_chunks.size(); chunk_index++) {
		const String chunk_string = p_chunks[chunk_index];
		const char32_t *chunk = chunk_string.ptr();

		for (int chunk_offset = 0; chunk_offset < chunk_string.length(); chunk_offset += 2) {
			if (data_index >= 16) {
				return false;
			}
			if (!hexchars_to_uint8(data[data_index], chunk[chunk_offset], chunk[chunk_offset + 1])) {
				return false;
			}
			data_index++;
		}
	}

	return data_index == 16;
}
} //namespace

StringName OpenXRUtilities::uuid_to_string_name(const XrUuid &p_uuid) {
	const uint8_t *data = p_uuid.data;
	char uuid_str[37];

	sprintf(uuid_str, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			data[0], data[1], data[2], data[3],
			data[4], data[5],
			data[6], data[7],
			data[8], data[9],
			data[10], data[11], data[12], data[13], data[14], data[15]);

	return StringName(uuid_str);
}

bool OpenXRUtilities::string_to_uuid(const godot::String &p_uuid_string, XrUuid &r_uuid) {
	// Validate p_uuid_string as a lowercase UUID of the form: "ffffffff-ffff-ffff-ffff-ffffffffffff".
	if (p_uuid_string.length() != 36) {
		return false;
	}

	PackedStringArray strs = p_uuid_string.split("-");
	if (strs.size() != 5 || strs[0].length() != 8 || strs[1].length() != 4 || strs[2].length() != 4 || strs[3].length() != 4 || strs[4].length() != 12) {
		return false;
	}

	if (!uuid_chunks_to_xr_uuid(strs, r_uuid)) {
		return false;
	}

	return true;
}

void OpenXRUtilities::xrMatrix4x4f_to_godot_projection(XrMatrix4x4f *m, godot::Projection &p) {
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			p.columns[j][i] = m->m[j * 4 + i];
		}
	}
}

Transform3D OpenXRUtilities::xrPosef_to_godot_transform3d(const XrPosef &pose) {
	Transform3D out;
	out.origin.x = pose.position.x;
	out.origin.y = pose.position.y;
	out.origin.z = pose.position.z;
	out.basis = Basis{ Quaternion{ pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w } };
	return out;
}

Vector3 OpenXRUtilities::XrVector3f_to_godot_vector3(const XrVector3f &vector) {
	Vector3 out;
	out.x = vector.x;
	out.y = vector.y;
	out.z = vector.z;
	return out;
}

XrUuid OpenXRUtilities::string_name_to_uuid(const StringName &p_uuid_str) {
	// Validate p_uuid_str as a lowercase UUID of the form: "ffffffff-ffff-ffff-ffff-ffffffffffff".
	if (p_uuid_str.length() != 36) {
		return XrUuid{};
	}

	PackedStringArray strs = p_uuid_str.split("-");
	if (strs.size() != 5 || strs[0].length() != 8 || strs[1].length() != 4 || strs[2].length() != 4 || strs[3].length() != 4 || strs[4].length() != 12) {
		return XrUuid{};
	}

	XrUuid ret;
	if (!uuid_chunks_to_xr_uuid(strs, ret)) {
		return XrUuid{};
	}

	return ret;
}
