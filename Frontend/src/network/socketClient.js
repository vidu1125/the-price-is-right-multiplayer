import { handleIncoming } from "./receiver";

let socket = null;

export function initSocket(url = "ws://localhost:8080") {
  if (socket) return socket;   // 🔒 chỉ tạo 1 lần

  socket = new WebSocket(url);
  socket.binaryType = "arraybuffer"; // 🔥 BẮT BUỘC (RAW BINARY)

  socket.onopen = () => {
    console.log("Connected to gateway");
  };

  socket.onmessage = (event) => {
    console.log("[Socket] recv", new Uint8Array(event.data));
    handleIncoming(event.data);  // ✅ Dispatch to handlers
  };

  socket.onerror = (err) => {
    console.error("[Socket] error", err);
  };

  socket.onclose = (e) => {
    console.warn("Socket closed", e.code, e.reason);
    socket = null;
  };

  return socket;
}

export function sendRaw(buffer) {
  if (!socket || socket.readyState !== WebSocket.OPEN) {
    console.warn("sendRaw: socket not open");
    return;
  }
  socket.send(buffer);
}

export function isConnected() {
  return socket && socket.readyState === WebSocket.OPEN;
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
