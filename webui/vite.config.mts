import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import tailwindcss from '@tailwindcss/vite'


function manualChunks(id) {
    if (id.endsWith(".css")) {
        return;
    }
    if (id.includes("/node_modules/")) {
        return "vendor";
    }
}

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [vue(), tailwindcss()],
    build: {
        rollupOptions: {
            output: {
                entryFileNames: `[name].js`,
                chunkFileNames: `[name].js`,
                assetFileNames: `[name].[ext]`,
                manualChunks: manualChunks
            },
        }
    }
})
