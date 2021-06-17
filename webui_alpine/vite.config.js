const { posthtmlPlugin } = require("vite-plugin-posthtml");
const vitePluginPostHTMLComponent = require("./vite-plugin-component.js");
const include = require("posthtml-include");
const expressions = require("posthtml-expressions");
require('dotenv').config({ path: './.env' });

export default {
  build: {
    manifest: true,
    //     rollupOptions: {
    //       output: {
    //         manualChunks: undefined,
    //         entryFileNames: `assets/[name].js`,
    //         chunkFileNames: `assets/[name].js`,
    //         assetFileNames: `assets/[name].[ext]`, // Does not work (use public folder)
    //       },
    //     },
  },
  plugins: [
    posthtmlPlugin({
      plugins: [
        include({ root: "./src", encoding: "utf-8" }),
        expressions({
          locals: {
            APP_TITLE: process.env.VITE_APP_TITLE,
            APP_VERSION: process.env.VITE_APP_VERSION
          },
        }),
      ],
    }),
    vitePluginPostHTMLComponent(),
  ],
};
