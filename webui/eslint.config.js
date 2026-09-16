// Author: kelexine <https://github.com/kelexine>
import globals from 'globals';

export default [
    {
        files: ['**/*.js'],
        ignores: ['node_modules/**', '../webroot/**'],
        languageOptions: {
            ecmaVersion: 2022,
            sourceType: 'module',
            globals: { ...globals.browser },
        },
        rules: {
            'no-unused-vars': 'warn',
            'no-undef': 'error',
        },
    },
];
