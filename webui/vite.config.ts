import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vitejs.dev/config/
export default defineConfig({
	plugins: [vue()],
	build: {
		rollupOptions: {
			output: {
				entryFileNames: `[name].js`,   // works
				chunkFileNames: `[name].js`,   // works
				assetFileNames: `[name].[ext]` // does not work for images
			}
		}
	}
})
