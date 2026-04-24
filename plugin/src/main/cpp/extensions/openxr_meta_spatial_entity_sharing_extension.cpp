/**************************************************************************/
/*  openxr_meta_spatial_entity_sharing_extension.cpp                      */
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

#include "extensions/openxr_meta_spatial_entity_sharing_extension.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/open_xrapi_extension.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

OpenXRMetaSpatialEntitySharingExtension *OpenXRMetaSpatialEntitySharingExtension::singleton = nullptr;

OpenXRMetaSpatialEntitySharingExtension *OpenXRMetaSpatialEntitySharingExtension::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(OpenXRMetaSpatialEntitySharingExtension());
	}
	return singleton;
}

OpenXRMetaSpatialEntitySharingExtension::OpenXRMetaSpatialEntitySharingExtension() :
		OpenXRExtensionWrapper() {
	ERR_FAIL_COND_MSG(singleton != nullptr, "An OpenXRMetaSpatialEntitySharingExtension singleton already exists.");

	request_extensions[XR_META_SPATIAL_ENTITY_SHARING_EXTENSION_NAME] = &meta_spatial_entity_sharing_ext;
	request_extensions[XR_META_SPATIAL_ENTITY_GROUP_SHARING_EXTENSION_NAME] = &meta_spatial_entity_group_sharing_ext;
	singleton = this;
}

OpenXRMetaSpatialEntitySharingExtension::~OpenXRMetaSpatialEntitySharingExtension() {
	cleanup();
	singleton = nullptr;
}

void OpenXRMetaSpatialEntitySharingExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_spatial_entity_sharing_supported"), &OpenXRMetaSpatialEntitySharingExtension::is_spatial_entity_sharing_supported);
	ClassDB::bind_method(D_METHOD("is_spatial_entity_group_sharing_supported"), &OpenXRMetaSpatialEntitySharingExtension::is_spatial_entity_group_sharing_supported);
}

void OpenXRMetaSpatialEntitySharingExtension::cleanup() {
	meta_spatial_entity_sharing_ext = false;
	meta_spatial_entity_group_sharing_ext = false;
	system_sharing_properties.supportsSpatialEntitySharing = XR_FALSE;
	system_group_sharing_properties.supportsSpatialEntityGroupSharing = XR_FALSE;
	requests.clear();
}

uint64_t OpenXRMetaSpatialEntitySharingExtension::_set_system_properties_and_get_next_pointer(void *p_next_pointer) {
	if (meta_spatial_entity_group_sharing_ext) {
		system_group_sharing_properties.type = XR_TYPE_SYSTEM_SPATIAL_ENTITY_GROUP_SHARING_PROPERTIES_META;
		system_group_sharing_properties.next = p_next_pointer;
		system_group_sharing_properties.supportsSpatialEntityGroupSharing = XR_FALSE;
		p_next_pointer = &system_group_sharing_properties;
	}

	if (meta_spatial_entity_sharing_ext) {
		system_sharing_properties.type = XR_TYPE_SYSTEM_SPATIAL_ENTITY_SHARING_PROPERTIES_META;
		system_sharing_properties.next = p_next_pointer;
		system_sharing_properties.supportsSpatialEntitySharing = XR_FALSE;
		p_next_pointer = &system_sharing_properties;
	}

	return reinterpret_cast<uint64_t>(p_next_pointer);
}

Dictionary OpenXRMetaSpatialEntitySharingExtension::_get_requested_extensions(uint64_t p_xr_version) {
	Dictionary result;
	for (auto ext : request_extensions) {
		uint64_t value = reinterpret_cast<uint64_t>(ext.value);
		result[ext.key] = (Variant)value;
	}
	return result;
}

void OpenXRMetaSpatialEntitySharingExtension::_on_instance_created(uint64_t instance) {
	if (!meta_spatial_entity_sharing_ext) {
		return;
	}

	if (!initialize_meta_spatial_entity_sharing_extension((XrInstance)instance)) {
		UtilityFunctions::printerr("Failed to initialize meta_spatial_entity_sharing extension");
		meta_spatial_entity_sharing_ext = false;
	}
}

