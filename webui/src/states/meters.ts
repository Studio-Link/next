interface Meters {
    socket?: WebSocket
    websocket(ws_host: string): void
    /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
    update(message: any): void
}

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
    index = index + 1
    if (index < 2)
        return
    const val = iec_scale(parseFloat(peak))
    document.getElementById("level" + (index - 2))?.style.setProperty("--my-level", val + "% 0 0 0")
}

export const meters: Meters = {
    websocket(ws_host): void {
        this.socket = new WebSocket('ws://' + ws_host + '/ws/v1/meters')
        this.socket.onerror = function() {
            console.log('Websocket error')
        }
        this.socket.onmessage = (message) => {
            this.update(message)
        }
    },
    update(message): void {
        const peaks = message.data.split(" ")
        peaks.forEach(update_meters)
    }
}
