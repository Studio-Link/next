<!-- This example requires Tailwind CSS v2.0+ -->
<template>
    <!--
    This example requires updating your template:

    ```
    <html class="h-full bg-gray-50">
    <body class="h-full overflow-hidden">
    ```
  -->

    <!-- Static sidebar for desktop -->
    <div class="flex flex-shrink-0 mr-2">
        <div class="flex w-16 flex-col">
            <div class="flex min-h-0 flex-1 flex-col overflow-y-auto bg-sl-01dpa ">
                <div class="flex-1">
                    <nav aria-label="Sidebar" class="flex flex-col items-center space-y-3 py-6">
                        <a href="#" class="relative block group -mt-2">
                            <div class="absolute flex items-center h-full -left-2">
                                <div class="h-10 w-1 transition-all duration-200 origin-left bg-sl-primary rounded-r">
                                </div>
                            </div>
                            <div class="group-active:translate-y-px">
                                <div
                                    class="flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden rounded-2xl text-white hover:bg-sl-08dpa">
                                    <img class="h-10" src="/logo_solo.svg" alt="Studio Link - Default" />
                                    <span class="sr-only">Studio Link Default</span>
                                </div>
                            </div>
                        </a>
                        <hr class="w-2/3 border-t-white/[.06] border-t-2 rounded mx-2" />
                        <a href="#" class="relative block group">
                            <div class="absolute flex items-center h-full -left-2">
                                <div
                                    class="h-5 scale-0 opacity-0 group-hover:opacity-100 group-hover:scale-100 w-1 transition-all duration-200 origin-left bg-sl-primary rounded-r">
                                </div>
                            </div>
                            <div class="group-active:translate-y-px">
                                <div
                                    class="text-gray-100 group-hover:rounded-2xl group-hover:bg-brand group-hover:text-white bg-sl-02dpa rounded-3xl flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden">
                                    <img alt="Studio Metaebene" class="rounded-lg"
                                        src="https://mein.podstock.de/storage/small/logos/7GakpLlUWvPYH6NlzHEBD88wZiI6TnKRc1hX56Ds.png" />
                                    <span class="sr-only">Podstock</span>
                                </div>
                            </div>
                        </a>
                        <a href="#" class="relative block group">
                            <div class="absolute flex items-center h-full -left-4">
                                <div
                                    class="h-5 scale-0 opacity-0 group-hover:opacity-100 group-hover:scale-100 w-1 transition-all duration-200 origin-left bg-sl-primary rounded-r">
                                </div>
                            </div>
                            <div class="group-active:translate-y-px">
                                <div
                                    class="text-sl-surface group-hover:rounded-2xl group-hover:bg-brand group-hover:text-sl-disabled bg-sl-02dpa rounded-3xl flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden">
                                    <svg aria-hidden="true" xmlns="http://www.w3.org/2000/svg" fill="none"
                                        viewBox="0 0 24 24" stroke="currentColor" class="h-20 w-20">
                                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                                            d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
                                    </svg>
                                    <span class="sr-only">New Studio Link</span>
                                </div>
                            </div>
                        </a>
                    </nav>
                </div>
                <div class="flex flex-shrink-0 pb-5">
                    <a href="#" class="w-full flex-shrink-0" @click="oauth">
                        <img class="mx-auto block h-10 w-10 rounded-full" :src="user.imageUrl" alt="" />
                        <div class="sr-only">
                            <p>{{ user.name }}</p>
                            <p>Account settings</p>
                        </div>
                    </a>
                </div>
            </div>
        </div>
    </div>

</template>

<script setup lang='ts'>
import {
    BookmarkSquareIcon,
    FireIcon,
    HomeIcon,
    InboxIcon,
    UserIcon,
} from '@heroicons/vue/24/outline'

const user = {
    name: 'Emily Selman',
    email: 'emily.selman@example.com',
    imageUrl:
        'https://images.unsplash.com/photo-1502685104226-ee32379fefbe?ixlib=rb-1.2.1&ixid=eyJhcHBfaWQiOjEyMDd9&auto=format&fit=facearea&facepad=2&w=256&h=256&q=80',
}
const navigation = [
    { name: 'Home', href: '#', icon: HomeIcon },
    { name: 'Trending', href: '#', icon: FireIcon },
    { name: 'Bookmarks', href: '#', icon: BookmarkSquareIcon },
    { name: 'Messages', href: '#', icon: InboxIcon },
    { name: 'Profile', href: '#', icon: UserIcon },
]

function oauth() {
    var uri = 'https://my.studio.link/'
    var left = (screen.width / 2 - 400)
    var top = (screen.height / 2 - 350)
    window.open(uri, 'oauth window', 'resizable=yes, width=800, height=700, top=' + top + ', left=' + left)
}
</script>
