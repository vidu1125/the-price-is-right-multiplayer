import { sendPacket } from "../network/dispatcher";
import { registerHandler } from "../network/receiver";
import { OPCODE } from "../network/opcode";

/**
 * Gửi request xem history
 * CMD_HIST = 0x0502
 */
export function viewHistory() {
  console.log("[Service][History] viewHistory() called");
  sendPacket(OPCODE.CMD_HIST);
}

/**
 * Nhận response từ server
 * CMD_HIST_RESP (ví dụ: 0x0503)
 */
registerHandler(OPCODE.CMD_HIST_RESP, (payload) => {
  console.log("[Service][History] response payload:", payload);

  // TODO:
  // - parse payload
  // - update state / store
});
