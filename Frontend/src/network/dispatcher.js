import { sendRaw, isConnected, ensureConnected } from "./socketClient";
// OPCODE import removed because not used here

const MAGIC = 0x4347;        // ⚠️ PHẢI KHỚP server
const VERSION = 1;

let seqNum = 1;
let pendingCallbacks = new Map(); // Track pending requests by seqNum

// Export function to handle responses by seqNum
export function registerPendingCallback(seqNum, callback) {
  pendingCallbacks.set(seqNum, callback);
}

export function getPendingCallback(seqNum) {
  return pendingCallbacks.get(seqNum);
}

export function removePendingCallback(seqNum) {
  pendingCallbacks.delete(seqNum);
}

export async function sendPacket(command, payload = null) {
  // Ensure socket is connected
  await ensureConnected();
  
  const connected = isConnected();
  console.log("[DISPATCH] isConnected:", connected);
  
  if (!connected) {
    console.warn("⚠️ Socket not connected, cannot send command 0x" + command.toString(16));
    return false;
  }

  const payloadLen = payload ? payload.byteLength : 0;
  const HEADER_SIZE = 16;

  const buffer = new ArrayBuffer(HEADER_SIZE + payloadLen);
  const view = new DataView(buffer);

  let offset = 0;

  view.setUint16(offset, MAGIC, false); offset += 2;
  view.setUint8(offset, VERSION);       offset += 1;
  view.setUint8(offset, 0);             offset += 1; // flags
  view.setUint16(offset, command, false); offset += 2;
  view.setUint16(offset, 0);             offset += 2; // reserved
  const currentSeqNum = seqNum;
  view.setUint32(offset, currentSeqNum, false); offset += 4;
  seqNum++;
  view.setUint32(offset, payloadLen, false); offset += 4;

  if (payloadLen > 0) {
    new Uint8Array(buffer, HEADER_SIZE).set(new Uint8Array(payload));
  }

  console.log("[DISPATCH] ✅ sending", {
    command: "0x" + command.toString(16),
    payloadLen,
    buffer: new Uint8Array(buffer)
  });

  sendRaw(buffer);
  return currentSeqNum; // Return seqNum for tracking
}
