import './assets/reset.css'

import { createApp } from 'vue'
import App from './App.vue'
import Schedule from './components/Schedule.vue'

createApp(App)
  .component('Schedule', Schedule)
  .mount('#app')
