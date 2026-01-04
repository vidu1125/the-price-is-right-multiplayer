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
 * 16-byte Header Format:
 * | magic (2B) | version (1B) | flags (1B) | command (2B) | reserved (2B) | seqNum (4B) | length (4B) | payload |
 */
export function handleIncoming(buffer) {
  if (!(buffer instanceof ArrayBuffer)) {
    console.warn("[Receiver] non-binary message ignored");
    return;
  }

  const view = new DataView(buffer);

  if (buffer.byteLength < 16) {
    console.error("[Receiver] invalid packet length, need at least 16 bytes header");
    return;
  }

  // Parse 16-byte header
  const magic = view.getUint16(0, false);      // Big-endian
  const version = view.getUint8(2);
  const flags = view.getUint8(3);
  const command = view.getUint16(4, false);    // Big-endian
  const reserved = view.getUint16(6, false);
  const seqNum = view.getUint32(8, false);     // Big-endian
  const payloadLen = view.getUint32(12, false); // Big-endian

  // Validate magic number
  if (magic !== 0x4347) {
    console.error("[Receiver] invalid magic number:", magic.toString(16));
    return;
  }

  const payload = buffer.slice(16, 16 + payloadLen);

  console.log(
    "[Receiver]",
    "command=0x" + command.toString(16),
    "seqNum=" + seqNum,
    "payloadLen=" + payloadLen
  );

  const handler = handlers.get(command);
  if (!handler) {
    console.warn("[Receiver] no handler for opcode", "0x" + command.toString(16));
    return;
  }

  handler(payload);
}
