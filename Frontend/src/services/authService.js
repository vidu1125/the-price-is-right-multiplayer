import { sendPacket } from "../network/dispatcher";
import { registerDefaultHandler, registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

console.log("[AuthService] module loading, registering default handler");

const encoder = new TextEncoder();
const decoder = new TextDecoder();

let registerPending = null;
let loginPending = null;
let reconnectPending = null;
let logoutPending = null;

function parsePayload(payload) {
  const text = decoder.decode(payload);
  try {
    return JSON.parse(text);
  } catch (e) {
    return { success: false, error: "Invalid response from server" };
  }
}

function finishAuth(result, isError = false) {
  // Resolve whichever auth flow is pending: register, login, reconnect
  if (registerPending) {
    const { resolve, reject, timeoutId } = registerPending;
    clearTimeout(timeoutId);
    registerPending = null;
    isError ? reject(result) : resolve(result);
    return;
  }

  if (loginPending) {
    const { resolve, reject, timeoutId } = loginPending;
    clearTimeout(timeoutId);
    loginPending = null;
    isError ? reject(result) : resolve(result);
    return;
  }

  if (reconnectPending) {
    const { resolve, reject, timeoutId } = reconnectPending;
    clearTimeout(timeoutId);
    reconnectPending = null;
    isError ? reject(result) : resolve(result);
  }

  if (logoutPending) {
    const { resolve, reject, timeoutId } = logoutPending;
    clearTimeout(timeoutId);
    logoutPending = null;
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
  const ERR_NOT_LOGGED_IN = 401;
  const ERR_INVALID_USERNAME = 402;
  const ERR_SERVER_ERROR = 500;
  const ERR_SERVICE_UNAVAILABLE = 501;

  // Only handle auth-specific opcodes here (not ERR_NOT_LOGGED_IN - it's handled elsewhere)
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

// Also register direct handlers for auth opcodes
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

// Special handler for ERR_NOT_LOGGED_IN - only force logout if from another device
registerHandler(OPCODE.ERR_NOT_LOGGED_IN, (payload) => {
  const text = new TextDecoder().decode(payload);
  console.log("[Auth] ERR_NOT_LOGGED_IN received:", text);
  
  // Check if this is a forced logout from another device
  if (text.includes("Account logged in from another device")) {
    console.warn("Account logged in elsewhere");
    clearAuth();
    alert("Your account has been logged in from another device. You have been logged out.");
    window.location.href = "/";
    return;
  }
  
  // Otherwise, if there's a pending auth request, handle as auth error
  if (loginPending || registerPending || reconnectPending) {
    console.log("[Auth] ERR_NOT_LOGGED_IN - auth request pending, treating as error");
    handleAuthError(payload);
    return;
  }
  
  // Otherwise, let other handlers (e.g., profileService) deal with it
  console.log("[Auth] ERR_NOT_LOGGED_IN - letting other handlers deal with it");
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

[OPCODE.ERR_BAD_REQUEST, OPCODE.ERR_NOT_LOGGED_IN, OPCODE.ERR_INVALID_USERNAME, OPCODE.ERR_SERVER_ERROR, OPCODE.ERR_SERVICE_UNAVAILABLE]
  .forEach((opcode) => {
    registerHandler(opcode, (payload) => {
      const text = new TextDecoder().decode(payload);
      console.log("[Auth] direct ERR opcode", opcode, text);
      handleAuthError(payload);
    });
  });

export function registerAccount({ email, password, confirm, name }) {
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

    const payload = encoder.encode(JSON.stringify({ email: cleanEmail, password, confirm, name }));
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

export function reconnectSession({ sessionId } = {}) {
  return new Promise((resolve, reject) => {
    const sid = sessionId || localStorage.getItem("session_id");
    const storedAccountId = localStorage.getItem("account_id");
    if (!sid) {
      reject({ success: false, error: "Missing session" });
      return;
    }

    // Cancel other pending auth flows
    if (reconnectPending) {
      clearTimeout(reconnectPending.timeoutId);
    }
    if (loginPending) {
      clearTimeout(loginPending.timeoutId);
      loginPending = null;
    }
    if (registerPending) {
      clearTimeout(registerPending.timeoutId);
      registerPending = null;
    }

    reconnectPending = { resolve, reject, timeoutId: null };
    reconnectPending.timeoutId = setTimeout(() => {
      finishAuth({ success: false, error: "Reconnect timed out" }, true);
    }, AUTH_TIMEOUT);

    const payloadData = { session_id: sid };
    if (storedAccountId) {
      payloadData.account_id = Number(storedAccountId);
    }

    const payload = encoder.encode(JSON.stringify(payloadData));
    sendPacket(OPCODE.CMD_RECONNECT, payload);
  });
}

export function logoutAccount() {
  return new Promise((resolve, reject) => {
    const sessionId = localStorage.getItem("session_id");

    // If already logged out locally, resolve quickly
    if (!sessionId) {
      clearAuth();
      resolve({ success: true, message: "Already logged out" });
      return;
    }

    if (logoutPending) {
      clearTimeout(logoutPending.timeoutId);
    }

    logoutPending = { resolve, reject, timeoutId: null };
    logoutPending.timeoutId = setTimeout(() => {
      finishAuth({ success: false, error: "Logout timed out" }, true);
    }, AUTH_TIMEOUT);

    const payload = encoder.encode(JSON.stringify({ session_id: sessionId }));
    sendPacket(OPCODE.CMD_LOGOUT_REQ, payload);
  }).then((result) => {
    if (result?.success) {
      clearAuth();
    }
    return result;
  });
}

// Persist auth to localStorage
export function persistAuth(data) {
  if (data?.account_id) localStorage.setItem("account_id", String(data.account_id));
  if (data?.session_id) localStorage.setItem("session_id", String(data.session_id));
  if (data?.profile?.email) localStorage.setItem("email", data.profile.email);
  // Store other profile data for UI (display name, avatar, etc)
  if (data?.profile) localStorage.setItem("profile", JSON.stringify(data.profile));
}

// Get current auth state
export function getAuthState() {
  return {
    accountId: localStorage.getItem("account_id"),
    sessionId: localStorage.getItem("session_id"),
    email: localStorage.getItem("email"),
    profile: JSON.parse(localStorage.getItem("profile") || "{}"),
  };
}

// Clear auth
export function clearAuth() {
  localStorage.removeItem("account_id");
  localStorage.removeItem("session_id");
  localStorage.removeItem("email");
  localStorage.removeItem("profile");
}