void OpenXRMetaSpatialEntitySharingExtension::_on_instance_destroyed() {
	cleanup();
}

bool OpenXRMetaSpatialEntitySharingExtension::initialize_meta_spatial_entity_sharing_extension(const XrInstance &p_instance) {
	GDEXTENSION_INIT_XR_FUNC_V(xrShareSpacesMETA);

	return true;
}

bool OpenXRMetaSpatialEntitySharingExtension::is_spatial_entity_sharing_supported() const {
	return meta_spatial_entity_sharing_ext && system_sharing_properties.supportsSpatialEntitySharing;
}

bool OpenXRMetaSpatialEntitySharingExtension::is_spatial_entity_group_sharing_supported() const {
	return is_spatial_entity_sharing_supported() &&
			meta_spatial_entity_group_sharing_ext &&
			system_group_sharing_properties.supportsSpatialEntityGroupSharing;
}

bool OpenXRMetaSpatialEntitySharingExtension::_on_event_polled(const void *event) {
	if (static_cast<const XrEventDataBuffer *>(event)->type == XR_TYPE_EVENT_DATA_SHARE_SPACES_COMPLETE_META) {
		on_share_spaces_complete((const XrEventDataShareSpacesCompleteMETA *)event);
		return true;
	}

	return false;
}

bool OpenXRMetaSpatialEntitySharingExtension::share_spaces(const XrShareSpacesInfoMETA *p_info, ShareSpacesCompleteCallback p_callback, void *p_userdata) {
	if (!is_spatial_entity_sharing_supported()) {
		p_callback(XR_ERROR_FEATURE_UNSUPPORTED, p_userdata);
		return false;
	}

	XrAsyncRequestIdFB request_id = 0;
	const XrResult result = xrShareSpacesMETA(SESSION, p_info, &request_id);
	if (!XR_SUCCEEDED(result)) {
		WARN_PRINT("xrShareSpacesMETA failed!");
		WARN_PRINT(get_openxr_api()->get_error_string(result));
		p_callback(result, p_userdata);
		return false;
	}

	requests[request_id] = RequestInfo(p_callback, p_userdata);
	return true;
}

bool OpenXRMetaSpatialEntitySharingExtension::share_spaces_with_groups(XrSpace *p_spaces, uint32_t p_space_count, XrUuid *p_groups, uint32_t p_group_count, ShareSpacesCompleteCallback p_callback, void *p_userdata) {
	if (!is_spatial_entity_group_sharing_supported()) {
		p_callback(XR_ERROR_FEATURE_UNSUPPORTED, p_userdata);
		return false;
	}

	if (p_space_count == 0 || p_group_count == 0) {
		p_callback(XR_ERROR_VALIDATION_FAILURE, p_userdata);
		return false;
	}

	XrShareSpacesRecipientGroupsMETA recipient_info = {
		XR_TYPE_SHARE_SPACES_RECIPIENT_GROUPS_META,
		nullptr,
		p_group_count,
		p_groups,
	};

	XrShareSpacesInfoMETA info = {
		XR_TYPE_SHARE_SPACES_INFO_META,
		nullptr,
		p_space_count,
		p_spaces,
		reinterpret_cast<const XrShareSpacesRecipientBaseHeaderMETA *>(&recipient_info),
	};

	return share_spaces(&info, p_callback, p_userdata);
}

void OpenXRMetaSpatialEntitySharingExtension::on_share_spaces_complete(const XrEventDataShareSpacesCompleteMETA *p_event) {
	if (!requests.has(p_event->requestId)) {
		WARN_PRINT("Received unexpected XR_TYPE_EVENT_DATA_SHARE_SPACES_COMPLETE_META");
		return;
	}

	RequestInfo *request = requests.getptr(p_event->requestId);
	request->callback(p_event->result, request->userdata);
	requests.erase(p_event->requestId);
}
