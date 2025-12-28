<script setup>

import { ref, toRaw, onMounted, onUnmounted } from 'vue'

const haveChanges = ref(false);
const schedule = ref(null);

function doClose() {
  fetch('/api/close');
}

function doOpen() {
  fetch('/api/open');
}

function onChange(direction, newSchedule) {
  schedule.value[direction] = newSchedule;
  haveChanges.value = true;
}

function getSchedule() {
   fetch(`/schedule.json`)
    .then(res => res.json())
    .then(scheduleRes => {
      schedule.value = scheduleRes;
      haveChanges.value = false;
    });
}

function postSchedule() {
  const rawSchedule = toRaw(schedule.value);
  const content = JSON.stringify(rawSchedule);
  const blob = new Blob([content], { type: "application/json" }); 
  const formData = new FormData();
  formData.append('file', blob, 'schedule.json');

  fetch(`/`, {
    method: 'POST', 
    body: formData
  })
  .then(() => fetch('/api/readSchedule'))
  .then(() => getSchedule());
}

let intervalId;

onMounted(() => {
  intervalId = setInterval(() => {
    if(haveChanges.value == true) {
      postSchedule();
    }
  }, 2000); 
});

onUnmounted(() => {
  clearInterval(intervalId);
   console.log("not doing it any more")
})



getSchedule();


</script>

<template>
<main>
  <div id="controls">
    <span @click="doOpen">
      <svg class="icon" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 640"><!--!Font Awesome Free v7.1.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free Copyright 2025 Fonticons, Inc.--><path d="M320 576C461.4 576 576 461.4 576 320C576 178.6 461.4 64 320 64C178.6 64 64 178.6 64 320C64 461.4 178.6 576 320 576zM441 335C450.4 344.4 450.4 359.6 441 368.9C431.6 378.2 416.4 378.3 407.1 368.9L320.1 281.9L233.1 368.9C223.7 378.3 208.5 378.3 199.2 368.9C189.9 359.5 189.8 344.3 199.2 335L303 231C312.4 221.6 327.6 221.6 336.9 231L441 335z"/></svg>
    </span>
    <span @click="doClose">
      <svg class="icon" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 640"><!--!Font Awesome Free v7.1.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free Copyright 2025 Fonticons, Inc.--><path d="M320 64C178.6 64 64 178.6 64 320C64 461.4 178.6 576 320 576C461.4 576 576 461.4 576 320C576 178.6 461.4 64 320 64zM199 305C189.6 295.6 189.6 280.4 199 271.1C208.4 261.8 223.6 261.7 232.9 271.1L319.9 358.1L406.9 271.1C416.3 261.7 431.5 261.7 440.8 271.1C450.1 280.5 450.2 295.7 440.8 305L337 409C327.6 418.4 312.4 418.4 303.1 409L199 305z"/></svg>
    </span>
  </div>
  <div id="heading">
    <p>Schedule  -- haveChanges: {{ haveChanges }}</p>
  </div>
  <div id="schedule">
    <span>
      <Schedule :schedule="schedule.open" @change="(schedule) => onChange('open', schedule)" />
    </span>
    <span>
      <Schedule :schedule="schedule.close" @change="(schedule) => onChange('close', schedule)"/>
    </span>
  </div>
</main>
</template>


<style scoped>
  main {
    display: flex;
    flex-direction: column;
    box-sizing: border-box;
    width: 100vw;
    height: 100vh;
    font-size:1rem;
    background-color: #222233;
    color: white;
    user-select: none;
    max-width: 400px;
    margin-left: auto;
    margin-right: auto;
    position: relative;
    overflow: hidden;
  }

  main > .full {
    position: absolute;
    width: 100vw;
    height: 100vh;
    background-color: rgba(0,0,0,0.75);
    max-width: 400px;
    margin-left: auto;
    margin-right: auto;
  }

  #controls {
    display: grid;
    grid-template-columns: 50% 50%;
  }

  #controls > span {
    position: relative;
  }
  
  #controls .icon {
    fill: #007bff;
  }

  #schedule {
    display: grid;
    grid-template-columns: 50% 50%;
  }
  
  #schedule > span {
    position: relative;
  }

  #heading {
    display: flex;
    justify-content: center; 
    align-items: center;     
    height: 6vh;           
  }

</style>
