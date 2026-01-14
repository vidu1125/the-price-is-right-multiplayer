// src/network/receiver.js
const handlers = new Map(); // Map<number, Array<fn>>
const defaultHandlers = []; // Array<(opcode, payload) => boolean | void>

/**
 * Đăng ký handler cho opcode response
 * @param {number} opcode
 * @param {(payload: ArrayBuffer) => void} handler
 */
export function registerHandler(opcode, handler) {
  if (opcode === undefined || opcode === null) {
    console.warn("[Receiver] Ignore registering handler for undefined opcode");
    return;
  }
  const list = handlers.get(opcode) || [];
  list.push(handler);
  handlers.set(opcode, list);

  console.log("[Receiver] registerHandler opcode", opcode, "count", list.length);
}

/**
 * Hủy đăng ký handler cho opcode
 * @param {number} opcode
 * @param {Function} handler
 */
export function unregisterHandler(opcode, handler) {
  if (opcode === undefined || opcode === null) return;
  const list = handlers.get(opcode);
  if (!list) return;

  const index = list.indexOf(handler);
  if (index !== -1) {
    list.splice(index, 1);
    handlers.set(opcode, list);
    console.log("[Receiver] unregisterHandler opcode", opcode, "count", list.length);
  }
}



// Đăng ký handler mặc định (gọi khi không có handler cụ thể cho opcode)
export function registerDefaultHandler(handler) {
  defaultHandlers.push(handler);
  console.log("[Receiver] default handler registered, count:", defaultHandlers.length);
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

  const list = handlers.get(command);

  if (!list || list.length === 0) {
    console.warn("[Receiver] no handler for opcode", "0x" + command.toString(16), "(decimal:", command, ")");
    // Log available handlers for debugging
    console.log("[Receiver] Available handlers:", Array.from(handlers.keys()).map(k => {
      try { return typeof k === 'number' ? `0x${k.toString(16)}(${k})` : `${k}`; } catch (e) { return 'ERR'; }
    }));

    let handled = false;
    console.log("[Receiver] no specific handler, trying", defaultHandlers.length, "default handlers");
    for (const handler of defaultHandlers) {
      try {
        console.log("[Receiver] invoking default handler for opcode", "0x" + command.toString(16));
        handler(command, payload);
        handled = true; // treat any default handler call as handled
      } catch (err) {
        console.error("[Receiver] default handler error", err);
      }
    }
    if (!handled) {
      console.warn("[Receiver] no handler for opcode", "0x" + command.toString(16), "seq", seqNum, "len", payloadLen);
    }
    return;
  }

  console.log("[Receiver] dispatch", {
    opcode: "0x" + command.toString(16),
    seqNum,
    payloadLen,
    handlerCount: list.length,
  });

  for (const handler of list) {
    try {
      handler(payload);
    } catch (err) {
      console.error("[Receiver] handler error for opcode", "0x" + command.toString(16), err);
    }
  }
}