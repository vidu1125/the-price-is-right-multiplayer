// src/network/receiver.js
const handlers = new Map();

/**
 * Đăng ký handler cho opcode response
 * @param {number} opcode
 * @param {(payload: ArrayBuffer) => void} handler
 */
export function registerHandler(opcode, handler) {
  handlers.set(opcode, handler);
}

/**
 * Xử lý packet raw từ server
 * Format:
 * | opcode (2B) | payload_len (2B) | payload |
 */
export function handleIncoming(buffer) {
  if (!(buffer instanceof ArrayBuffer)) {
    console.warn("[Receiver] non-binary message ignored");
    return;
  }

  const view = new DataView(buffer);

  if (buffer.byteLength < 4) {
    console.error("[Receiver] invalid packet length");
    return;
  }

  const opcode = view.getUint16(0, false);
  const payloadLen = view.getUint16(2, false);

  const payload = buffer.slice(4, 4 + payloadLen);

  console.log(
    "[Receiver]",
    "opcode=0x" + opcode.toString(16),
    "payloadLen=" + payloadLen
  );

  const handler = handlers.get(opcode);
  if (!handler) {
    console.warn("[Receiver] no handler for opcode", opcode);
    return;
  }

  handler(payload);
}
