<template>
    <div class="flex justify-center space-x-5 sm:space-x-16 mb-5 mx-auto">
        <button :class="[tracks.isMuted(1) ? 'bg-red-900 hover:bg-red-800' : 'bg-sl-08dp hover:bg-neutral-700']"
            class="rounded-full h-20 w-20 flex items-center justify-center shadow-sm uppercase font-semibold"
            @click="api.track_mute(1)">
            Mute
        </button>
        <button :class="[record ? 'bg-red-900 hover:bg-red-800' : 'bg-sl-08dp hover:bg-neutral-700']"
            class="rounded-full h-20 w-20 flex items-center justify-center shadow-sm uppercase font-semibold"
            @click="record = !record">
            Record
        </button>
    </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import api from '../api'
import { tracks} from '../states/tracks'


const record = ref(false)


</script>
