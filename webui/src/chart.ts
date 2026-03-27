type ChartData = {
    x: number[],
    y: number[]
};

export class CanvasLineChart {
    private canvas: HTMLCanvasElement;
    private ctx: CanvasRenderingContext2D;
    private padding = 20;
    private id: string;
    private data: ChartData;

    constructor(canvasId: string) {
        const canvas = document.getElementById(canvasId) as HTMLCanvasElement;
        const ctx = canvas.getContext("2d");

        if (!canvas || !ctx) {
            throw new Error("Canvas or context not found");
        }

        this.canvas = canvas;
        this.ctx = ctx;
        this.id = canvasId;
        this.data = { x: [], y: [] };
    }

    private realDraw() {
        const { x, y } = this.data;

        const width = this.canvas.width;
        const height = this.canvas.height;

        const chartWidth = width - this.padding * 2;
        const chartHeight = height - this.padding * 2;

        const maxValue = Math.max(...y);
        const minValue = Math.min(...y);
        const xScale = chartWidth / (x[x.length - 1] - x[0]);
        this.clear();
        this.drawAxes();

        this.ctx.fillStyle = "#888";
        this.ctx.font = "14px mono";
        this.ctx.fillText(maxValue.toString()+ "ms", this.padding + 25, this.padding - 5);
        this.ctx.fillText(minValue.toString()+ "ms", this.padding + 25, height - this.padding - (minValue / maxValue) * chartHeight + 12);
        this.ctx.font = "16px mono";
        this.ctx.textAlign = "right"
        this.ctx.fillText(this.id, width - this.padding, this.padding - 5);

        this.ctx.beginPath();
        this.ctx.lineWidth = 2;

        x.forEach((ts, i) => {
            const xv = this.padding + (ts - x[0]) * xScale;
            const yv =
                height - this.padding - (y[i] / maxValue) * chartHeight;

            if (i === 0) {
                this.ctx.moveTo(xv, yv);
            } else {
                this.ctx.lineTo(xv, yv);
            }

            // draw point
            this.ctx.fillStyle = "#df7c00";
            this.ctx.beginPath();
            this.ctx.arc(xv, yv, 2, 0, Math.PI * 2);
            this.ctx.fill();
        });

    }

    draw(data: ChartData) {
        this.data = data;
        requestAnimationFrame(() => this.realDraw())
    }

    private drawAxes() {
        const width = this.canvas.width;
        const height = this.canvas.height;

        this.ctx.beginPath();
        this.ctx.moveTo(this.padding, this.padding);
        this.ctx.lineTo(this.padding, height - this.padding);
        this.ctx.lineTo(width - this.padding, height - this.padding);
        this.ctx.strokeStyle = "#000";
        this.ctx.stroke();
    }

    private clear() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    }
}
