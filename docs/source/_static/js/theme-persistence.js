(function() {
    'use strict';

    const MODE_KEY = 'mode';
    const THEME_KEY = 'theme';

    function getPreferredTheme() {
        const storedMode = localStorage.getItem(MODE_KEY);
        if (storedMode) {
            return storedMode;
        }

        const storedTheme = localStorage.getItem(THEME_KEY);
        if (storedTheme) {
            return storedTheme;
        }

        return 'dark';
    }

    function applyTheme(mode) {
        const html = document.documentElement;

        html.dataset.mode = mode;

        if (mode === 'auto') {
            const systemPrefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
            html.dataset.theme = systemPrefersDark ? 'dark' : 'light';
        } else {
            html.dataset.theme = mode;
        }
    }

    function saveTheme(mode) {
        try {
            localStorage.setItem(MODE_KEY, mode);
        } catch (e) {
            console.warn('Failed to save theme to localStorage:', e);
        }
    }

    function setupThemeSwitcher() {
        const switcherButtons = document.querySelectorAll('.theme-switch-button');

        switcherButtons.forEach(button => {
            button.addEventListener('click', function(e) {
                const currentMode = document.documentElement.dataset.mode || 'dark';
                let newMode;

                if (currentMode === 'light') {
                    newMode = 'dark';
                } else if (currentMode === 'dark') {
                    newMode = 'auto';
                } else {
                    newMode = 'light';
                }

                applyTheme(newMode);
                saveTheme(newMode);
            });
        });
    }

    function updateThemeSwitcherIcons() {
        const currentMode = document.documentElement.dataset.mode || 'dark';
        const icons = document.querySelectorAll('.theme-switch');

        icons.forEach(icon => {
            const iconMode = icon.dataset.mode;
            if (iconMode === currentMode) {
                icon.style.display = 'inline';
            } else {
                icon.style.display = 'none';
            }
        });
    }

    function init() {
        const mode = getPreferredTheme();
        applyTheme(mode);

        setupThemeSwitcher();
        updateThemeSwitcherIcons();

        const observer = new MutationObserver(function(mutations) {
            mutations.forEach(function(mutation) {
                if (mutation.type === 'attributes' && mutation.attributeName === 'data-mode') {
                    updateThemeSwitcherIcons();
                }
            });
        });

        observer.observe(document.documentElement, { attributes: true });

        window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function(e) {
            const currentMode = document.documentElement.dataset.mode;
            if (currentMode === 'auto') {
                applyTheme('auto');
            }
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
