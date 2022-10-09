<template>
    <Menu as="div" class="relative inline-block text-left">
        <div>
            <MenuButton
                class="flex items-center rounded-full text-neutral-500 hover:text-neutral-400 focus:outline-none focus:ring-2 focus:ring-sl-primary">
                <span class="sr-only">Open options</span>
                <EllipsisVerticalIcon class="h-6 w-6" aria-hidden="true" />
            </MenuButton>
        </div>

        <transition enter-active-class="transition ease-out duration-100"
            enter-from-class="transform opacity-0 scale-95" enter-to-class="transform opacity-100 scale-100"
            leave-active-class="transition ease-in duration-75" leave-from-class="transform opacity-100 scale-100"
            leave-to-class="transform opacity-0 scale-95">
            <MenuItems
                class="absolute right-0 z-10 mt-2 w-56 origin-top-right divide-y divide-neutral-800 rounded-md shadow-lg ring-1 ring-black ring-opacity-5 bg-sl-06dp text-sl-on_surface_2">
                <div class="py-1">
                    <MenuItem v-slot="{ active }">
                    <a href="#" @click="audioSettings()"
                        :class="[active ? 'bg-sl-01dp' : '', 'group flex items-center px-4 py-2 text-sm']">

                        <svg class="mr-3 h-5 w-5 fill-neutral-400 group-hover:fill-neutral-500" aria-hidden="true"
                            xmlns="http://www.w3.org/2000/svg" viewBox="0 0 384 512">
                            <path
                                d="M192 352c53 0 96-43 96-96V96c0-53-43-96-96-96s-96 43-96 96v160C96 309 139 352 192 352zM144 96c0-26.5 21.5-48 48-48s48 21.5 48 48l-32 0c-8.844 0-16 7.156-16 16s7.156 16 16 16L240 128v32h-32C199.2 160 192 167.2 192 176S199.2 192 208 192h32v32h-32C199.2 224 192 231.2 192 240S199.2 256 208 256h32c0 26.5-21.5 48-48 48S144 282.5 144 256V96zM344 192C330.7 192 320 202.7 320 215.1V256c0 74.75-64.5 134.9-140.8 127.4C112.8 376.9 64 317.1 64 250.3v-34.25C64 202.7 53.25 192 40 192S16 202.7 16 215.1v32.15c0 89.63 64 169.6 152 181.7V464h-48C106.7 464 96 474.7 96 488C96 501.3 106.7 512 120 512h144c13.25 0 24-10.75 24-24c0-13.26-10.75-24-24-24h-48v-33.77C301.8 418.5 368 344.9 368 256V215.1C368 202.7 357.3 192 344 192z" />
                        </svg>
                        Audio Settings
                    </a>
                    </MenuItem>
                </div>
            </MenuItems>
        </transition>
    </Menu>
</template>

<script setup lang="ts">
import { defineProps } from 'vue'
import { Menu, MenuButton, MenuItem, MenuItems } from '@headlessui/vue'
import { EllipsisVerticalIcon } from '@heroicons/vue/20/solid'
import { LocalTrackStates, tracks } from '../states/tracks'
import api from '../api'

const props = defineProps({ 'pkey': { type: Number, required: true } })

function audioSettings() {
    tracks.state[props.pkey].local = LocalTrackStates.SelectAudio
    tracks.state[props.pkey].extended = true
}


</script>
