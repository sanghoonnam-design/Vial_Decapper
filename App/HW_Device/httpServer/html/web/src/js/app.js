import { initBootActions } from './features/boot.js';
import { initSystemActions } from './features/system.js';
import { initNetwork, loadNetworkUntilSuccess } from './features/network.js';
import { initDownload } from './features/download.js';
import { initFileManager } from './features/fs.js';
import { syncTimeUntilSuccess, waitTimeSyncUntilSuccess } from './features/timeSync.js';
import { startStatusPolling } from './features/status.js';
import { setupIpInputs } from './ui/ipInput.js';
import { state } from './state.js';

window.appJsLoaded = true;

function startWatchdogReload(intervalMs = 2000) {
  setInterval(() => {
    if (!window.appJsLoaded) {
      console.log('app.js not loaded → reload');
      location.reload();
    }
  }, intervalMs);
}

async function bootstrapInitialRequests() {
  if (sessionStorage.getItem(state.sessionKeys.waitTimeSync) === '1') {
    await waitTimeSyncUntilSuccess();
  } else {
    await syncTimeUntilSuccess(1000);
  }

  await loadNetworkUntilSuccess(1000);
}

document.addEventListener('DOMContentLoaded', async () => {
  setupIpInputs();
  initBootActions();
  initSystemActions();
  initNetwork();
  initDownload();
  initFileManager();

  await bootstrapInitialRequests();

  startStatusPolling(1000);
  startWatchdogReload(2000);
});
