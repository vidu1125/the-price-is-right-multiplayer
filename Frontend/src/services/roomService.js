import { sendPacket } from '../network/dispatcher';
import { OPCODE } from '../network/opcode';
import { registerHandler } from '../network/receiver';

/**
 * Join phòng bằng ID hoặc code
 * @param {number|null} roomId - Room ID (cho join từ danh sách công khai)
 * @param {string|null} roomCode - Room code (cho join riêng tư)
 */
export async function joinRoom(roomId = null, roomCode = null) {
    console.log('[ROOM_SERVICE] joinRoom:', { roomId, roomCode });

    // Xác định phương thức join
    const byCode = roomCode ? 1 : 0;

    // Tạo payload 16-byte
    const buffer = new ArrayBuffer(16);
    const view = new DataView(buffer);

    // byte 0: by_code
    view.setUint8(0, byCode);

    // bytes 1-3: reserved (padding)
    view.setUint8(1, 0);
    view.setUint8(2, 0);
    view.setUint8(3, 0);

    // bytes 4-7: room_id (network byte order)
    if (byCode === 0 && roomId) {
        view.setUint32(4, roomId, false); // big-endian
    } else {
        view.setUint32(4, 0, false);
    }

    // bytes 8-15: room_code (8 bytes, null-terminated)
    if (byCode === 1 && roomCode) {
        const encoder = new TextEncoder();
        const codeBytes = encoder.encode(roomCode.substring(0, 8));
        const codeArray = new Uint8Array(buffer, 8, 8);

        // Clear buffer trước để tránh dữ liệu rác
        codeArray.fill(0);
        codeArray.set(codeBytes);
    }

    // Gửi packet
    sendPacket(OPCODE.CMD_JOIN_ROOM, new Uint8Array(buffer));
}

// Đăng ký handler cho RES_ROOM_JOINED
registerHandler(OPCODE.RES_ROOM_JOINED, (payload) => {
    console.log('[ROOM_SERVICE] ✅ Join phòng thành công');

    try {
        const text = new TextDecoder().decode(payload);
        const data = JSON.parse(text);
        console.log('[ROOM_SERVICE] Room Info:', data);

        // Dispatch custom event để điều hướng với data
        window.dispatchEvent(new CustomEvent('room_joined', {
            detail: data
        }));
    } catch (e) {
        console.error('[ROOM_SERVICE] Failed to parse RES_ROOM_JOINED:', e);
        // Fallback for empty payload (should not happen after server fix)
        window.dispatchEvent(new CustomEvent('room_joined', {
            detail: { success: true }
        }));
    }
});

// Đăng ký handler cho lỗi (Room Full, Game Started handled centrally or here if specific UI feedback needed)
// Currently ERR_BAD_REQUEST is handled in RoomPanel generic handler

/**
 * Toggle ready state
 */
export function toggleReady() {
    console.log('[ROOM_SERVICE] Toggling ready state');

    // Send packet with EMPTY payload (0 bytes)
    sendPacket(OPCODE.CMD_READY, new Uint8Array(0));
}

// Register handler for RES_READY_OK
registerHandler(OPCODE.RES_READY_OK, (payload) => {
    console.log('[ROOM_SERVICE] ✅ Ready state updated');
    // UI will be updated via NTF_PLAYER_READY
});
