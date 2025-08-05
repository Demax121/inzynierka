/**
 * Link Store - Centralized management of API endpoints and URLs
 * 
 * This Pinia store manages application URLs including database API endpoints
 * and CDN URLs. Provides getters for URL construction.
 */
import { defineStore } from 'pinia';

export const useLinkStore = defineStore('linkStore', {
  // State: Holds the base URLs for different services
  state: () => ({
    links: {
      databaseApi: 'http://localhost:8884/',  // Base URL for PHP API endpoints
      cdnURL: 'http://localhost:8885/',       // Base URL for CDN/static assets
    },
  }),
  
  getters: {
    // Getter: Retrieves a specific link by name from the links object
    getLink: (state) => (linkName) => state.links[linkName],
    
    // Getter: Constructs full PHP API URLs by combining base API URL with filename
    getPhpApiUrl: (state) => (phpFileName) => {
      if (!phpFileName || typeof phpFileName !== 'string') {
        console.warn('PHP filename must be a non-empty string.');
        return state.links.databaseApi;
      }
      
      return `${state.links.databaseApi}${phpFileName}`;
    }
  },
});