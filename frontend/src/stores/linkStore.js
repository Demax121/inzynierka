/**
 * Link Store - Centralized management of API endpoints and URLs
 * 
 * This Pinia store manages application URLs including database API endpoints
 * and CDN URLs. Provides getters for URL construction.
 */
import { defineStore } from 'pinia';
import { ref } from 'vue';

export const useLinkStore = defineStore('linkStore', {
  // State: Holds the base URLs for different services
  state: () => ({
    links: {
      databaseApi: 'http://192.168.1.4:8884/',  // Base URL for PHP API endpoints
      cdnURL: 'http://192.168.1.4:8885/',       // Base URL for CDN/static assets
      wledIP: 'http://192.168.1.25',            // WLED device IP
    },
    // WLED state variables
    wledPresets: [],
    wledPresetsLoading: false,
    wledPresetsLoaded: false,
  }),
  
  getters: {
    // Getter: Retrieves a specific link by name from the links object
    getLink: (state) => (linkName) => state.links[linkName],
    
    // Getter: Constructs full image URLs from CDN
    getImage: (state) => (imageName) => {
      if (!imageName || typeof imageName !== 'string') {
        console.warn('Image name must be a non-empty string.');
        return null;
      }
      return `${state.links.cdnURL}images/${imageName}`;
    },
    
    getFile: (state) => (fileName) => {
      if (!fileName || typeof fileName !== 'string') {
        console.warn('File name must be a non-empty string.');
        return null;
      }
      return `${state.links.cdnURL}config_files/${fileName}`;
    },

    // Getter: Constructs full PHP API URLs by combining base API URL with filename
    getPhpApiUrl: (state) => (phpFileName) => {
      if (!phpFileName || typeof phpFileName !== 'string') {
        console.warn('PHP filename must be a non-empty string.');
        return state.links.databaseApi;
      }
      
      return `${state.links.databaseApi}${phpFileName}`;
    },
    
    // Getter: Get WLED presets
    getWledPresets: (state) => () => {
      return {
        presets: state.wledPresets,
        loading: state.wledPresetsLoading,
        loaded: state.wledPresetsLoaded
      };
    }
  },
  
  actions: {
    // Action to fetch WLED presets
    async fetchWledPresets() {
      const WLED_PRESETS_ENDPOINT = "/presets.json";
      this.wledPresetsLoading = true;
      this.wledPresets = [];
      
      try {
        console.log("Fetching WLED presets...");
        const response = await fetch(`${this.links.wledIP}${WLED_PRESETS_ENDPOINT}`);
        
        if (!response.ok) {
          throw new Error(`HTTP error! Status: ${response.status}`);
        }
        
        const data = await response.json();
        console.log("WLED presets:", data);
        
        // Process presets data
        if (data) {
          const processedPresets = [];
          
          Object.entries(data).forEach(([key, preset]) => {
            // Filter numeric keys which represent presets and exclude Preset 0
            const presetId = parseInt(key);
            if (!isNaN(presetId) && presetId !== 0) {
              processedPresets.push({
                id: presetId,
                name: preset.n || `Preset ${key}` // Use preset name or default
              });
            }
          });
          
          // Sort presets by ID
          processedPresets.sort((a, b) => a.id - b.id);
          this.wledPresets = processedPresets;
          this.wledPresetsLoaded = true;
          console.log(`Found ${processedPresets.length} presets`);
        } else {
          console.log("No presets found or invalid data format");
        }
      } catch (error) {
        console.error("Error fetching WLED presets:", error);
      } finally {
        this.wledPresetsLoading = false;
      }
    },
    
    // Action to send command to WLED
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
        console.log("WLED response:", data);
        return data;
      } catch (error) {
        console.error("Error sending command to WLED:", error);
        throw error;
      } finally {
        this.wledPresetsLoading = false;
      }
    }
  }
});