import { postFactorySet, postSystemReset } from '../api.js';
import { el } from '../ui/elements.js';
import { log } from '../ui/log.js';
import { state } from '../state.js';
import { startReloadLoop } from './boot.js';
export function initSystemActions() {
  if (el.btnFactorySet) {
    el.btnFactorySet.addEventListener('click', async () => {
      const ok = confirm("Are you sure you want to factory reset?");
      if (!ok) return;

      try {
        await postFactorySet();
        log('FactorySet OK');
      } catch (err) {
        alert(err.toString());
      }
    });
  }

  if (el.btnSystemReset) {
    el.btnSystemReset.addEventListener('click', async () => {
      const ok = confirm("Are you sure you want to system reset?");
      if (!ok) return;

      try {
        await postSystemReset();
        log('SystemReset OK');
        sessionStorage.setItem(state.sessionKeys.waitTimeSync, '1');
        setTimeout(() => {
          startReloadLoop(1000);
        }, 500);
      } catch (err) {
        alert(err.toString());
      }
    });
  }
}
