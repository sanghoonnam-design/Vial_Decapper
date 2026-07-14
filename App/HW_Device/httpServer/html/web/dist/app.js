// ===== state.js =====
const state = {
  waitReloadTimer: null,
  appConfirmTimer: null,
  statusPollTimer: null,
  watchdogTimer: null,
  sessionKeys: {
    waitTimeSync: 'waitTimeSync'
  }
};


// ===== utils.js =====
function logError(err) {
  console.error(err);
}
function delay(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}
function parseKeyValue(text) {
  const obj = {};
  const lines = String(text || '').trim().split('\n');
  for (const line of lines) {
    const idx = line.indexOf('=');
    if (idx <= 0) continue;
    const key = line.slice(0, idx).trim();
    const value = line.slice(idx + 1).trim();
    obj[key] = value;
  }
  return obj;
}
function pad2(value) {
  return String(value).padStart(2, '0');
}
function getCurrentTimeString() {
  const now = new Date();
  return `${now.getFullYear()}-${pad2(now.getMonth() + 1)}-${pad2(now.getDate())} ${pad2(now.getHours())}:${pad2(now.getMinutes())}:${pad2(now.getSeconds())}`;
}
function normalizeIpString(value) {
  return String(value || '').replace(/_/g, '').trim();
}
function isValidIpPart(part) {
  if (!/^\d{1,3}$/.test(part)) return false;
  const num = Number(part);
  return num >= 0 && num <= 255;
}
function isValidIp(value) {
  const clean = normalizeIpString(value);
  const parts = clean.split('.');
  if (parts.length !== 4) return false;
  return parts.every(isValidIpPart);
}


// ===== elements.js =====
const el = {
  btnGotoBoot:   document.getElementById('btnGotoBoot'),
  btnFactorySet: document.getElementById('btnFactorySet'),
  btnSystemReset:document.getElementById('btnSystemReset'),
  btnDownload:   document.getElementById('btnDownload'),
  btnGetNetwork: document.getElementById('btnGetNetwork'),
  btnSetNetwork: document.getElementById('btnSetNetwork'),
  appConfirmed:  document.getElementById('appConfirmed'),
  rtcTime:       document.getElementById('RtcTime'),
  ip:            document.getElementById('ip'),
  subnet:        document.getElementById('subnet'),
  gateway:       document.getElementById('gateway'),
  logBox:        document.getElementById('logBox'),
  modelName:     document.getElementById('modelName'),
  btnFsList:     document.getElementById('btnFsList'),
  btnFsDownload: document.getElementById('btnFsDownload'),
  btnFsDelete:   document.getElementById('btnFsDelete'),
  btnFsClose:    document.getElementById('btnFsClose'),
  btnFsUpload:   document.getElementById('btnFsUpload'),
  fsUploadInput: document.getElementById('fsUploadInput'),
  fsBreadcrumb:  document.getElementById('fsBreadcrumb'),
  fsFileList:    document.getElementById('fsFileList'),
  fsEditor:      document.getElementById('fsEditor'),
  fsFileName:    document.getElementById('fsFileName')
};


// ===== log.js =====
function log(msg) {
  if (!el.logBox) return;
  const line = document.createElement('div');
  line.textContent = msg;
  el.logBox.appendChild(line);
  el.logBox.scrollTop = el.logBox.scrollHeight;
}


// ===== ipInput.js =====
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
function setupIpInputs() {
  attachIpInput(el.ip);
  attachIpInput(el.subnet);
  attachIpInput(el.gateway);
}
function setIpValue(input, value) {
  if (!input) return;
  input.value = value || PLACEHOLDER;
}
function getIpValue(input, name) {
  const value = normalizeIpString(input ? input.value : '');
  if (!isValidIp(value)) {
    throw new Error(`${name} format is invalid`);
  }
  return value;
}


