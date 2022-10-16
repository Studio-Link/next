<template>
    <div class="flex justify-center space-x-5 sm:space-x-16 mb-5 mx-auto">
        <button :class="[muted ? 'bg-red-900 hover:bg-red-800' : 'bg-sl-08dp hover:bg-neutral-700']"
            class="rounded-full h-20 w-20 flex items-center justify-center shadow uppercase font-semibold"
            @click="muted = !muted">
            Mute
        </button>
        <button :class="[record ? 'bg-red-900 hover:bg-red-800' : 'bg-sl-08dp hover:bg-neutral-700']"
            class="rounded-full h-20 w-20 flex items-center justify-center shadow uppercase font-semibold"
            @click="record = !record">
            Record
        </button>
    </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
    data() {
        return {
            muted: false,
            record: false,
            chat: false
        };
    },
});
</script>
