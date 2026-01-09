import { WebSocketServer } from "ws";
import net from "net";

const WS_PORT = 8080;
const C_HOST = process.env.C_SERVER_HOST || "network";
const C_PORT = Number(process.env.C_SERVER_PORT || 5500);

const wss = new WebSocketServer({
  port: WS_PORT,
  perMessageDeflate: false,
});

console.log(`🟢 WS Bridge listening on :${WS_PORT}`);

wss.on("connection", (ws) => {
  console.log("🔗 WS client connected");

  ws.binaryType = "arraybuffer";

  const tcp = net.createConnection(
    { host: C_HOST, port: C_PORT },
    () => console.log(`➡️ Connected to C server ${C_HOST}:${C_PORT}`)
  );

  // 🔥 TCP BUFFER (per connection)
  let tcpBuffer = Buffer.alloc(0);

  // WS → TCP
  ws.on("message", (data) => {
    const buf = Buffer.from(data);
    console.log("📥 WS → TCP", buf);
    tcp.write(buf);
  });

  // TCP → WS (FRAME-AWARE)
  tcp.on("data", (chunk) => {
    tcpBuffer = Buffer.concat([tcpBuffer, chunk]);

    while (tcpBuffer.length >= 16) {
      const magic    = tcpBuffer.readUInt16BE(0);
      const version  = tcpBuffer.readUInt8(2);
      const flags    = tcpBuffer.readUInt8(3);
      const command  = tcpBuffer.readUInt16BE(4);
      const reserved = tcpBuffer.readUInt16BE(6);
      const seqNum   = tcpBuffer.readUInt32BE(8);
      const length   = tcpBuffer.readUInt32BE(12);

      if (magic !== 0x4347) {
        console.error("❌ Bad magic from C server:", magic);
        tcp.destroy();
        return;
      }

      const totalLen = 16 + length;
      if (tcpBuffer.length < totalLen) {
        // chưa đủ payload
        return;
      }

      const frame = tcpBuffer.slice(0, totalLen);

      console.log("📤 TCP → WS FRAME", {
        command: `0x${command.toString(16)}`,
        seqNum,
        length,
      });

      ws.send(frame);

      tcpBuffer = tcpBuffer.slice(totalLen);
    }
  });

  ws.on("close", (code, reason) => {
    console.warn("❌ WS closed", code, reason.toString());
    tcp.destroy();
  });

  ws.on("error", (err) => {
    console.error("❌ WS error", err);
    tcp.destroy();
  });

  tcp.on("close", () => {
    console.warn("❌ TCP closed");
    ws.close();
  });

  tcp.on("error", (err) => {
    console.error("❌ TCP error", err);
    ws.close();
  });
});
