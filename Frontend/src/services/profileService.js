import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

const encoder = new TextEncoder();
const decoder = new TextDecoder();

function parsePayload(payload) {
  const text = decoder.decode(payload);
  try {
    return JSON.parse(text);
  } catch (e) {
    return { success: false, error: "Invalid response from server" };
  }
}

let pending = null;
const TIMEOUT = 15000;

export function updateProfile({ name, avatar, bio }) {
  return new Promise((resolve, reject) => {
    if (pending) {
      clearTimeout(pending.timeoutId);
    }

    pending = { resolve, reject, timeoutId: null };
    pending.timeoutId = setTimeout(() => {
      pending = null;
      reject({ success: false, error: "Timeout updating profile" });
    }, TIMEOUT);

    const payload = encoder.encode(
      JSON.stringify({ name: name || undefined, avatar: avatar || undefined, bio: bio || undefined })
    );
    sendPacket(OPCODE.CMD_UPDATE_PROFILE, payload);
  });
}

registerHandler(OPCODE.RES_PROFILE_UPDATED, (payload) => {
  const data = parsePayload(payload);
  const isError = !data?.success;
  if (pending) {
    const { resolve, reject, timeoutId } = pending;
    clearTimeout(timeoutId);
    pending = null;
    if (isError) reject(data);
    else {
      // Persist updated profile
      if (data?.profile) {
        localStorage.setItem("profile", JSON.stringify(data.profile));
      }
      resolve(data);
    }
  }
});
