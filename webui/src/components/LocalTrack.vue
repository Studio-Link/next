<template>
	<li aria-label="Local track" class="col-span-1" @mouseenter="setActive()">
		<div class="flex justify-between">
			<h2
				class="ml-1 font-semibold text-sl-disabled text-sm truncate pr-2"
			>{{ getTrackName() }} me@studio.link</h2>
			<div class="flex">
				<div class="font-semibold text-sm text-green-600 uppercase text-right">Online</div>
			</div>
		</div>

		<div class="flex mt-1">
			<div class="bg-sl-02dpa rounded-lg h-44 w-full shadow">
				<div class="flex justify-between items-center">
					<div
						:class="{ 'bg-sl-disabled': isActive() }"
						class="ml-2 text-base leading-none text-black font-bold hover:bg-gray-500 rounded-full px-2 py-1"
					>{{ pkey }}</div>
					<div class="flex-shrink-0 pr-2 text-right mt-1">
						<button
							ref="settings"
							v-click-outside="{
								exclude: ['settings'],
								handler: settingsClose,
							}"
							aria-label="Track Settings"
							class="w-8 h-8 inline-flex items-center justify-center text-sl-disabled rounded-full bg-transparent hover:text-gray-500 focus:outline-none focus:text-sl-surface focus:bg-sl-on_surface_2 transition ease-in-out duration-150"
							@focus="setActive()"
							@click="settingsOpen = !settingsOpen"
						>
							<svg aria-hidden="true" class="w-5 h-5" viewBox="0 0 20 20" fill="currentColor">
								<path
									v-if="isActive()"
									d="M10 6a2 2 0 110-4 2 2 0 010 4zM10 12a2 2 0 110-4 2 2 0 010 4zM10 18a2 2 0 110-4 2 2 0 010 4z"
								/>
							</svg>
						</button>
						<TrackSettings v-if="isActive()" :active="settingsOpen" />
					</div>
				</div>
				<div class="flex justify-center mt-6">
					<Button>
						<svg
							xmlns="http://www.w3.org/2000/svg"
							viewBox="0 0 20 20"
							fill="currentColor"
							class="h-6 mr-1"
						>
							<path
								fill-rule="evenodd"
								d="M7 4a3 3 0 016 0v4a3 3 0 11-6 0V4zm4 10.93A7.001 7.001 0 0017 8a1 1 0 10-2 0A5 5 0 015 8a1 1 0 00-2 0 7.001 7.001 0 006 6.93V17H6a1 1 0 100 2h8a1 1 0 100-2h-3v-2.07z"
								clip-rule="evenodd"
							/>
						</svg>
						Select Microphone
					</Button>
				</div>
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

<script lang="ts">
import { ref, defineComponent } from 'vue'
import TrackSettings from './TrackSettings.vue'
import { tracks } from '../states/tracks'

export default defineComponent({
	components: {
		TrackSettings,
	},
	props: { pkey: { type: Number, required: true } },
	setup(props) {
		const settingsOpen = ref(false)

		function isActive() {
			return tracks.isSelected(props.pkey)
		}

		function setActive() {
			tracks.select(props.pkey)
		}

		function getTrackName() {
			return tracks.getTrackName(props.pkey)
		}

		function settingsClose() {
			settingsOpen.value = false
		}

		return {
			isActive,
			setActive,
			getTrackName,
			settingsOpen,
			settingsClose,
		}
	},
})
</script>
