//------------------------------------------------------------------------
// StudioLink VST3 - version information
//
// Copyright Sebastian Reimers
// License: MIT (see LICENSE File)
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/fplatform.h"

#define MAJOR_VERSION_STR "26"
#define MAJOR_VERSION_INT 26

#define SUB_VERSION_STR "01"
#define SUB_VERSION_INT 1

#define RELEASE_NUMBER_STR "0"
#define RELEASE_NUMBER_INT 0

#define BUILD_NUMBER_STR "0"
#define BUILD_NUMBER_INT 0

#define FULL_VERSION_STR                                                      \
	MAJOR_VERSION_STR "." SUB_VERSION_STR "." RELEASE_NUMBER_STR          \
			  "." BUILD_NUMBER_STR

#define VERSION_STR                                                           \
	MAJOR_VERSION_STR "." SUB_VERSION_STR "." RELEASE_NUMBER_STR

#define stringOriginalFilename "studiolink.vst3"
#define stringFileDescription  "StudioLink Audio over IP"
#define stringCompanyName      "Studio Link"
#define stringCompanyWeb       "https://studio-link.de"
#define stringCompanyEmail     "mailto:info@studio-link.de"
#define stringLegalCopyright   "Copyright (C) 2013-2026 Sebastian Reimers"
#define stringLegalTrademarks                                                 \
	"VST is a trademark of Steinberg Media Technologies GmbH"

#if SMTG_PLATFORM_64
#define stringPluginName "StudioLink VST3 (64Bit)"
#else
#define stringPluginName "StudioLink VST3"
#endif
