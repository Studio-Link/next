<template>
	<li aria-label="Empty Remote Call" class="col-span-1" @mouseenter="setActive()">
		<div class="flex justify-between px-1">
			<h2 class="font-semibold text-sl-disabled text-sm truncate pr-2">{{ getTrackName() }}</h2>
			<div class="flex">
				<div class="font-semibold text-sm text-green-600 uppercase text-right">Connected</div>
				<div class="w-4"></div>
			</div>
			<!-- <div class="font-semibold text-sm text-yellow-500 uppercase">Calling</div> -->
			<!-- <div class="font-semibold text-sm text-red-500 uppercase">Error</div> -->
			<!-- <div class="font-semibold text-sm text-sl-green uppercase">Connected</div> -->
			<!-- <div class="font-semibold text-sm text-sl-yellow uppercase">Calling</div> -->
			<!-- <div class="font-semibold text-sm text-sl-red uppercase">Error</div> -->
		</div>

		<div class="flex mt-1">
			<div class="bg-sl-02dpa rounded-lg h-44 w-full shadow">
				<div class="flex justify-end">
					<div class="flex-shrink-0 pr-2 text-right mt-1">
						<button
							ref="settings"
							v-click-outside="{
								exclude: ['settings'],
								handler: settingsClose,
							}"
							aria-label="Track Settings"
							class="w-8 h-8 inline-flex items-center justify-center text-gray-400 rounded-full bg-transparent hover:text-gray-500 focus:outline-none focus:text-sl-surface focus:bg-sl-on_surface_2 transition ease-in-out duration-150"
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
				<div v-if="isActive()" class="flex-1 px-4 grid grid-cols-1">
					<label
						:for="pkey.toString()"
						class="block text-sm font-medium leading-5 text-sl-on_surface_2"
					>Enter Partner ID</label>
					<div class="mt-1 relative rounded-md shadow-sm">
						<input
							:id="pkey.toString()"
							ref="slid"
							type="text"
							class="form-input block w-full sm:text-sm sm:leading-5 text-sl-on_surface_1 bg-sl-surface mb-2 border-none focus:ring-sl-primary rounded-lg"
							placeholder="xyz@studio.link"
						/>
					</div>
					<div class="mt-2 flex justify-between items-center">
						<Button>
							<svg
								xmlns="http://www.w3.org/2000/svg"
								viewBox="0 0 20 20"
								fill="currentColor"
								class="h-4 mr-1"
							>
								<path
									d="M17.924 2.617a.997.997 0 00-.215-.322l-.004-.004A.997.997 0 0017 2h-4a1 1 0 100 2h1.586l-3.293 3.293a1 1 0 001.414 1.414L16 5.414V7a1 1 0 102 0V3a.997.997 0 00-.076-.383z"
								/>
								<path
									d="M2 3a1 1 0 011-1h2.153a1 1 0 01.986.836l.74 4.435a1 1 0 01-.54 1.06l-1.548.773a11.037 11.037 0 006.105 6.105l.774-1.548a1 1 0 011.059-.54l4.435.74a1 1 0 01.836.986V17a1 1 0 01-1 1h-2C7.82 18 2 12.18 2 5V3z"
								/>
							</svg>
							Call
						</Button>
						<span class="text-sl-on_surface_2">OR</span>
						<Button>
							<svg
								xmlns="http://www.w3.org/2000/svg"
								class="h-4 mr-1"
								viewBox="0 0 20 20"
								fill="currentColor"
							>
								<path
									fill-rule="evenodd"
									d="M12.586 4.586a2 2 0 112.828 2.828l-3 3a2 2 0 01-2.828 0 1 1 0 00-1.414 1.414 4 4 0 005.656 0l3-3a4 4 0 00-5.656-5.656l-1.5 1.5a1 1 0 101.414 1.414l1.5-1.5zm-5 5a2 2 0 012.828 0 1 1 0 101.414-1.414 4 4 0 00-5.656 0l-3 3a4 4 0 105.656 5.656l1.5-1.5a1 1 0 10-1.414-1.414l-1.5 1.5a2 2 0 11-2.828-2.828l3-3z"
									clip-rule="evenodd"
								/>
							</svg>Invite
						</Button>
					</div>
				</div>
				<div v-if="!isActive()" class="text-center mt-10 text-sl-disabled">No call</div>
				<div class="text-right mt-1 text-xs">
					<button
						class="opacity-30 hover:opacity-60 focus:opacity-60"
						title="Remove track"
						@click="deleteRemoteTrack()"
						@focus="setActive()"
					>
						<svg v-if="isActive()" class="w-5 h-5" viewBox="0 0 20 20" fill="currentColor"> 
							<path
								fill-rule="evenodd"
								d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z"
								clip-rule="evenodd"
							/>
						</svg>
					</button>
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
import api from '../api'

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

		function deleteRemoteTrack() {
			api.track_del(props.pkey)
			/* let req = new XMLHttpRequest() */
			/* req.open('DELETE', '/api/v1/tracks') */
			/* req.send(String(props.pkey)) */
		}

		return {
			isActive,
			setActive,
			getTrackName,
			settingsOpen,
			settingsClose,
			deleteRemoteTrack,
		}
	},
})
</script>
