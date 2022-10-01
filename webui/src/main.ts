import { createApp } from "vue";
import App from "./App.vue";
import "./index.css";
import "typeface-roboto-mono";
import ButtonPrimary from "./components/ButtonPrimary.vue";
import ButtonSecondary from "./components/ButtonSecondary.vue";
import { tracks } from "./states/tracks";
import { config } from "./config";
import './shortcuts'

tracks.websocket(config.ws_host());

const app = createApp(App);
app.component("ButtonPrimary", ButtonPrimary);
app.component("ButtonSecondary", ButtonSecondary);
app.mount("#app");

document.addEventListener("contextmenu", function (e) {
    e.preventDefault();
});
