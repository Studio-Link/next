<template>
    <div class="flex h-full">
        <StudioNav />
        <div class="container mx-auto">
            <header class="flex flex-wrap items-end justify-between p-2">
                <!--
                <a aria-label="Logo Reload App" href="javascript:window.location.reload(true)">
                    <img src="/logo_standalone.svg" class="h-9 opacity-40" alt="Logo" />
                </a>
                -->
                <!-- <HeadStatus /> -->
                <RoomsNav />
                <Navbar />
            </header>
            <div>
                <main>
                    <Tracks class="px-2" />
                </main>
            </div>
        </div>
    </div>
    <footer class="bottom-0 fixed left-16 right-0">
        <div class="flex flex-wrap justify-between items-end px-2">
            <BottomActions />
            <Version />
        </div>
    </footer>
</template>

<script setup lang='ts'>
import Version from './components/Version.vue'
import BottomActions from './components/BottomActions.vue'
import Tracks from './components/Tracks.vue'
import Navbar from './components/Navbar.vue'
import RoomsNav from './components/RoomsNav.vue'
import StudioNav from './components/StudioNav.vue'
</script>
