import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'
import { fileURLToPath } from 'url' 

export default defineConfig({
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
      
    },
  },
  plugins: [
    vue(),
    vueDevTools(),
  ],
  css: {
    preprocessorOptions: {
      scss: {
        additionalData: '@use "@/scss" as *;'
      }
    }
  },
  server: {
    host: '0.0.0.0',
    port: 5173,
    allowedHosts: [
      'testserver.simplysmart.duckdns.org',
      'websocket.simplysmart.duckdns.org'
    ],
    /**
     * Configure HMR to work when site is accessed via HTTPS reverse proxy domain.
     * If VITE_HMR_HOST is set, use it; falls back to first allowed host.
     * clientPort 443 ensures browser connects via standard HTTPS port through proxy.
     */
    hmr: {
      host: process.env.VITE_HMR_HOST || 'website.simplysmart.duckdns.org',
      protocol: 'wss',
      clientPort: 443,
    },
    watch: {
      usePolling: true
    }
  },
})