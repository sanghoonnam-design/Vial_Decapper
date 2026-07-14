import { getNetworkInfo, postSetNetwork } from '../api.js';
import { el } from '../ui/elements.js';
import { log } from '../ui/log.js';
import { parseKeyValue, delay } from '../utils.js';
import { getIpValue, setIpValue } from '../ui/ipInput.js';

export async function loadNetwork() {
  const txt = await getNetworkInfo();
  const data = parseKeyValue(txt);
  setIpValue(el.ip, data.ip || '');
  setIpValue(el.subnet, data.subnet || '');
  setIpValue(el.gateway, data.gateway || '');
  log('Network info loaded');
}

export async function loadNetworkUntilSuccess(intervalMs = 1000) {
  while (true) {
    try {
      await loadNetwork();
      log('Initial network load success');
      return true;
    } catch (err) {
      await delay(intervalMs);
    }
  }
}

async function saveNetwork() {
  const ip = getIpValue(el.ip, 'IP');
  const subnet = getIpValue(el.subnet, 'Subnet');
  const gateway = getIpValue(el.gateway, 'Gateway');
  await postSetNetwork(ip, subnet, gateway);
  log('Network settings saved');
}
export function initNetwork() {
  if (el.btnGetNetwork) {
    el.btnGetNetwork.addEventListener('click', async () => {
      try {
        await loadNetwork();
      } catch (err) {
        alert(err.toString());
      }
    });
  }
  if (el.btnSetNetwork) {
    el.btnSetNetwork.addEventListener('click', async () => {
      try {
        await saveNetwork();
      } catch (err) {
        alert(err.toString());
      }
    });
  }
}
