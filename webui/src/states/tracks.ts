import { reactive } from 'vue'

interface Track {
	id: number
	name: string
}

interface State extends Track {
	selected: boolean
}

interface RemoteTrack extends Track {
	status?: string
}

interface Tracks {
	socket?: WebSocket
	state: State[]
	remote_tracks: RemoteTrack[]
	local_tracks: Track[]
	stream_tracks: Track[]
	selected_debounce: boolean
	clear_tracks(): void
	/* eslint-disable-next-line @typescript-eslint/no-explicit-any */
	update(tracks: any): void
	getTrackName(id: number): string
	websocket(ws_host: string): void
	isValid(id: number): boolean
	isSelected(id: number): boolean
	select(id: number): void
}

export const tracks: Tracks = {
	state: reactive([]),
	remote_tracks: reactive([]),
	local_tracks: reactive([]),
	stream_tracks: reactive([]),
	selected_debounce: false,

	getTrackName(id: number): string {
		if (this.state[id]) {
			return this.state[id].name
		}

		return 'error'
	},

	websocket(ws_host: string): void {
		this.socket = new WebSocket('ws://' + ws_host + '/ws/v1/tracks')
		this.socket.onerror = function() {
			console.log('Websocket error')
		}
		this.socket.onmessage = (message) => {
			const tracks = JSON.parse(message.data)
			this.update(tracks)
		}
		this.selected_debounce = false
	},

	clear_tracks(): void {
		this.state.length = 0
		this.local_tracks.length = 0
		this.remote_tracks.length = 0
		this.stream_tracks.length = 0
	},

	update(tracks): void {
		this.clear_tracks()
		for (const key in tracks) {
			tracks[key].id = parseInt(key);
			if (tracks[key].type == 'remote') {
				this.remote_tracks.push(tracks[key])
			}
			if (tracks[key].type == 'local') {
				this.local_tracks.push(tracks[key])
			}
			this.state[tracks[key].id] = {
				id: tracks[key].id,
				selected: false,
				name: tracks[key].name,
			}
		}
	},

	isValid(id: number): boolean {
		if (this.state[id]) {
			return true
		}
		return false
	},

	isSelected(id: number): boolean {
		if (this.state[id]) {
			return this.state[id].selected
		}
		return false
	},

	select(id: number): void {
		//Workaround for mouseenter event after focus change
		if (this.selected_debounce) return
		this.selected_debounce = true
		setTimeout(() => {
			this.selected_debounce = false
		}, 20)

		this.state.forEach((el) => {
			el.selected = false
		})

		this.state[id].selected = true
	},
}
