import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";
import { waitForConnection } from "../network/socketClient";

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

// RES_PROFILE_FOUND is 227
const RES_PROFILE_FOUND = 227;

let fetchPending = null;
let profileCache = null; // In-memory cache

// Try to load from localStorage on init
try {
  const saved = localStorage.getItem("profile");
  if (saved) {
    profileCache = JSON.parse(saved);
  }
} catch (e) {
  console.warn("Failed to load profile from storage", e);
}

export function fetchProfile(forceRefresh = false) {
  console.log("[PROFILE_SERVICE] fetchProfile called, force:", forceRefresh);

  return new Promise((resolve, reject) => {
    // 1. Return cache if available and not forcing refresh
    if (!forceRefresh && profileCache) {
      console.log("[PROFILE_SERVICE] Returning cached profile");
      resolve(profileCache);
      return;
    }

    if (fetchPending) {
      clearTimeout(fetchPending.timeoutId);
    }

    fetchPending = { resolve, reject, timeoutId: null };
    fetchPending.timeoutId = setTimeout(() => {
      fetchPending = null;
      console.error("[PROFILE_SERVICE] Timeout fetching profile");
      reject({ success: false, error: "Timeout fetching profile" });
    }, TIMEOUT);

    waitForConnection().then(() => {
      console.log("[PROFILE_SERVICE] Socket ready, sending CMD_GET_PROFILE");
      const cmd = OPCODE.CMD_GET_PROFILE || 0x0105;

      // Explicit payload object (even if empty) for clarity
      const payloadObj = {};
      const payloadStr = JSON.stringify(payloadObj);
      const payloadBuf = encoder.encode(payloadStr);

      sendPacket(cmd, payloadBuf);
    }).catch(err => {
      console.error("[PROFILE_SERVICE] Socket failed", err);
      if (fetchPending) {
        clearTimeout(fetchPending.timeoutId);
        fetchPending = null;
        reject({ success: false, error: "Socket not connected" });
      }
    });
  });
}

registerHandler(RES_PROFILE_FOUND, (payload) => {
  const data = parsePayload(payload);
  console.log("[PROFILE_SERVICE] Received profile data", data);

  // Always update cache when we get fresh data
  if (data?.success && data?.profile) {
    profileCache = data.profile;
    localStorage.setItem("profile", JSON.stringify(data.profile));
  }

  if (fetchPending) {
    const { resolve, reject, timeoutId } = fetchPending;
    clearTimeout(timeoutId);
    fetchPending = null;
    if (!data?.success) reject(data);
    else {
      if (data?.profile) {
        localStorage.setItem("profile", JSON.stringify(data.profile));
      }
      resolve(data.profile);
    }
  }
});
