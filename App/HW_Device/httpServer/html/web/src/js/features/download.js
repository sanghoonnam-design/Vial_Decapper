import { el } from '../ui/elements.js';
import { log } from '../ui/log.js';
export function initDownload() {
  if (!el.btnDownload) return;
  el.btnDownload.addEventListener('click', () => {
    const link = document.createElement('a');
    link.href = '/download';
    link.download = 'download.bin';
    document.body.appendChild(link);
    link.click();
    link.remove();
    log('Download requested');
  });
}
