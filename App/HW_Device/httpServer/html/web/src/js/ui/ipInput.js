import { el } from './elements.js';
import { isValidIp, normalizeIpString } from '../utils.js';
const PLACEHOLDER = '___.___.___.___';
function sanitizeInput(value) {
  const only = String(value || '').replace(/[^0-9.]/g, '');
  const rawParts = only.split('.');
  const parts = [];
  for (let i = 0; i < rawParts.length && parts.length < 4; i++) {
    let part = rawParts[i].slice(0, 3);
    if (part.length > 0) {
      const num = Number(part);
      if (!Number.isNaN(num) && num > 255) {
        part = '255';
      }
    }
    parts.push(part);
  }
  return parts.join('.');
}
function attachIpInput(input) {
  if (!input) return;
  input.addEventListener('focus', () => {
    if (input.value === PLACEHOLDER) input.value = '';
  });
  input.addEventListener('blur', () => {
    if (!normalizeIpString(input.value)) input.value = PLACEHOLDER;
  });
  input.addEventListener('input', () => {
    input.value = sanitizeInput(input.value);
  });
}
export function setupIpInputs() {
  attachIpInput(el.ip);
  attachIpInput(el.subnet);
  attachIpInput(el.gateway);
}
export function setIpValue(input, value) {
  if (!input) return;
  input.value = value || PLACEHOLDER;
}
export function getIpValue(input, name) {
  const value = normalizeIpString(input ? input.value : '');
  if (!isValidIp(value)) {
    throw new Error(`${name} format is invalid`);
  }
  return value;
}
