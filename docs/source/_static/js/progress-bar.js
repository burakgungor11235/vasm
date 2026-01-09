(function() {
    'use strict';

    const TUTORIAL_ORDER = [
        '01-introduction',
        '02-instruction-set',
        '03-memory-model',
        '04-syscalls',
        '05-examples'
    ];

    const STORAGE_KEY = 'varm_tutorial_progress';
    const PROGRESS_BAR_SELECTOR = 'progress-bar-container';
    const TEAL_COLOR = '#009d9a';

    function getCurrentPage() {
        const path = window.location.pathname;
        const filename = path.split('/').pop() || 'index.html';

        for (const page of TUTORIAL_ORDER) {
            if (filename.includes(page)) {
                return page;
            }
        }
        return null;
    }

    function getProgressIndex() {
        const currentPage = getCurrentPage();
        if (!currentPage) return -1;

        const index = TUTORIAL_ORDER.indexOf(currentPage);
        return index !== -1 ? index : -1;
    }

    function calculateProgress() {
        const index = getProgressIndex();
        if (index === -1) return 0;

        const total = TUTORIAL_ORDER.length;
        const completed = index + 1;
        return Math.round((completed / total) * 100);
    }

    function calculateCounts() {
        const index = getProgressIndex();
        if (index === -1) return { completed: 0, total: TUTORIAL_ORDER.length };

        return {
            completed: index + 1,
            total: TUTORIAL_ORDER.length
        };
    }

    function loadStoredProgress() {
        try {
            const stored = localStorage.getItem(STORAGE_KEY);
            return stored ? JSON.parse(stored) : {};
        } catch (e) {
            return {};
        }
    }

    function saveProgress(page, percentage) {
        try {
            const progress = loadStoredProgress();
            progress[page] = percentage;
            localStorage.setItem(STORAGE_KEY, JSON.stringify(progress));
        } catch (e) {
            console.warn('Failed to save progress to localStorage:', e);
        }
    }

    function createProgressBar() {
        const container = document.createElement('div');
        container.id = PROGRESS_BAR_SELECTOR;
        container.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 4px;
            background: transparent;
            z-index: 9999;
            font-family: 'IBM Plex Sans', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
        `;

        const bar = document.createElement('div');
        bar.id = 'progress-bar-fill';
        bar.style.cssText = `
            height: 100%;
            width: 0%;
            background: ${TEAL_COLOR};
            transition: width 0.4s cubic-bezier(0.4, 0, 0.2, 1);
            border-radius: 0 2px 2px 0;
        `;

        const text = document.createElement('div');
        text.id = 'progress-text';
        text.style.cssText = `
            position: fixed;
            top: 12px;
            right: 24px;
            font-size: 12px;
            color: #525252;
            opacity: 0;
            transform: translateY(-4px);
            transition: opacity 0.3s ease, transform 0.3s ease;
            font-weight: 500;
            letter-spacing: 0.5px;
        `;

        container.appendChild(bar);
        container.appendChild(text);

        const existingContainer = document.getElementById(PROGRESS_BAR_SELECTOR);
        if (existingContainer) {
            existingContainer.remove();
        }

        document.body.appendChild(container);

        return { bar, text };
    }

    function updateProgress() {
        const percentage = calculateProgress();
        const counts = calculateCounts();
        const currentPage = getCurrentPage();

        const bar = document.getElementById('progress-bar-fill');
        const text = document.getElementById('progress-text');

        if (bar && text && currentPage) {
            bar.style.width = `${percentage}%`;

            text.textContent = `${counts.completed} of ${counts.total} complete`;
            text.style.opacity = '1';
            text.style.transform = 'translateY(0)';

            saveProgress(currentPage, percentage);
        }
    }

    function init() {
        const currentPage = getCurrentPage();
        if (!currentPage) return;

        const { bar, text } = createProgressBar();

        setTimeout(() => {
            updateProgress();
        }, 50);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

    window.addEventListener('popstate', () => {
        setTimeout(updateProgress, 100);
    });

    const originalPushState = history.pushState;
    history.pushState = function(...args) {
        originalPushState.apply(this, args);
        setTimeout(updateProgress, 100);
    };
})();
