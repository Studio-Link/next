import uPlot from 'uplot'

interface Debug {
    socket?: WebSocket
    websocket(ws_host: string): void
    close(): void
}

interface ChartStore {
    uplot: uPlot
    xs: number[]
    ys: number[]
    zs: number[]
}

interface Data {
    id: string
    x: number
    y?: number
    z?: number
}

let chartsById: Record<string, ChartStore> = {}
const maxPoints = 1000


function createChart(id: string): void {
    const container = document.createElement("div");
    container.className = "chart-container bg-sl-01dpa h-96 text-gray-500";

    /*
    const title = document.createElement("h3");
    title.textContent = `ID: ${id}`;
    container.appendChild(title);
    */

    const chartEl = document.createElement("div");
    chartEl.className = "chart";
    container.appendChild(chartEl);

    document.getElementById("charts")!.appendChild(container);

    const xs: number[] = [];
    const ys: number[] = [];
    const zs: number[] = [];

    const { width, height } = container.getBoundingClientRect();

    const opts: uPlot.Options = {
        title: `${id}`,
        width: width,
        height: height - 60,
        scales: { x: { time: false } },
        series: [
            { label: "ts" },
            { label: "ms", stroke: "#f27800" },
            { label: "over/underrun", stroke: "red",
      points: {
        show: true,
        size: 10,          // size of the point
      },

            },
        ],
        axes: [
            { label: "timestamp (ms)", stroke: "white" },
            { label: "value", stroke: "white" },
        ],
    };

    const u = new uPlot(opts, [xs, ys, zs], chartEl);
    chartsById[id] = { uplot: u, xs, ys, zs };
}


function handleNewData(d: Data): void {
    const id = d.id;
    if (!chartsById[id]) {
        createChart(id);
    }

    const { uplot, xs, ys, zs } = chartsById[id];

    xs.push(d.x);
    if (d.y)
        ys.push(d.y);
    if (d.z)
        zs.push(d.z)

    if (xs.length > maxPoints) {
        xs.shift();
        ys.shift();
        zs.shift();
    }

    uplot.setData([xs, ys, zs]);
}


export const Debug: Debug = {
    websocket(ws_host): void {
        this.socket = new WebSocket('ws://' + ws_host + '/ws/v1/debug')
        this.socket.onerror = function () {
            console.log('Websocket error')
        }
        this.socket.onmessage = (message) => {
            const json = JSON.parse(message.data)
            const ts = json.ts / 1000 / 1000
            if (json.cat == 'slmain') {
                const d: Data = { id: 'max_jitter', x: ts, y: json.args.max_jitter }
                handleNewData(d)
                return
            }
            if (json.cat == 'aubuf' && json.name == 'cur_sz_ms') {
                const d: Data = { id: json.id, x: ts, y: json.args.cur_sz_ms }
                handleNewData(d)
                return
            }
            if (json.cat == 'aubuf') {
                if (json.name == 'overrun') {
                    const d: Data = { id: json.id, x: ts, z: 100 }
                    handleNewData(d)
                    return
                }
                else if (json.name == 'underrun') {
                    const d: Data = { id: json.id, x: ts, z: 10 }
                    handleNewData(d)
                    return
                }
            }
        }
    },

    close(): void {
        chartsById = {}
        this.socket?.close()
    }
}
