import { CanvasLineChart } from '../chart'

interface Debug {
    socket?: WebSocket
    websocket(ws_host: string): void
    close(): void
}

interface ChartStore {
    chart: CanvasLineChart
    xs: number[]
    ys: number[]
}

interface Data {
    id: string
    x: number
    y: number
}

let chartsById: Record<string, ChartStore> = {}


function createChart(id: string): void {
    const canvas = document.createElement("canvas")
    canvas.className = "chart-container bg-sl-01dpa text-gray-500"
    canvas.id = id

    document.getElementById("charts")!.appendChild(canvas)

    const xs: number[] = [];
    const ys: number[] = [];

    const u = new CanvasLineChart(id);
    chartsById[id] = { chart: u, xs, ys };
}


function handleNewData(d: Data, maxPoints: number = 100): void {
    const id = d.id;
    if (!chartsById[id]) {
        createChart(id);
        console.log(d)
    }

    const { chart, xs, ys } = chartsById[id];

    xs.push(d.x);
    ys.push(d.y);

    if (xs.length > maxPoints) {
        xs.shift();
        ys.shift();
    }

    chart.draw({ x: xs, y: ys });
}


export const Debug: Debug = {
    websocket(ws_host): void {
        this.socket = new WebSocket('ws://' + ws_host + '/ws/v1/debug')
        this.socket.onerror = function () {
            console.log('Websocket error')
        }
        this.socket.onmessage = (message) => {
            const json = JSON.parse(message.data)
            const ts = Math.floor(json.ts / 1000)
            if (json.cat == 'slmain') {
                const d: Data = { id: 're_loop_jitter', x: ts, y: json.args.loop_jitter }
                handleNewData(d)
                return
            }
            if (json.cat == 'aubuf' && json.name == 'cur_sz_ms' && json.id) {
                const d: Data = { id: 'aubuf_'+json.id, x: ts, y: json.args.cur_sz_ms }
                handleNewData(d, 500)
                return
            }
            if (json.cat == 'aubuf') {
                if (json.name == 'overrun' || json.name == 'underrun') {
                    const d: Data = { id: json.id + json.name, x: ts, y: 1 }
                    handleNewData(d)
                    return
                }
            }
            if (json.cat == 'jbuf') {
                const d: Data = { id: "jbuf_" + json.id + "_" + json.name, x: ts, y: json.args[json.name] }
                handleNewData(d)
            }
        }
    },

    close(): void {
        chartsById = {}
        this.socket?.close()
    }
}
