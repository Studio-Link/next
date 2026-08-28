//------------------------------------------------------------------------
// StudioLink VST3 - module factory
//
// Copyright Sebastian Reimers
// License: MIT (see LICENSE File)
//------------------------------------------------------------------------

#include "plugin.h"
#include "cids.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringSubCategory Steinberg::Vst::PlugType::kFxNetwork

BEGIN_FACTORY_DEF (stringCompanyName, stringCompanyWeb, stringCompanyEmail)

DEF_CLASS2 (INLINE_UID_FROM_FUID (StudioLink::kStudioLinkProcessorUID),
	PClassInfo::kManyInstances,
	kVstAudioEffectClass,
	stringPluginName,
	0, // single component effects cannot be distributed
	stringSubCategory,
	FULL_VERSION_STR,
	kVstVersionString,
	StudioLink::Processor::createInstance)

END_FACTORY
