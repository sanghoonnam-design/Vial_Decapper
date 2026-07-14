export function logError(err) {
  console.error(err);
}
export function delay(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}
export function parseKeyValue(text) {
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
export function pad2(value) {
  return String(value).padStart(2, '0');
}
export function getCurrentTimeString() {
  const now = new Date();
  return `${now.getFullYear()}-${pad2(now.getMonth() + 1)}-${pad2(now.getDate())} ${pad2(now.getHours())}:${pad2(now.getMinutes())}:${pad2(now.getSeconds())}`;
}
export function normalizeIpString(value) {
  return String(value || '').replace(/_/g, '').trim();
}
export function isValidIpPart(part) {
  if (!/^\d{1,3}$/.test(part)) return false;
  const num = Number(part);
  return num >= 0 && num <= 255;
}
export function isValidIp(value) {
  const clean = normalizeIpString(value);
  const parts = clean.split('.');
  if (parts.length !== 4) return false;
  return parts.every(isValidIpPart);
}
