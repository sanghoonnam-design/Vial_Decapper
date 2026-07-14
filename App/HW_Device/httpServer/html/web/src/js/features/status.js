import { getStatus, postAppConfirm } from '../api.js';
import { el } from '../ui/elements.js';
import { log } from '../ui/log.js';
import { parseKeyValue } from '../utils.js';
import { state } from '../state.js';
async function pollOnce() {
  const txt = await getStatus();
  const data = parseKeyValue(txt);

  el.modelName.innerText = data.model || 'Unknown';
  el.appConfirmed.innerText = data.status || 'None';
  el.rtcTime.innerText = data.time || '';

  if (data.status === 'NO') {
    startAutoAppConfirm();
  } else {
    stopAutoAppConfirm();
  }
}
export function startAutoAppConfirm() {
  if (state.appConfirmTimer) return;
  log('Auto appConfirm started');
  state.appConfirmTimer = setInterval(async () => {
    try {
      await postAppConfirm();
      console.log('appconfirm sent');
    } catch (err) {
      console.log(err);
    }
  }, 1000);
}
export function stopAutoAppConfirm() {
  if (!state.appConfirmTimer) return;
  clearInterval(state.appConfirmTimer);
  state.appConfirmTimer = null;
  log('appconfirmed');
}
export function startStatusPolling(intervalMs = 1000) {
  stopStatusPolling();
  state.statusPollTimer = setInterval(async () => {
    try {
      await pollOnce();
    } catch (err) {
      console.log('status read failed');
    }
  }, intervalMs);
  pollOnce().catch(() => {
    console.log('status read failed');
  });
}
export function stopStatusPolling() {
  if (!state.statusPollTimer) return;
  clearInterval(state.statusPollTimer);
  state.statusPollTimer = null;
}
