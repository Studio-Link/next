<template>
    <div v-if="isActive() && isNoCall()" class="flex-1 px-4 grid grid-cols-1">
        <label :for="pkey.toString()" class="block text-sm font-medium leading-5 text-sl-on_surface_2">Enter
            Partner ID</label>
        <div class="mt-1 relative rounded-md shadow-sm">
            <input @keyup.enter="api.dial(pkey, peer)" :id="pkey.toString()" v-model="peer" ref="slid" type="text"
                class="form-input block w-full sm:text-sm sm:leading-5 text-sl-on_surface_1 bg-sl-surface mb-2 border-none focus:ring-sl-primary rounded-lg"
                placeholder="xyz@studio.link" />
        </div>
        <div class="mt-2 flex justify-between items-center">
            <ButtonPrimary @click="api.dial(pkey, peer)">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor"
                    class="h-4 mr-1">
                    <path
                        d="M17.924 2.617a.997.997 0 00-.215-.322l-.004-.004A.997.997 0 0017 2h-4a1 1 0 100 2h1.586l-3.293 3.293a1 1 0 001.414 1.414L16 5.414V7a1 1 0 102 0V3a.997.997 0 00-.076-.383z" />
                    <path
                        d="M2 3a1 1 0 011-1h2.153a1 1 0 01.986.836l.74 4.435a1 1 0 01-.54 1.06l-1.548.773a11.037 11.037 0 006.105 6.105l.774-1.548a1 1 0 011.059-.54l4.435.74a1 1 0 01.836.986V17a1 1 0 01-1 1h-2C7.82 18 2 12.18 2 5V3z" />
                </svg>
                Call
            </ButtonPrimary>
            <!--
            <span class="text-sl-on_surface_2">OR</span>
            <ButtonSecondary>
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 mr-1" viewBox="0 0 20 20"
                    fill="currentColor">
                    <path fill-rule="evenodd"
                        d="M12.586 4.586a2 2 0 112.828 2.828l-3 3a2 2 0 01-2.828 0 1 1 0 00-1.414 1.414 4 4 0 005.656 0l3-3a4 4 0 00-5.656-5.656l-1.5 1.5a1 1 0 101.414 1.414l1.5-1.5zm-5 5a2 2 0 012.828 0 1 1 0 101.414-1.414 4 4 0 00-5.656 0l-3 3a4 4 0 105.656 5.656l1.5-1.5a1 1 0 10-1.414-1.414l-1.5 1.5a2 2 0 11-2.828-2.828l3-3z"
                        clip-rule="evenodd" />
                </svg>Invite
            </ButtonSecondary>
            -->
        </div>
    </div>
    <div v-if="!isActive() && isNoCall()" class="text-center mt-10 text-sl-disabled">No call</div>
    <div v-if="isCalling()" class="text-center mt-2 text-sl-disabled">
        <div class="mb-4 truncate">Calling <br />{{ peer }}</div>
        <ButtonSecondary @click="api.hangup(pkey)">
            Hangup
        </ButtonSecondary>
    </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { tracks, RemoteTrackStates } from '../../states/tracks'
import api from '../../api'

const props = defineProps({
'pkey': { type: Number, required: true },
'idx': { type: Number, required: true }
})

const peer = ref("");

function isActive() {
    return tracks.isSelected(props.pkey)
}

function isNoCall() {
    return tracks.remote_tracks[props.idx].state === RemoteTrackStates.NoCall
}

function isCalling() {
    return tracks.remote_tracks[props.idx].state === RemoteTrackStates.Calling
}

</script>
