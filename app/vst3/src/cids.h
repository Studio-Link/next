//------------------------------------------------------------------------
// StudioLink VST3 - class and parameter identifiers
//
// Copyright Sebastian Reimers
// License: MIT (see LICENSE File)
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace StudioLink {

// Single component effect: processor and controller share one class id.
// Never change this, hosts identify the plug-in by it.
static const Steinberg::FUID kStudioLinkProcessorUID (
	0x195982fc, 0xb4a148e8, 0x91a5bf40, 0x3eba43a1);

enum ParamIds : Steinberg::Vst::ParamID {
	kParamMute   = 100,
	kParamRecord = 101,
	kParamWebUi  = 102,
};

} // namespace StudioLink
