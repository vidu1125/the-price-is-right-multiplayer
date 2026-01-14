import { handleIncoming } from "./receiver";

let socket = null;
let connectionPromise = null;

export function initSocket(url = "ws://localhost:8080") {
  if (socket && socket.readyState === WebSocket.OPEN) {
    console.log("[Socket] Already connected");
    return Promise.resolve(socket);
  }

  if (connectionPromise) {
    console.log("[Socket] Connection in progress...");
    return connectionPromise;
  }

  console.log("[Socket] Creating new connection to", url);
  socket = new WebSocket(url);
  socket.binaryType = "arraybuffer";

  connectionPromise = new Promise((resolve, reject) => {
    socket.onopen = () => {
      console.log("[Socket] Connected to gateway");
      connectionPromise = null;
      resolve(socket);
    };

    socket.onerror = (err) => {
      console.error("❌ [Socket] error", err);
      connectionPromise = null;
      reject(err);
    };
  });

  socket.onmessage = (event) => {
    console.log("[Socket] recv", new Uint8Array(event.data).slice(0, 20), "...");
    handleIncoming(event.data);  // ✅ Dispatch to handlers
  };

  socket.onclose = (e) => {
    console.warn("❌ [Socket] closed", e.code, e.reason);
    socket = null;
    connectionPromise = null;
  };

  return connectionPromise;
}

export function sendRaw(buffer) {
  if (!socket) {
    console.warn("sendRaw: socket is null");
    return false;
  }
  if (socket.readyState !== WebSocket.OPEN) {
    console.warn("sendRaw: socket not OPEN, state=" + socket.readyState);
    return false;
  }
  console.log("[Socket] Sending", buffer.byteLength, "bytes");
  socket.send(buffer);
  return true;
}

export function isConnected() {
  const connected = socket && socket.readyState === WebSocket.OPEN;
  console.log("[Socket] isConnected check: socket=" + (socket ? "exists" : "null") +
    ", readyState=" + (socket ? socket.readyState : "N/A") +
    ", OPEN=" + WebSocket.OPEN + ", result=" + connected);
  return connected;
}


export async function ensureConnected() {
  if (isConnected()) {
    return socket;
  }
  return initSocket();
}


export function waitForConnection(timeout = 5000) {
  return new Promise((resolve, reject) => {
    if (isConnected()) {
      resolve();
      return;
    }

    const interval = 100;
    let elapsed = 0;

    const check = setInterval(() => {
      if (isConnected()) {
        clearInterval(check);
        resolve();
      } else {
        elapsed += interval;
        if (elapsed >= timeout) {
          clearInterval(check);
          reject(new Error("Connection timeout"));
        }
      }
    }, interval);
  });
}
