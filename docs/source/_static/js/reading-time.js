/**
 * Reading Time Estimation for varm Documentation
 * 
 * Estimates reading time based on word count.
 * Adds a reading time indicator to each page.
 */

(function() {
    'use strict';

    const WORDS_PER_MINUTE = 200;

    /**
     * Calculate reading time for a given text
     * @param {string} text - The text to analyze
     * @returns {object} - { minutes, words }
     */
    function calculateReadingTime(text) {
        // Count words (split on whitespace)
        const words = text.trim().split(/\s+/).filter(word => word.length > 0);
        const wordCount = words.length;
        const minutes = Math.ceil(wordCount / WORDS_PER_MINUTE);
        return { minutes, wordCount };
    }

    /**
     * Get the main content element
     * @returns {Element|null}
     */
    function getMainContent() {
        // Try PyData theme selectors first
        const selectors = [
            '.bd-article',
            '.rst-content',
            'article',
            'main',
            '.document'
        ];
        
        for (const selector of selectors) {
            const element = document.querySelector(selector);
            if (element && element.textContent.trim().length > 100) {
                return element;
            }
        }
        
        return null;
    }

    /**
     * Create reading time element
     * @param {number} minutes - Estimated reading time in minutes
     * @returns {HTMLElement}
     */
    function createReadingTimeElement(minutes) {
        const container = document.createElement('div');
        container.className = 'reading-time';
        container.style.cssText = `
            display: inline-flex;
            align-items: center;
            gap: 6px;
            font-size: 0.875rem;
            color: var(--cds-text-secondary, #c6c6c6);
            font-family: var(--cds-font-sans, -apple-system, BlinkMacSystemFont, sans-serif);
            margin-bottom: 0.5rem;
        `;

        // SVG icon for reading time (clock)
        const icon = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
        icon.setAttribute('width', '16');
        icon.setAttribute('height', '16');
        icon.setAttribute('viewBox', '0 0 24 24');
        icon.setAttribute('fill', 'none');
        icon.setAttribute('stroke', 'currentColor');
        icon.setAttribute('stroke-width', '2');
        icon.innerHTML = '<circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline>';

        const text = document.createElement('span');
        text.textContent = `${minutes} min read`;

        container.appendChild(icon);
        container.appendChild(text);

        return container;
    }

    /**
     * Add reading time to the page
     */
    function addReadingTime() {
        const content = getMainContent();
        if (!content) {
            return;
        }

        // Get text content, excluding certain elements
        const clone = content.cloneNode(true);
        const excludeSelectors = ['script', 'style', 'nav', 'header', 'footer', '.bd-sidebar', '.bd-toc', '.toctree-wrapper'];
        excludeSelectors.forEach(selector => {
            clone.querySelectorAll(selector).forEach(el => el.remove());
        });

        const text = clone.textContent || clone.innerText || '';
        const { minutes } = calculateReadingTime(text);

        // Find a good place to insert the reading time
        // Try the first heading or the article meta area
        const insertPoints = [
            '.article-meta',
            '.docutils',
            '.section',
            'h1',
            '.bd-article-container > .container'
        ];

        let inserted = false;
        for (const selector of insertPoints) {
            const element = document.querySelector(selector);
            if (element) {
                const readingTime = createReadingTimeElement(minutes);
                
                // Try different insertion strategies
                const firstHeading = element.querySelector('h1, h2');
                if (firstHeading && firstHeading.parentNode === element) {
                    element.insertBefore(readingTime, firstHeading);
                    inserted = true;
                    break;
                } else if (element.querySelector('p')) {
                    const firstP = element.querySelector('p');
                    firstP.parentNode.insertBefore(readingTime, firstP);
                    inserted = true;
                    break;
                } else {
                    element.insertBefore(readingTime, element.firstChild);
                    inserted = true;
                    break;
                }
            }
        }

        // Fallback: append to article container
        if (!inserted) {
            const articleContainer = document.querySelector('.bd-article-container, .document');
            if (articleContainer) {
                const readingTime = createReadingTimeElement(minutes);
                readingTime.style.marginTop = '1rem';
                articleContainer.insertBefore(readingTime, articleContainer.firstChild);
            }
        }
    }

    // Run when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', addReadingTime);
    } else {
        // DOM already loaded, but wait for layout
        setTimeout(addReadingTime, 100);
    }

    // Also run when navigating (for SPA-like behavior)
    if (typeof window !== 'undefined') {
        const observer = new MutationObserver(() => {
            const readingTime = document.querySelector('.reading-time');
            if (!readingTime) {
                addReadingTime();
            }
        });
        
        observer.observe(document.body, {
            childList: true,
            subtree: true
        });
    }
})();
