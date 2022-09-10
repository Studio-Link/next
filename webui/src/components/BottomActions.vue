<template>
  <div class="flex justify-center space-x-5 sm:space-x-16 mb-5 mx-auto">
    <button
      :class="{ 'bg-red-900 hover:bg-red-800': muted, 'bg-sl-08dp': !muted}"
      class="rounded-full h-20 w-20 flex items-center justify-center hover:bg-gray-700 shadow uppercase font-semibold"
      @click="muted = !muted"
    >
      Mute
    </button>
    <button
      :class="{ 'bg-red-900 hover:bg-red-800': record, 'bg-sl-08dp': !record}"
      class="rounded-full h-20 w-20 flex items-center justify-center hover:bg-gray-700 shadow uppercase font-semibold"
      @click="record = !record"
    >
      Record
    </button>
    <button
      :class="{ 'bg-red-900 hover:bg-red-800': chat, 'bg-sl-08dp': !chat}"
      class="rounded-full h-20 w-20 flex items-center justify-center hover:bg-gray-700 shadow uppercase font-semibold"
      @click="chat = !chat"
    >
     Chat
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
