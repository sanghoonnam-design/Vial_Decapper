import { postGotoBoot } from '../api.js';
import { el } from '../ui/elements.js';
import { log } from '../ui/log.js';
import { state } from '../state.js';
function startReloadLoop(intervalMs = 1000) {
  if (state.waitReloadTimer) {
    clearInterval(state.waitReloadTimer);
  }
  state.waitReloadTimer = setInterval(() => {
    location.reload();
  }, intervalMs);
}
export function stopReloadLoop() {
  if (!state.waitReloadTimer) return;
  clearInterval(state.waitReloadTimer);
  state.waitReloadTimer = null;
}
export function initBootActions() {
  if (!el.btnGotoBoot) return;
  el.btnGotoBoot.addEventListener('click', async () => {
    try {
      await postGotoBoot();
      log('gotoBoot OK. Waiting for Boot server...');
      startReloadLoop(1000);
    } catch (err) {
      alert(err.toString());
    }
  });
}
export { startReloadLoop };
