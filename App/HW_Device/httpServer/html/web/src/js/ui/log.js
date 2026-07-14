import { el } from './elements.js';
export function log(msg) {
  if (!el.logBox) return;
  const line = document.createElement('div');
  line.textContent = msg;
  el.logBox.appendChild(line);
  el.logBox.scrollTop = el.logBox.scrollHeight;
}
