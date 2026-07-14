import { el } from '../ui/elements.js';
import { log } from '../ui/log.js';
import { postFsList, postFsDelete, downloadFsFile, postFsUploadChunk } from '../api.js';
import { startStatusPolling, stopStatusPolling } from './status.js';
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
export function initFileManager() {

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
