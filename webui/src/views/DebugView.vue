<template>
    <h2>Debug</h2>

     <div id="charts" class="grid grid-cols-3 gap-4"></div>
</template>

<script setup lang="ts">
import "uplot/dist/uPlot.min.css"
import { onMounted } from 'vue'
import { Debug } from "../states/debug"
import { config } from "../config"

onMounted(() => {
    Debug.websocket(config.ws_host())
})
</script>
