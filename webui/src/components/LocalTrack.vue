<template>
    <li aria-label="Local track" class="col-span-1" @mouseenter="setActive()" @touchstart.passive="setActive()">
        <div class="flex justify-between h-5">
            <h2 class="ml-1 font-semibold text-sl-disabled text-sm truncate pr-2">{{ getTrackName() }}
            </h2>
            <div class="flex">
                <div v-if="tracks.state[props.pkey].status === TrackStatus.LOCAL_REGISTER_OK" class="font-semibold text-sm text-green-600 uppercase text-right">ONLINE</div>
                <div v-if="tracks.state[props.pkey].status === TrackStatus.LOCAL_REGISTER_FAIL" class="font-semibold text-sm text-red-600 uppercase text-right">FAILED</div>
            </div>
        </div>

        <div class="flex mt-1">
            <div class="bg-sl-02dpa rounded-lg min-h-[11em] w-full shadow pb-2">
                <div class="flex justify-between items-center">
                    <div :id="`track${pkey}`" tabindex="0"
                        :class="{ 'bg-sl-disabled': isActive(), 'bg-sl-24dpa': !isActive() }"
                        class="inline-flex items-center justify-center ml-2 text-sm leading-none text-black font-bold rounded-full px-2 py-1 focus:outline-none">
                        <span class="sr-only">Local Track</span>
                        {{ pkey }}
                        <span class="sr-only">selected</span>
                    </div>
                    <div class="flex-shrink-0 pr-2 text-right w-8 h-8 mt-2">
                        <LocalTrackSettings v-if="isActive()" :pkey="props.pkey" />
                    </div>
                </div>
                <AudioSettings :active="isActive()" :pkey="pkey" />

                <svg v-if="localState() == LocalTrackStates.Ready" class="w-24 h-24 fill-neutral-700 mx-auto"
                    xmlns="http://www.w3.org/2000/svg" viewBox="0 0 448 512">
                    <path
                        d="M224 32C135.6 32 64 103.6 64 192v16c0 8.8-7.2 16-16 16s-16-7.2-16-16V192C32 86 118 0 224 0S416 86 416 192v16c0 61.9-50.1 112-112 112H240 224 208c-17.7 0-32-14.3-32-32s14.3-32 32-32h32c17.7 0 32 14.3 32 32h32c44.2 0 80-35.8 80-80V192c0-88.4-71.6-160-160-160zM96 192c0-70.7 57.3-128 128-128s128 57.3 128 128c0 13.9-2.2 27.3-6.3 39.8C337.4 246.3 321.8 256 304 256h-8.6c-11.1-19.1-31.7-32-55.4-32H208c-35.3 0-64 28.7-64 64c0 1.4 0 2.7 .1 4C114.8 268.6 96 232.5 96 192zM224 352h16 64 9.6C387.8 352 448 412.2 448 486.4c0 14.1-11.5 25.6-25.6 25.6H25.6C11.5 512 0 500.5 0 486.4C0 412.2 60.2 352 134.4 352H208h16z" />
                </svg>

            </div>

            <div class="flex w-5 items-end ml-0.5 opacity-60" aria-hidden="true">
                <div id="levels" class="levels">
                    <div id="level1" class="level"></div>
                    <div id="level2" class="level"></div>
                </div>
            </div>
        </div>
    </li>
</template>

<script setup lang="ts">
import { watch } from 'vue'
import LocalTrackSettings from './LocalTrackSettings.vue'
import AudioSettings from './AudioSettings.vue'
import { LocalTrackStates, tracks, TrackStatus } from '../states/tracks'
import api from '../api'

const props = defineProps({ 'pkey': { type: Number, required: true } })

function localState() {
    return tracks.localState(props.pkey)
}

function isActive() {
    return tracks.isSelected(props.pkey)
}

function setActive() {
    tracks.select(props.pkey)
}

function getTrackName() {
    return tracks.getTrackName(props.pkey)
}

/* Add remote track if local track is ready and no 2nd track exists */
watch(() => tracks.state[props.pkey].local, (state) => {
    if (tracks.state[2] !== undefined) {
        return
    }

    if (state == LocalTrackStates.Ready) {
        api.track_add('remote')
    }
})
</script>
