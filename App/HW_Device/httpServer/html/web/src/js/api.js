async function requestText(url, options = {}) {
  const res = await fetch(url, options);
  if (!res.ok) {
    throw new Error(`${url} failed: ${res.status}`);
  }
  return await res.text();
}
export function getStatus() {
  return requestText('/status');
}
export function postGotoBoot() {
  return requestText('/gotoboot', { method: 'POST' });
}
export function postFactorySet() {
  return requestText('/factorySet', { method: 'POST' });
}
export function postSystemReset() {
  return requestText('/systemReset', { method: 'POST' });
}
export function postAppConfirm() {
  return requestText('/appconfirm', { method: 'POST' });
}
export function postSetTime(timeStr) {
  return requestText('/settime', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `time=${timeStr}`
  });
}
export function getNetworkInfo() {
  return requestText('/getNetwork');
}
export function postSetNetwork(ip, subnet, gateway) {
  const body = `ip=${ip}\nsubnet=${subnet}\ngateway=${gateway}`;
  return requestText('/setNetwork', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body
  });
}
export function postFsList(path) {
  const body = (path && path !== '/') ? `path=${path}` : 'path=/';
  return requestText('/fs/list', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body
  });
}
export function postFsRead(filename) {
  return requestText('/fs/read', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `path=${filename}`
  });
}
export function postFsWrite(filename, content) {
  return requestText('/fs/write', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `path=${filename}\n${content}`
  });
}
export function postFsDelete(filename) {
  return requestText('/fs/delete', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: `path=${filename}`
  });
}
export async function postFsUploadChunk(remotePath, offset, chunkBytes) {
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
export async function downloadFsFile(fullPath) {
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
