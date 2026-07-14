import { postSetTime } from '../api.js';
import { state } from '../state.js';
import { delay, getCurrentTimeString } from '../utils.js';
import { log } from '../ui/log.js';
import { stopReloadLoop } from './boot.js';

export async function setTimeToDevice() {
  const timeStr = getCurrentTimeString();
  try {
    await postSetTime(timeStr);
    log('Time synchronized');
    return true;
  } catch (err) {
    return false;
  }
}

export async function waitTimeSyncUntilSuccess() {
  log('Waiting device reboot...');
  while (true) {
    const ok = await setTimeToDevice();
    if (ok) {
      log('Time sync success after reset');
      sessionStorage.removeItem(state.sessionKeys.waitTimeSync);
      stopReloadLoop();
      break;
    }
    await delay(1000);
  }
}

export async function syncTimeUntilSuccess(intervalMs = 1000) {
  while (true) {
    const ok = await setTimeToDevice();
    if (ok) {
      log('Initial time sync success');
      return true;
    }
    await delay(intervalMs);
  }
}
