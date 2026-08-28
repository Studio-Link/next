//------------------------------------------------------------------------
// StudioLink VST3 - processor and controller
//
// Copyright Sebastian Reimers
// License: MIT (see LICENSE File)
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstsinglecomponenteffect.h"

struct sl_vst_io;

namespace StudioLink {

//------------------------------------------------------------------------
/**
 * StudioLink as a VST3 effect.
 *
 * The plug-in has no editor of its own - StudioLink is operated through its
 * web frontend, which the "Open Web UI" parameter launches. Processor and
 * controller are combined, because the shared engine cannot be split across
 * two components anyway.
 *
 * Audio flow:
 *   DAW input  -> StudioLink local track -> remote peers
 *   DAW output <- mix of all remote peers
 */
class Processor : public Steinberg::Vst::SingleComponentEffect
{
public:
	Processor () = default;
	~Processor () SMTG_OVERRIDE;

	static Steinberg::FUnknown* createInstance (void*)
	{
		return static_cast<Steinberg::Vst::IAudioProcessor*> (
		    new Processor);
	}

	//--- from IPluginBase --------------------------------------------
	Steinberg::tresult PLUGIN_API initialize (
	    Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	//--- from IComponent ---------------------------------------------
	Steinberg::tresult PLUGIN_API setActive (
	    Steinberg::TBool state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState (
	    Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (
	    Steinberg::IBStream* state) SMTG_OVERRIDE;

	//--- from IAudioProcessor ----------------------------------------
	Steinberg::tresult PLUGIN_API setBusArrangements (
	    Steinberg::Vst::SpeakerArrangement* inputs,
	    Steinberg::int32 numIns,
	    Steinberg::Vst::SpeakerArrangement* outputs,
	    Steinberg::int32 numOuts) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API canProcessSampleSize (
	    Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setupProcessing (
	    Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API process (
	    Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
	// The remote peers keep sending even when the DAW input is silent,
	// so the host must never stop calling process().
	Steinberg::uint32 PLUGIN_API getTailSamples () SMTG_OVERRIDE
	{
		return Steinberg::Vst::kInfiniteTail;
	}

	//--- from IEditController -----------------------------------------
	Steinberg::tresult PLUGIN_API setParamNormalized (
	    Steinberg::Vst::ParamID tag,
	    Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;

private:
	// Forwards a parameter to the StudioLink thread, but only when it
	// actually changed - process() runs on the realtime thread.
	void applyParam (Steinberg::Vst::ParamID tag,
	                 Steinberg::Vst::ParamValue value);
	void handleParamChanges (Steinberg::Vst::IParameterChanges* changes);

	sl_vst_io* mIo = nullptr;
	bool mEngineRef = false;

	bool mMute = false;
	bool mRecord = false;
	bool mWebUi = false;

	double mSampleRate = 0.0;
	Steinberg::int32 mMaxBlockSize = 0;
};

//------------------------------------------------------------------------
} // namespace StudioLink
