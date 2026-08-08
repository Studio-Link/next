//------------------------------------------------------------------------
// StudioLink VST3 - processor and controller implementation
//
// Copyright Sebastian Reimers
// License: MIT (see LICENSE File)
//------------------------------------------------------------------------

#include "plugin.h"
#include "cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <cstdio>

// Pure C bridge to libsl. Deliberately does not pull in <re.h>, which is
// not C++ clean (_Atomic).
#include "sl_vst.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace StudioLink {

enum { kStateVersion = 1 };

//------------------------------------------------------------------------
Processor::~Processor ()
{
	if (mIo)
		sl_vst_io_free (mIo);

	if (mEngineRef)
		sl_vst_engine_deref ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::initialize (FUnknown* context)
{
	tresult result = SingleComponentEffect::initialize (context);
	if (result != kResultOk)
		return result;

	addAudioInput (STR16 ("Stereo In"), SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), SpeakerArr::kStereo);

	parameters.addParameter (STR16 ("Mute"), nullptr, 1, 0,
	                         ParameterInfo::kCanAutomate, kParamMute);
	parameters.addParameter (STR16 ("Record"), nullptr, 1, 0,
	                         ParameterInfo::kCanAutomate, kParamRecord);
	parameters.addParameter (STR16 ("Open Web UI"), nullptr, 1, 0,
	                         ParameterInfo::kNoFlags, kParamWebUi);

	int err = sl_vst_engine_ref ();
	if (err != 0) {
		std::fprintf (stderr,
		              "StudioLink VST3: engine start failed (%d)\n",
		              err);
		return kResultFalse;
	}

	mEngineRef = true;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::terminate ()
{
	if (mIo) {
		sl_vst_io_free (mIo);
		mIo = nullptr;
	}

	if (mEngineRef) {
		sl_vst_engine_deref ();
		mEngineRef = false;
	}

	return SingleComponentEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::setActive (TBool state)
{
	if (!state) {
		if (mIo) {
			sl_vst_io_free (mIo);
			mIo = nullptr;
		}
		return kResultOk;
	}

	if (!mEngineRef || mSampleRate <= 0.0 || mMaxBlockSize <= 0)
		return kResultFalse;

	if (mIo)
		return kResultOk;

	int err = sl_vst_io_alloc (&mIo, static_cast<uint32_t> (mSampleRate),
	                           static_cast<size_t> (mMaxBlockSize));
	if (err != 0) {
		std::fprintf (stderr,
		              "StudioLink VST3: io alloc failed (%d)\n", err);
		return kResultFalse;
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::setupProcessing (ProcessSetup& setup)
{
	// Block size and rate decide the bridge buffers, so rebuild it.
	if (mIo) {
		sl_vst_io_free (mIo);
		mIo = nullptr;
	}

	mSampleRate = setup.sampleRate;
	mMaxBlockSize = setup.maxSamplesPerBlock;

	return SingleComponentEffect::setupProcessing (setup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// StudioLink is 16 bit / 48 kHz internally, double precision would
	// only add conversions.
	return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::setBusArrangements (SpeakerArrangement* inputs,
                                                  int32 numIns,
                                                  SpeakerArrangement* outputs,
                                                  int32 numOuts)
{
	if (numIns == 1 && numOuts == 1 && inputs[0] == SpeakerArr::kStereo &&
	    outputs[0] == SpeakerArr::kStereo) {
		return SingleComponentEffect::setBusArrangements (
		    inputs, numIns, outputs, numOuts);
	}

	return kResultFalse;
}

//------------------------------------------------------------------------
void Processor::applyParam (ParamID tag, ParamValue value)
{
	const bool on = value >= 0.5;

	switch (tag) {

	case kParamMute:
		if (on == mMute)
			break;
		mMute = on;
		sl_vst_engine_cmd (SL_VST_CMD_MUTE, on ? 1 : 0);
		break;

	case kParamRecord:
		if (on == mRecord)
			break;
		mRecord = on;
		sl_vst_engine_cmd (SL_VST_CMD_RECORD, on ? 1 : 0);
		break;

	case kParamWebUi:
		// fire on the rising edge only
		if (on && !mWebUi)
			sl_vst_engine_cmd (SL_VST_CMD_WEBUI, 0);
		mWebUi = on;
		break;

	default:
		break;
	}
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::setParamNormalized (ParamID tag,
                                                  ParamValue value)
{
	tresult result =
	    SingleComponentEffect::setParamNormalized (tag, value);
	if (result != kResultTrue)
		return result;

	applyParam (tag, value);

	return kResultTrue;
}

//------------------------------------------------------------------------
void Processor::handleParamChanges (IParameterChanges* changes)
{
	if (!changes)
		return;

	int32 count = changes->getParameterCount ();

	for (int32 i = 0; i < count; i++) {

		IParamValueQueue* queue = changes->getParameterData (i);
		if (!queue)
			continue;

		int32 points = queue->getPointCount ();
		if (points <= 0)
			continue;

		int32 offset = 0;
		ParamValue value = 0.0;

		// sample accurate automation is pointless here, these
		// parameters map to network side state
		if (queue->getPoint (points - 1, offset, value) != kResultTrue)
			continue;

		setParamNormalized (queue->getParameterId (), value);
	}
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::process (ProcessData& data)
{
	handleParamChanges (data.inputParameterChanges);

	if (data.numSamples <= 0)
		return kResultOk;

	if (data.numOutputs < 1 || data.outputs[0].numChannels < 1)
		return kResultOk;

	if (data.symbolicSampleSize != kSample32)
		return kResultOk;

	const float* const* in = nullptr;
	int32 chIn = 0;

	if (data.numInputs > 0 && data.inputs[0].numChannels > 0 &&
	    data.inputs[0].channelBuffers32) {
		in = data.inputs[0].channelBuffers32;
		chIn = data.inputs[0].numChannels;
	}

	sl_vst_io_process (
	    mIo, in, data.outputs[0].channelBuffers32,
	    static_cast<unsigned> (chIn),
	    static_cast<unsigned> (data.outputs[0].numChannels),
	    static_cast<size_t> (data.numSamples));

	// we always produce audio, never flag the output as silent
	data.outputs[0].silenceFlags = 0;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	int32 version = 0;
	if (!streamer.readInt32 (version))
		return kResultFalse;

	if (version != kStateVersion)
		return kResultFalse;

	int32 mute = 0;
	int32 record = 0;

	if (!streamer.readInt32 (mute) || !streamer.readInt32 (record))
		return kResultFalse;

	setParamNormalized (kParamMute, mute ? 1.0 : 0.0);
	setParamNormalized (kParamRecord, record ? 1.0 : 0.0);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::getState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	streamer.writeInt32 (kStateVersion);
	streamer.writeInt32 (mMute ? 1 : 0);
	streamer.writeInt32 (mRecord ? 1 : 0);

	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace StudioLink
