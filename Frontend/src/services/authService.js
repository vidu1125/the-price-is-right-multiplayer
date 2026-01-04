import { sendPacket } from "../network/dispatcher";
import { registerDefaultHandler, registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

console.log("[AuthService] module loading, registering default handler");

const encoder = new TextEncoder();
const decoder = new TextDecoder();

let registerPending = null;
let loginPending = null;

function parsePayload(payload) {
  const text = decoder.decode(payload);
  try {
    return JSON.parse(text);
  } catch (e) {
    return { success: false, error: "Invalid response from server" };
  }
}

function finishAuth(result, isError = false) {
  // Route to whichever is pending (register or login)
  if (registerPending) {
    const { resolve, reject, timeoutId } = registerPending;
    clearTimeout(timeoutId);
    registerPending = null;
    isError ? reject(result) : resolve(result);
  } else if (loginPending) {
    const { resolve, reject, timeoutId } = loginPending;
    clearTimeout(timeoutId);
    loginPending = null;
    isError ? reject(result) : resolve(result);
  }
}

const AUTH_TIMEOUT = 30000; // 30 seconds to tolerate cold starts
function handleAuthError(payload) {
  const data = parsePayload(payload);
  finishAuth(data, true);
}

registerDefaultHandler((opcode, payload) => {
  const RES_SUCCESS = 200;
  const RES_LOGIN_OK = 201;
  const ERR_BAD_REQUEST = 400;
  const ERR_INVALID_USERNAME = 402;
  const ERR_SERVER_ERROR = 500;
  const ERR_SERVICE_UNAVAILABLE = 501;

  const authOpcodes = [RES_SUCCESS, RES_LOGIN_OK, ERR_BAD_REQUEST, ERR_INVALID_USERNAME, ERR_SERVER_ERROR, ERR_SERVICE_UNAVAILABLE];
  const isAuth = authOpcodes.includes(opcode);

  console.log("[Auth] default handler invoked", { opcode, authOpcodes, isAuth });

  if (!isAuth) {
    console.log("[Auth] not an auth opcode, skipping");
    return false; // not an auth opcode, let other handlers try
  }

  try {
    const payloadText = new TextDecoder().decode(payload);
    console.log("[Auth] received opcode 0x" + opcode.toString(16), payloadText);
    const data = parsePayload(payload);
    const isError = !data?.success;
    finishAuth(data, isError);
  } catch (err) {
    console.error("[Auth] default handler error", err);
  }

  return true; // handled
});

console.log("[AuthService] default handler registered");

// Also register direct handlers for auth opcodes to avoid any default-handler edge cases
registerHandler(OPCODE.RES_SUCCESS, (payload) => {
  const data = parsePayload(payload);
  console.log("[Auth] direct RES_SUCCESS", data);
  finishAuth(data, !data?.success);
});

registerHandler(OPCODE.RES_LOGIN_OK, (payload) => {
  const data = parsePayload(payload);
  console.log("[Auth] direct RES_LOGIN_OK", data);
  finishAuth(data, !data?.success);
});

[OPCODE.ERR_BAD_REQUEST, OPCODE.ERR_INVALID_USERNAME, OPCODE.ERR_SERVER_ERROR, OPCODE.ERR_SERVICE_UNAVAILABLE]
  .forEach((opcode) => {
    registerHandler(opcode, (payload) => {
      const text = new TextDecoder().decode(payload);
      console.log("[Auth] direct ERR opcode", opcode, text);
      handleAuthError(payload);
    });
  });

export function registerAccount({ email, password, name }) {
  return new Promise((resolve, reject) => {
    const cleanEmail = email?.trim().toLowerCase();
    if (!cleanEmail || !password) {
      reject({ success: false, error: "Email and password are required" });
      return;
    }

    // Cancel previous pending request
    if (registerPending) {
      clearTimeout(registerPending.timeoutId);
    }
    if (loginPending) {
      clearTimeout(loginPending.timeoutId);
      loginPending = null;
    }

    registerPending = { resolve, reject, timeoutId: null };
    registerPending.timeoutId = setTimeout(() => {
      finishAuth({ success: false, error: "Server timed out. Please try again." }, true);
    }, AUTH_TIMEOUT);

    const payload = encoder.encode(JSON.stringify({ email: cleanEmail, password, name }));
    sendPacket(OPCODE.CMD_REGISTER_REQ, payload);
  });
}

export function loginAccount({ email, password }) {
  return new Promise((resolve, reject) => {
    const cleanEmail = email?.trim().toLowerCase();
    if (!cleanEmail || !password) {
      reject({ success: false, error: "Email and password are required" });
      return;
    }

    // Cancel previous pending request
    if (loginPending) {
      clearTimeout(loginPending.timeoutId);
    }
    if (registerPending) {
      clearTimeout(registerPending.timeoutId);
      registerPending = null;
    }

    loginPending = { resolve, reject, timeoutId: null };
    loginPending.timeoutId = setTimeout(() => {
      finishAuth({ success: false, error: "Server timed out. Please try again." }, true);
    }, AUTH_TIMEOUT);

    const payload = encoder.encode(JSON.stringify({ email: cleanEmail, password }));
    sendPacket(OPCODE.CMD_LOGIN_REQ, payload);
  });
}
