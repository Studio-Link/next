import uPlot from 'uplot'

interface Debug {
    socket?: WebSocket
    websocket(ws_host: string): void
}

interface ChartStore {
    uplot: uPlot
    xs: number[]
    ys: number[]
}

interface Data {
    id: string
    x: number
    y: number
}

const chartsById: Record<string, ChartStore> = {}
const maxPoints = 200


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

    const { width, height } = container.getBoundingClientRect();

    const opts: uPlot.Options = {
        title: `${id}`,
        width: width,
        height: height - 60,
        scales: { x: { time: false } },
        series: [
            { label: "ts" },
            { label: "value", stroke: "#f27800" },
        ],
        axes: [
            { label: "timestamp (ms)", stroke: "white" },
            { label: "value", stroke: "white" },
        ],
    };

    const u = new uPlot(opts, [xs, ys], chartEl);
    chartsById[id] = { uplot: u, xs, ys };
}


function handleNewData(d: Data): void {
    const id = d.id;
    if (!chartsById[id]) {
        createChart(id);
    }

    const { uplot, xs, ys } = chartsById[id];

    xs.push(d.x);
    ys.push(d.y);

    if (xs.length > maxPoints) {
        xs.shift();
        ys.shift();
    }

    uplot.setData([xs, ys]);
}


export const Debug: Debug = {
    websocket(ws_host): void {
        this.socket = new WebSocket('ws://' + ws_host + '/ws/v1/debug')
        this.socket.onerror = function () {
            console.log('Websocket error')
        }
        this.socket.onmessage = (message) => {
            const json = JSON.parse(message.data)
            if (json.cat == 'slmain') {
                const d: Data = { id: 'max_jitter', x: json.ts / 1000 / 1000, y: json.args.max_jitter }
                handleNewData(d)
            }
        }
    }
}
