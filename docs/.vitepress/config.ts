import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
    title: "Studio Link",
    description: "Documentation",
    themeConfig: {
        // https://vitepress.dev/reference/default-theme-config
        nav: [
            { text: 'Home', link: '/' },
            { text: 'Examples', link: '/markdown-examples' }
        ],

        sidebar: [
            {
                text: 'Examples',
                items: [
                    { text: 'Markdown Examples', link: '/markdown-examples' },
                    { text: 'Runtime API Examples', link: '/api-examples' }
                ]
            }
        ],

        socialLinks: [
            { icon: 'github', link: 'https://github.com/studio-link/next' }
        ],

        search: {
            provider: 'local'
        }
    },
    locales: {
        root: {
            label: 'English',
            lang: 'en'
        },
        de: {
            label: 'German',
            lang: 'de', // optional, will be added  as `lang` attribute on `html` tag
            link: '/de/' // default /fr/ -- shows on navbar translations menu, can be external
        }
    }
})
