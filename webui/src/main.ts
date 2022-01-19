import { createApp } from "vue";
import App from "./App.vue";
import "./index.css";
import "typeface-roboto-mono";
import Button from "./components/Button.vue";
import { tracks } from "./states/tracks";
import { config } from "./config";
import './shortcuts'

tracks.websocket(config.ws_host());

let handleClickOutside: any;

const app = createApp(App);
app.directive("click-outside", {
  beforeMount(el, binding) {
    handleClickOutside = (e: any) => {
      e.stopPropagation();
      const { handler, exclude } = binding.value;
      let clickedOnExcludedEl = false;

      // Gives you the ability to exclude certain elements if you want,
      // pass as array of strings to exclude
      if (exclude) {
        exclude.forEach((refName: string) => {
          if (!clickedOnExcludedEl) {
            if (binding != null && binding.instance != null) {
              const excludedEl: any = binding.instance.$refs[refName];
              clickedOnExcludedEl = excludedEl.contains(e.target);
            }
          }
        });
      }

      if (!el.contains(e.target) && !clickedOnExcludedEl) {
        handler(e);
      }
    };
    document.addEventListener("click", handleClickOutside);
    document.addEventListener("touchstart", handleClickOutside);
  },
  unmounted() {
    document.removeEventListener("click", handleClickOutside);
    document.removeEventListener("touchstart", handleClickOutside);
  },
});
app.component("Button", Button);
app.mount("#app");

document.addEventListener("contextmenu", function(e) {
e.preventDefault();
});
