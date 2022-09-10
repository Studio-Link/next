<template>
    <div v-if="localState() == LocalTrackStates.Setup" class="flex justify-center mt-6">
        <ButtonPrimary @click="setLocalState(LocalTrackStates.SelectAudio); setExtended(true)">
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
            <select id="microphone" name="microphone"
                class="text-sl-on_surface_2 mt-1 block w-full rounded-md bg-sl-surface border-none py-2 pl-3 pr-10 text-base focus:border-sl-primary focus:outline-none focus:ring-sl-primary sm:text-sm">
                <option>Default Microphone</option>
                <option>Focusrite 2i2 Studio</option>
            </select>
        </div>
        <div class="px-2 mt-2">
            <label for="speaker" class="block text-sm font-medium text-sl-on_surface_2">Speaker</label>
            <select id="speaker" name="speaker"
                class="text-sl-on_surface_2 mt-1 block w-full rounded-md bg-sl-surface border-none py-2 pl-3 pr-10 text-base focus:border-sl-primary focus:outline-none focus:ring-sl-primary sm:text-sm">
                <option>Default Speaker</option>
                <option>Focusrite 2i2 Studio</option>
            </select>
        </div>
        <ButtonPrimary @click="setLocalState(LocalTrackStates.Ready); setExtended(false)"
            :class="{ 'visible': active, 'invisible': !active }" class="mt-3 ml-2">
            Save
        </ButtonPrimary>
    </div>
</template>


<script setup lang="ts">
import { tracks, LocalTrackStates } from '../states/tracks'
const props = defineProps({ 'active': Boolean, 'pkey': { type: Number, required: true } })

function localState() {
    return tracks.localState(props.pkey)
}

function setLocalState(state: LocalTrackStates) {
    tracks.state[props.pkey].local = state;
}

function setExtended(active: boolean) {
    tracks.extend(props.pkey, active)
}
</script>
