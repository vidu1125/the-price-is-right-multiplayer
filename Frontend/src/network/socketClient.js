import { handleIncoming } from "./receiver";

let socket = null;

export function initSocket(url = null) {
  if (socket) return socket;   // 🔒 chỉ tạo 1 lần

  // Auto-detect WebSocket URL if not provided
  if (!url) {
    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    const host = window.location.hostname;
    const port = 8080;
    url = `${protocol}//${host}:${port}`;
  }

  console.log("[Socket] connecting to", url);
  socket = new WebSocket(url);
  socket.binaryType = "arraybuffer"; // 🔥 BẮT BUỘC (RAW BINARY)

  socket.onopen = () => {
    console.log("Connected to gateway");
  };

  socket.onmessage = (event) => {
    const data = event.data;

    // Normalize to ArrayBuffer for receiver
    if (data instanceof ArrayBuffer) {
      console.log("[Socket] recv", new Uint8Array(data));
      handleIncoming(data);
      return;
    }

    if (data instanceof Blob) {
      data.arrayBuffer().then((buf) => {
        console.log("[Socket] recv (blob)", new Uint8Array(buf));
        handleIncoming(buf);
      }).catch((err) => console.error("[Socket] blob decode error", err));
      return;
    }

    if (ArrayBuffer.isView(data)) {
      const buf = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
      console.log("[Socket] recv (view)", new Uint8Array(buf));
      handleIncoming(buf);
      return;
    }

    console.warn("[Socket] non-binary message ignored", data);
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
