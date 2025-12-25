import { sendRaw, isConnected } from "./socketClient";
import { OPCODE } from "./opcode";

const MAGIC = 0x4347;        // ⚠️ PHẢI KHỚP server
const VERSION = 1;

let seqNum = 1;

export function sendPacket(command, payload = null) {
  if (!isConnected()) {
    console.warn("⚠️ Socket not connected");
    return;
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
  view.setUint32(offset, seqNum++, false); offset += 4;
  view.setUint32(offset, payloadLen, false); offset += 4;

  if (payloadLen > 0) {
    new Uint8Array(buffer, HEADER_SIZE).set(new Uint8Array(payload));
  }

  console.log("[DISPATCH] send", {
    command: command.toString(16),
    payloadLen,
  });

  sendRaw(buffer);
}
