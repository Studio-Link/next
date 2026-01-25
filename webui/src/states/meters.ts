import { Ref, ref } from 'vue'

function iec_scale(db: number) {
    let def = 0.0;

    if (db < -70.0 || isNaN(db)) {
        def = 0.0;
    } else if (db < -60.0) {
        def = (db + 70.0) * 0.25;
    } else if (db < -50.0) {
        def = (db + 60.0) * 0.5 + 2.5;
    } else if (db < -40.0) {
        def = (db + 50.0) * 0.75 + 7.5;
    } else if (db < -30.0) {
        def = (db + 40.0) * 1.5 + 15.0;
    } else if (db < -20.0) {
        def = (db + 30.0) * 2.0 + 30.0;
    } else if (db < 0.0) {
        def = (db + 20.0) * 2.5 + 50.0;
    } else {
        def = 100.0;
    }

    return ((100.0 - def));

}

function update_meters(peak: string, index: number) {
    const val = iec_scale(parseFloat(peak))
    document.getElementById("level" + (index + 1))?.style.setProperty("--my-level", val + "% 0 0 0")
}

interface Meters {
    socket?: WebSocket
    record_timer: Ref<string>
    record: Ref<boolean>
    websocket(ws_host: string): void
    /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
    update(message: any): void
}

function pad(num: number, size: number) {
    const s = '0000' + num
    return s.substring(s.length - size)
}

export const meters: Meters = {
    record_timer: ref("0:00:00"),
    record: ref(false),
    websocket(ws_host): void {
        this.socket = new WebSocket('ws://' + ws_host + '/ws/v1/meters')
        this.socket.onerror = function () {
            console.log('Websocket error')
        }
        this.socket.onmessage = (message) => {
            this.update(message)
        }
    },
    update(message): void {
        const peaks = message.data.split(" ")
        let time = parseInt(peaks.shift())
        if (time) {
            this.record.value = true
            time = time / 1000
            const h = Math.floor(time / (60 * 60))
            time = time % (60 * 60)
            const m = Math.floor(time / 60)
            time = time % 60
            const s = Math.floor(time)

            this.record_timer.value = pad(h, 1) + ':' + pad(m, 2) + ':' + pad(s, 2)
        }
        else {
            this.record.value = false
        }

        //console.log(peaks)
        peaks.forEach(update_meters)
    }
}
