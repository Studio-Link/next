import { createRouter, createWebHistory } from 'vue-router'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      name: 'Home',
      component: () => import('../views/TrackView.vue'),
    },
    {
      path: '/debug',
      name: 'Debug',
      component: () => import('../views/DebugView.vue'),
    },
  ],
})

export default router
