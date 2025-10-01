// Plik: frontend/src/stores/linkStore.js
import { defineStore } from 'pinia';

// Używamy zmiennych środowiskowych zdefiniowanych w pliku .env.development
const backendPrefix = import.meta.env.VITE_BACKEND_URL_PREFIX;
const cdnPrefix = import.meta.env.VITE_CDN_URL_PREFIX;
const wledIp = import.meta.env.VITE_WLED_IP;

export const useLinkStore = defineStore('linkStore', {
  state: () => ({
    links: {
      databaseApi: backendPrefix,
      cdnURL: cdnPrefix,
      wledIP: wledIp,
    },
    wledPresets: [],
    wledPresetsLoading: false,
    wledPresetsLoaded: false,
  }),
  
  getters: {
    getLink: (state) => (linkName) => state.links[linkName],
    
    getImage: (state) => (imageName) => {
      if (!imageName || typeof imageName !== 'string') return null;
      return `${state.links.cdnURL}/images/${imageName}`;
    },
    
    getFile: (state) => (fileName) => {
      if (!fileName || typeof fileName !== 'string') return null;
      return `${state.links.cdnURL}/config_files/${fileName}`;
    },

    getPhpApiUrl: (state) => (phpFileName) => {
      if (!phpFileName || typeof phpFileName !== 'string') return state.links.databaseApi;
      return `${state.links.databaseApi}/${phpFileName}`;
    },
  },
  
  actions: {
    async fetchWledPresets() {
      const WLED_PRESETS_ENDPOINT = "/json/presets";
      this.wledPresetsLoading = true;
      this.wledPresets = [];
      
      try {
        const response = await fetch(`${this.links.wledIP}${WLED_PRESETS_ENDPOINT}`);
        
        if (!response.ok) {
          throw new Error(`HTTP error! Status: ${response.status}`);
        }
        
        const data = await response.json();
        
        if (data) {
          const processedPresets = [];
          
          Object.entries(data).forEach(([key, preset]) => {
            const presetId = parseInt(key);
            if (!isNaN(presetId) && presetId !== 0) {
              processedPresets.push({
                id: presetId,
                name: preset.n || `Preset ${key}`
              });
            }
          });
          
          processedPresets.sort((a, b) => a.id - b.id);
          this.wledPresets = processedPresets;
          this.wledPresetsLoaded = true;
        } else {
          console.log("No presets found or invalid data format");
        }
      } catch (error) {
      } finally {
        this.wledPresetsLoading = false;
      }
    },
    
    async sendWledCommand(payload) {
      const WLED_STATE_ENDPOINT = "/json/state";
      this.wledPresetsLoading = true;
      
      try {
        const response = await fetch(`${this.links.wledIP}${WLED_STATE_ENDPOINT}`, {
          method: "POST",
          headers: {
            "Content-Type": "application/json"
          },
          body: JSON.stringify(payload)
        });
        
        const data = await response.json();
        return data;
      } catch (error) {
        throw error;
      } finally {
        this.wledPresetsLoading = false;
      }
    }
  }
});