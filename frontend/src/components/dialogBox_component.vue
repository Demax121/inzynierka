<template>
  <dialog ref="dialogEl" id="dialogbox" class="dialogbox" @close="handleNativeClose" closedby="any">
    <div class="dialogbox__body">
      <p v-if="mode !== 'hidden'" class="dialogbox__content">{{ message }}</p>
      <div v-if="mode === 'message'" class="dialogbox__actions">
        <button type="button" class="btn" @click="close()">OK</button>
      </div>
      <div v-else-if="mode === 'confirm'" class="dialogbox__actions">
        <button type="button" class="btn btn--danger" @click="confirmYes">Yes</button>
        <button type="button" class="btn btn--secondary" command="close" commandfor="dialogbox" @click="close()">No</button>
      </div>
      <slot />
    </div>
  </dialog>
</template>

<script setup>
import { ref, defineExpose } from 'vue'

// Reactive state
const dialogEl = ref(null)
const message = ref('')
const mode = ref('hidden') // 'hidden' | 'message' | 'confirm'
let confirmResolve = null

// Public API
function lockScroll() {
  const body = document.body
  if (!body.dataset._dialogScrollLocked) {
    body.dataset._dialogScrollLocked = '1'
    body.style.top = `-${window.scrollY}px`
    body.classList.add('dialog-scroll-lock')
  }
}


function openMessage(msg) {
  message.value = msg
  mode.value = 'message'
  if (!dialogEl.value.open) {
    dialogEl.value.showModal()
    lockScroll()
  }
}

function openConfirm(msg) {
  message.value = msg
  mode.value = 'confirm'
  if (!dialogEl.value.open) {
    dialogEl.value.showModal()
    lockScroll()
  }
  return new Promise((resolve) => { confirmResolve = resolve })
}

function close() {
  if (dialogEl.value?.open) dialogEl.value.close()
}

function confirmYes() {
  if (confirmResolve) confirmResolve(true)
  confirmResolve = null
  close()
}

function handleNativeClose() {
  if (confirmResolve) { confirmResolve(false); confirmResolve = null }
  mode.value = 'hidden'
  message.value = ''

}

defineExpose({ openMessage, openConfirm, close })
</script>

<style lang="scss" scoped>
@use "@/scss/colors" as *;

/* Centered modal dialog (override browser default top inset) */
// Adjustable variables
$dialog-width: 420px;
$dialog-height: 270px;
$dialog-actions-margin: 1.1rem;
$dialog-bg-color: rgba(255, 255, 255, 0.95);
$dialog-text-color: rgba(0, 0, 0, 0.87);

.dialogbox {
  padding: 1.25rem 1.5rem 1.4rem;
  border: none;
  border-radius: 0.85rem;
  background: $dialog-bg-color;
  color: $dialog-text-color;
  width: $dialog-width;
  max-width: 90vw; // fallback on very narrow screens
  box-shadow: 0 12px 28px -4px rgba(0,0,0,0.55), 0 2px 6px rgba(0,0,0,0.35);
  position: fixed; /* ensure consistent centering */
  inset: 50% auto auto 50%;
  transform: translate(-50%, -50%);
  margin: 0; /* remove default UA margin */
}
/* Body scroll lock helper */
.dialog-scroll-lock {
  position: fixed;
  overflow-y: hidden;
  width: 100%;
}

::backdrop { background: rgba(0,0,0,0.55); backdrop-filter: blur(2px); }

.dialogbox__body { display:flex; flex-direction:column; gap:1rem; }
.dialogbox__content { margin:0; line-height:1.4; font-size:0.95rem; }
.dialogbox__actions { display:flex; justify-content:flex-end; gap:0.75rem; margin-top: $dialog-actions-margin; }

.btn { font-size:0.8rem; padding:0.55rem 0.9rem; font-weight:600; }
.btn--danger { background:#f44336; color:#fff; }
.btn--secondary { background:#555; color:#fff; }

</style>