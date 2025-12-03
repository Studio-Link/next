import { createApp } from "vue";
import App from "./App.vue";
import "./index.css";
import "typeface-roboto-mono";
import ButtonPrimary from "./components/ButtonPrimary.vue";
import ButtonSecondary from "./components/ButtonSecondary.vue";
import { tracks } from "./states/tracks";
import { meters } from "./states/meters";
import { config } from "./config";
import './shortcuts'
import router from './router'

tracks.websocket(config.ws_host());
meters.websocket(config.ws_host());

const app = createApp(App);
app.component("ButtonPrimary", ButtonPrimary);
app.component("ButtonSecondary", ButtonSecondary);
app.use(router);
app.mount("#app");

document.addEventListener("contextmenu", function (e) {
    e.preventDefault();
});
