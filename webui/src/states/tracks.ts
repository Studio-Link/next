import { reactive } from 'vue'

export enum LocalTrackStates {
    Setup = 0,
    SelectAudio,
    Ready,
}

export enum TrackStatus {
    IDLE,
    AUDIO_READY,
    REMOTE_CONNECTED,
    REMOTE_CALLING,
    REMOTE_CLOSED
}

interface Track {
    id: number
    name: string
    status: TrackStatus
    error: string
}

interface State extends Track {
    selected: boolean
    local: LocalTrackStates
}

interface AudioDevice {
    idx: number
    name: string
}

interface AudioList {
    src: AudioDevice[]
    play: AudioDevice[]
    src_dev: number
    play_dev: number
}

interface LocalTrack extends Track {
    audio: AudioList
}

interface Tracks {
    socket?: WebSocket
    state: State[]
    local_tracks: LocalTrack[]
    remote_tracks: Track[]
    selected_debounce: boolean
    clear_tracks(): void
    /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
    update(tracks: any): void
    getTrackName(id: number): string
    websocket(ws_host: string): void
    isValid(id: number): boolean
    isSelected(id: number): boolean
    localState(id: number): LocalTrackStates
    select(id: number): void
    selected(): number,
}

export const tracks: Tracks = {
    state: reactive([]),
    remote_tracks: reactive([]),
    local_tracks: reactive([]),
    selected_debounce: false,

    getTrackName(id: number): string {
        if (this.state[id] != undefined) {
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
        this.local_tracks.length = 0
        this.remote_tracks.length = 0
    },

    update(tracks): void {
        let last_key = 0
        this.clear_tracks()

        for (const key in tracks) {
            tracks[key].id = parseInt(key)

            /* Initialize frontend state only once */
            if (this.state[tracks[key].id] === undefined) {
                this.state[tracks[key].id] = {
                    id: tracks[key].id,
                    selected: false,
                    status: TrackStatus.IDLE,
                    error: "",
                    local: LocalTrackStates.Setup,
                    name: tracks[key].name,
                }
                last_key = parseInt(key)
            }

            this.state[tracks[key].id].name = tracks[key].name

            if (tracks[key].type == 'local') {
                this.local_tracks.push(tracks[key])
            }

            if (tracks[key].type == 'remote') {
                this.remote_tracks.push(tracks[key])
            }
        }

        /* Cleanup state for deleted tracks */
        for (const key in this.state) {
            if (tracks[key] === undefined) {
                delete this.state[key]
            }
        }

        /* select last added track */
        if (last_key > 0) {
            this.select(last_key)
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

    localState(id: number): LocalTrackStates {
        return this.state[id].local
    },

    select(id: number): void {
        if (this.state[id] === undefined)
            return

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

    selected(): number {
        let id = -1
        this.state.forEach((el) => {
            if (el.selected) {
                id = el.id
            }
        })

        return id
    },
}
