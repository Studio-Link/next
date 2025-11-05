<template>
    <div v-if="localState() == LocalTrackStates.Setup" class="flex justify-center mt-6">
        <ButtonPrimary @click="setLocalState(LocalTrackStates.SelectAudio);">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor" class="h-6 mr-1">
                <path fill-rule="evenodd"
                    d="M7 4a3 3 0 016 0v4a3 3 0 11-6 0V4zm4 10.93A7.001 7.001 0 0017 8a1 1 0 10-2 0A5 5 0 015 8a1 1 0 00-2 0 7.001 7.001 0 006 6.93V17H6a1 1 0 100 2h8a1 1 0 100-2h-3v-2.07z"
                    clip-rule="evenodd" />
            </svg>
            Select Microphone
        </ButtonPrimary>
    </div>
    <div v-if="localState() == LocalTrackStates.SelectAudio">
        <div class="px-2">
            <label for="microphone" class="block text-sm font-medium text-sl-on_surface_2">Microphone</label>
            <select v-model="mic" id="microphone" name="microphone" autofocus
                class="text-sl-on_surface_2 mt-1 block w-full rounded-md bg-sl-surface border-none py-2 pl-3 pr-10 text-base focus:border-sl-primary focus:outline-hidden focus:ring-sl-primary sm:text-sm">

                <option v-for="option in tracks.local_tracks[0].audio.src" :key="option.idx" :value="option.idx">
                    {{option.name}}</option>
            </select>
        </div>
        <div class="px-2 mt-2">
            <label for="speaker" class="block text-sm font-medium text-sl-on_surface_2">Speaker</label>
            <select v-model="speaker" id="speaker" name="speaker"
                class="text-sl-on_surface_2 mt-1 block w-full rounded-md bg-sl-surface border-none py-2 pl-3 pr-10 text-base focus:border-sl-primary focus:outline-hidden focus:ring-sl-primary sm:text-sm">
                <option v-for="option in tracks.local_tracks[0].audio.play" :key="option.idx" :value="option.idx">
                    {{option.name}}</option>
            </select>
        </div>
        <ButtonPrimary @click="save()" class="mt-3 ml-2">
            Save
        </ButtonPrimary>
    </div>
</template>


<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { tracks, LocalTrackStates } from '../states/tracks'
import api from '../api'

const props = defineProps({ 'active': Boolean, 'pkey': { type: Number, required: true } })

const mic = ref(0)
const speaker = ref(0)

onMounted(() => {
    mic.value = tracks.local_tracks[0].audio.src_dev 
    speaker.value = tracks.local_tracks[0].audio.play_dev
})

function localState() {
    return tracks.localState(props.pkey)
}

function setLocalState(state: LocalTrackStates) {
    tracks.state[props.pkey].local = state;
}

function save() {
    setLocalState(LocalTrackStates.Ready)
    api.audio_device(1, mic.value, speaker.value)
}


</script>
