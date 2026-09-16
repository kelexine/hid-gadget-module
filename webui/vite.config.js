import { defineConfig } from 'vite';

// Author: kelexine <https://github.com/kelexine>
export default defineConfig({
    base: './',
    build: {
        outDir: '../webroot',
        emptyOutDir: true,
    },
});
