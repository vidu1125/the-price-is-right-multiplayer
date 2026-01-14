import { sendRaw, isConnected } from "./socketClient";
import { OPCODE } from "./opcode";

const MAGIC = 0x4347;        // ⚠️ PHẢI KHỚP server
const VERSION = 1;

let seqNum = 1;

export function sendPacket(command, payload = null) {
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
  view.setUint32(offset, seqNum++, false); offset += 4;
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
  return true;
}

// ============================================================================
// FRIEND HELPER FUNCTIONS
// ============================================================================

function jsonToPayload(jsonObj) {
  const jsonStr = JSON.stringify(jsonObj);
  const encoder = new TextEncoder();
  return encoder.encode(jsonStr).buffer;
}

export function sendFriendAdd(friendId = null, friendEmail = null) {
  const payload = {};
  if (friendId !== null) {
    payload.friend_id = friendId;
  }
  if (friendEmail !== null) {
    payload.friend_email = friendEmail;
  }
  return sendPacket(OPCODE.CMD_FRIEND_ADD, jsonToPayload(payload));
}

export function sendFriendAccept(requestId) {
  const payload = { request_id: requestId };
  return sendPacket(OPCODE.CMD_FRIEND_ACCEPT, jsonToPayload(payload));
}

export function sendFriendReject(requestId) {
  const payload = { request_id: requestId };
  return sendPacket(OPCODE.CMD_FRIEND_REJECT, jsonToPayload(payload));
}

export function sendFriendRemove(friendId) {
  const payload = { friend_id: friendId };
  return sendPacket(OPCODE.CMD_FRIEND_REMOVE, jsonToPayload(payload));
}

export function sendFriendList() {
  return sendPacket(OPCODE.CMD_FRIEND_LIST, null);
}

export function sendFriendRequests() {
  return sendPacket(OPCODE.CMD_FRIEND_REQUESTS, null);
}

export function sendSearchUser(query) {
  const payload = { query: query };
  return sendPacket(OPCODE.CMD_SEARCH_USER, jsonToPayload(payload));
}