// ===== api.js =====
async function requestText(url, options = {}) {
  const res = await fetch(url, options);
  if (!res.ok) {
    throw new Error(`${url} failed: ${res.status}`);
  }
  return await res.text();
}
function getStatus() {
  return requestText('/status');
}
function postGotoBoot() {
  return requestText('/gotoboot', { method: 'POST' });
}
function postFactorySet() {
  return requestText('/factorySet', { method: 'POST' });
}
function postSystemReset() {
  return requestText('/systemReset', { method: 'POST' });
}
function postAppConfirm() {
  return requestText('/appconfirm', { method: 'POST' });
}
function postSetTime(timeStr) {
  return requestText('/settime', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `time=${timeStr}`
  });
}
function getNetworkInfo() {
  return requestText('/getNetwork');
}
function postSetNetwork(ip, subnet, gateway) {
  const body = `ip=${ip}\nsubnet=${subnet}\ngateway=${gateway}`;
  return requestText('/setNetwork', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body
  });
}
function postFsList(path) {
  const body = (path && path !== '/') ? `path=${path}` : 'path=/';
  return requestText('/fs/list', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body
  });
}
function postFsRead(filename) {
  return requestText('/fs/read', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `path=${filename}`
  });
}
function postFsWrite(filename, content) {
  return requestText('/fs/write', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `path=${filename}\n${content}`
  });
}
function postFsDelete(filename) {
  return requestText('/fs/delete', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `path=${filename}`
  });
}
async function postFsUploadChunk(remotePath, offset, chunkBytes) {
  const header = new TextEncoder().encode(`path=${remotePath}\noffset=${offset}\n`);
  const body = new Uint8Array(header.length + chunkBytes.length);
  body.set(header, 0);
  body.set(chunkBytes, header.length);
  const res = await fetch('/fs/upload', {
    method: 'POST',
    headers: { 'Content-Type': 'application/octet-stream' },
    body
  });
  if (!res.ok) throw new Error(`/fs/upload failed: ${res.status}`);
  return await res.text();
}
async function downloadFsFile(fullPath) {
  const res = await fetch('/fs/download', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `path=${fullPath}`
  });
  if (!res.ok) throw new Error(`/fs/download failed: ${res.status}`);
  const blob = await res.blob();
  const filename = fullPath.split('/').pop();
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}


// ===== status.js =====
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
function startAutoAppConfirm() {
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
function stopAutoAppConfirm() {
  if (!state.appConfirmTimer) return;
  clearInterval(state.appConfirmTimer);
  state.appConfirmTimer = null;
  log('appconfirmed');
}
function startStatusPolling(intervalMs = 1000) {
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
function stopStatusPolling() {
  if (!state.statusPollTimer) return;
  clearInterval(state.statusPollTimer);
  state.statusPollTimer = null;
}


// ===== network.js =====
async function loadNetwork() {
  const txt = await getNetworkInfo();
  const data = parseKeyValue(txt);
  setIpValue(el.ip, data.ip || '');
  setIpValue(el.subnet, data.subnet || '');
  setIpValue(el.gateway, data.gateway || '');
  log('Network info loaded');
}
async function loadNetworkUntilSuccess(intervalMs = 1000) {
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
function initNetwork() {
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


// ===== boot.js =====
function startReloadLoop(intervalMs = 1000) {
  if (state.waitReloadTimer) {
    clearInterval(state.waitReloadTimer);
  }
  state.waitReloadTimer = setInterval(() => {
    location.reload();
  }, intervalMs);
}
function stopReloadLoop() {
  if (!state.waitReloadTimer) return;
  clearInterval(state.waitReloadTimer);
  state.waitReloadTimer = null;
}
function initBootActions() {
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
{ startReloadLoop };


// ===== system.js =====
function initSystemActions() {
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


// ===== download.js =====
function initDownload() {
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


// ===== timeSync.js =====
async function setTimeToDevice() {
  const timeStr = getCurrentTimeString();
  try {
    await postSetTime(timeStr);
    log('Time synchronized');
    return true;
  } catch (err) {
    return false;
  }
}
async function waitTimeSyncUntilSuccess() {
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
async function syncTimeUntilSuccess(intervalMs = 1000) {
  while (true) {
    const ok = await setTimeToDevice();
    if (ok) {
      log('Initial time sync success');
      return true;
    }
    await delay(intervalMs);
  }
}


// ===== fs.js =====
/* stopStatusPolling/startStatusPolling: 다운로드에만 사용.
 * 업로드는 S6(두 번째 HTTP 소켓)이 상태폴링을 병렬 처리하므로 멈출 필요 없음. */

/* ─────────────────────────────────────────────
 * 상태
 * ───────────────────────────────────────────── */
let fsPathSegments = [];  /* 현재 경로 세그먼트 배열 (루트 = []) */
let fsSelectedFile = null;

/* ─────────────────────────────────────────────
 * 경로 유틸
 * ───────────────────────────────────────────── */
function getCurrentPath() {
  return fsPathSegments.join('/');
}

function getFullFilePath(name) {
  const dir = getCurrentPath();
  return dir ? dir + '/' + name : name;
}

/* ─────────────────────────────────────────────
 * 파일 크기 포맷
 * 소수점 3자리까지, 단위 자동 선택 (B / KB / MB)
 * ───────────────────────────────────────────── */
function formatFileSize(bytes) {
  if (bytes >= 1024 * 1024)
    return (bytes / (1024 * 1024)).toFixed(3) + ' MB';
  if (bytes >= 1024)
    return (bytes / 1024).toFixed(3) + ' KB';
  return bytes + ' B';
}

/* ─────────────────────────────────────────────
 * 브레드크럼 렌더링
 * ───────────────────────────────────────────── */
function renderBreadcrumb() {
  if (!el.fsBreadcrumb) return;
  el.fsBreadcrumb.innerHTML = '';

  const rootEl = document.createElement('span');
  rootEl.className = 'fs-crumb';
  rootEl.textContent = '/';
  rootEl.addEventListener('click', () => {
    fsPathSegments = [];
    fsRefreshList();
  });
  el.fsBreadcrumb.appendChild(rootEl);

  for (let i = 0; i < fsPathSegments.length; i++) {
    const sep = document.createElement('span');
    sep.className = 'fs-crumb-sep';
    sep.textContent = ' > ';
    el.fsBreadcrumb.appendChild(sep);

    const crumb = document.createElement('span');
    crumb.className = 'fs-crumb';
    crumb.textContent = fsPathSegments[i];
    const depth = i + 1;
    crumb.addEventListener('click', () => {
      fsPathSegments = fsPathSegments.slice(0, depth);
      fsRefreshList();
    });
    el.fsBreadcrumb.appendChild(crumb);
  }
}

/* ─────────────────────────────────────────────
 * 파일 선택 (내용 읽기 없음)
 * ───────────────────────────────────────────── */
function fsSelectFile(fullPath) {
  fsSelectedFile = fullPath;
  if (el.fsFileName) el.fsFileName.textContent = fullPath;
  if (el.fsEditor)   el.fsEditor.style.display = '';
}

/* ─────────────────────────────────────────────
 * 파일 목록 렌더링
 * ───────────────────────────────────────────── */
function renderFileList(lines) {
  if (!el.fsFileList) return;
  el.fsFileList.innerHTML = '';

  const parsed = lines
    .filter(l => l.trim())
    .map(line => {
      const parts = line.split(',');
      if (parts.length < 3) return null;
      return {
        name: parts[0],
        size: parseInt(parts[1], 10),
        type: parts[2].trim()
      };
    })
    .filter(Boolean)
    .sort((a, b) => {
      if (a.type === b.type) return a.name.localeCompare(b.name);
      return a.type === 'D' ? -1 : 1;   /* 폴더 먼저 */
    });

  if (parsed.length === 0) {
    el.fsFileList.innerHTML =
      '<div style="padding:8px;color:#888;font-size:13px;">Empty</div>';
    return;
  }

  for (const entry of parsed) {
    const { name, size, type } = entry;
    const isDir = type === 'D';
    const displayName = isDir ? name.replace(/\/$/, '') : name;

    const item = document.createElement('div');
    item.className = 'fs-file-item' + (isDir ? ' fs-dir-item' : '');

    const nameEl = document.createElement('span');
    nameEl.className = 'fs-file-name';
    nameEl.textContent = (isDir ? '📁 ' : '📄 ') + displayName;
    item.appendChild(nameEl);

    if (!isDir) {
      /* 파일 크기: B / KB / MB 자동 단위 */
      const sizeEl = document.createElement('span');
      sizeEl.className = 'fs-file-size';
      sizeEl.textContent = formatFileSize(size);
      item.appendChild(sizeEl);

      item.addEventListener('click', () => {
        document.querySelectorAll('.fs-file-item').forEach(i => i.classList.remove('selected'));
        item.classList.add('selected');
        fsSelectFile(getFullFilePath(name));
      });
    } else {
      /* 폴더 클릭 → 해당 경로로 진입 */
      item.addEventListener('click', () => {
        fsPathSegments.push(displayName);
        if (el.fsEditor) el.fsEditor.style.display = 'none';
        fsSelectedFile = null;
        fsRefreshList();
      });
    }

    el.fsFileList.appendChild(item);
  }
}

/* ─────────────────────────────────────────────
 * 파일 목록 갱신
 * ───────────────────────────────────────────── */
async function fsRefreshList() {
  renderBreadcrumb();
  if (!el.fsFileList) return;

  el.fsFileList.innerHTML =
    '<div style="padding:8px;color:#888;font-size:13px;">Loading...</div>';

  try {
    const path = getCurrentPath();
    const txt  = await postFsList(path || '/');
    const lines = txt.split('\n');
    renderFileList(lines);
    const count = lines.filter(l => l.trim() && l.includes(',F')).length;
    log('SD [' + (path || '/') + ']: ' + count + ' file(s)');
  } catch (err) {
    log('SD list error: ' + err.message);
    if (el.fsFileList)
      el.fsFileList.innerHTML =
        '<div style="padding:8px;color:#c00;font-size:13px;">SD card error</div>';
  }
}

/* ─────────────────────────────────────────────
 * initFileManager – DOMContentLoaded 에서 호출
 * ───────────────────────────────────────────── */
function initFileManager() {

  /* 목록 새로고침 */
  if (el.btnFsList) {
    el.btnFsList.addEventListener('click', () => { fsRefreshList(); });
  }

  /* 파일 다운로드
   *
   * 단일 소켓 서버이므로 다운로드 중 상태 폴링이 충돌합니다.
   * 다운로드 시작 전 폴링을 일시 중지하고, 완료/실패 후 재개합니다.
   */
  if (el.btnFsDownload) {
    el.btnFsDownload.addEventListener('click', async () => {
      if (!fsSelectedFile) return;

      stopStatusPolling();          /* 폴링 일시 중지 (소켓 충돌 방지) */
      log('Downloading: ' + fsSelectedFile);

      try {
        await downloadFsFile(fsSelectedFile);
        log('Download complete: ' + fsSelectedFile);
      } catch (err) {
        log('Download error: ' + err.message);
      } finally {
        startStatusPolling(1000);   /* 폴링 재개 */
      }
    });
  }

  /* 파일 삭제 */
  if (el.btnFsDelete) {
    el.btnFsDelete.addEventListener('click', async () => {
      if (!fsSelectedFile) return;
      if (!confirm('Delete ' + fsSelectedFile + '?')) return;

      try {
        await postFsDelete(fsSelectedFile);
        log('Deleted: ' + fsSelectedFile);
        if (el.fsEditor) el.fsEditor.style.display = 'none';
        fsSelectedFile = null;
        await fsRefreshList();
      } catch (err) {
        log('Delete error: ' + err.message);
      }
    });
  }

  /* 편집기 닫기 */
  if (el.btnFsClose) {
    el.btnFsClose.addEventListener('click', () => {
      if (el.fsEditor) el.fsEditor.style.display = 'none';
      fsSelectedFile = null;
      document.querySelectorAll('.fs-file-item').forEach(i => i.classList.remove('selected'));
    });
  }

  /* 파일 업로드
   *
   * 단일 소켓 서버 + 4KB 바디 한도 → 3000B 청크로 분할 전송.
   * 업로드 중 상태 폴링 일시 중지.
   */
  if (el.btnFsUpload) {
    el.btnFsUpload.addEventListener('click', () => {
      if (el.fsUploadInput) el.fsUploadInput.click();
    });
  }

  if (el.fsUploadInput) {
    el.fsUploadInput.addEventListener('change', async () => {
      const file = el.fsUploadInput.files[0];
      if (!file) return;

      const dir        = getCurrentPath();
      const remotePath = dir ? dir + '/' + file.name : file.name;
      el.fsUploadInput.value = '';   /* 동일 파일 재선택 허용 */

      const buffer   = await file.arrayBuffer();
      const total    = buffer.byteLength;
      const CHUNK    = 14000;
      const chunks   = Math.ceil(total / CHUNK) || 1;

      log('Uploading: ' + remotePath + ' (' + formatFileSize(total) + ', ' + chunks + ' chunk(s))');

      try {
        if (total === 0) {
          await postFsUploadChunk(remotePath, 0, new Uint8Array(0));
        } else {
          for (let i = 0; i < chunks; i++) {
            const offset   = i * CHUNK;
            const chunkLen = Math.min(CHUNK, total - offset);
            const slice    = new Uint8Array(buffer, offset, chunkLen);
            await postFsUploadChunk(remotePath, offset, slice);
            if (chunks > 1) log('  ' + (offset + chunkLen) + '/' + total + ' bytes');
          }
        }
        log('Upload complete: ' + remotePath);
        await fsRefreshList();
      } catch (err) {
        log('Upload error: ' + err.message);
      }
    });
  }

  /* //add new func: 새 파일 관련 버튼/기능은 여기에 추가 */

  fsRefreshList();
}


// ===== app.js =====
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
