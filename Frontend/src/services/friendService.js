import { OPCODE } from "../network/opcode";
import { sendPacket, registerPendingCallback, getPendingCallback, removePendingCallback } from "../network/dispatcher";
import { registerDefaultHandler, registerHandler } from "../network/receiver";

// Use generated opcodes to avoid drift from backend definitions
const CMD_FRIEND_ADD = OPCODE.CMD_FRIEND_ADD;
const CMD_FRIEND_ACCEPT = OPCODE.CMD_FRIEND_ACCEPT;
const CMD_FRIEND_REJECT = OPCODE.CMD_FRIEND_REJECT;
const CMD_FRIEND_REMOVE = OPCODE.CMD_FRIEND_REMOVE;
const CMD_FRIEND_LIST = OPCODE.CMD_FRIEND_LIST;
const CMD_FRIEND_REQUESTS = OPCODE.CMD_FRIEND_REQUESTS;
const CMD_SEARCH_USER = OPCODE.CMD_SEARCH_USER;
const RES_SUCCESS = OPCODE.RES_SUCCESS; // generic success used by search


// Helper to parse JSON from ArrayBuffer payload
function parsePayload(buffer) {
  const decoder = new TextDecoder();
  const jsonStr = decoder.decode(buffer);
  try {
    return JSON.parse(jsonStr);
  } catch (e) {
    console.error("[FriendService] Failed to parse payload:", e);
    return null;
  }
}

// Register default handler to capture all responses
registerDefaultHandler((opcode, buffer, seqNum) => {
  const data = parsePayload(buffer);
  console.log("[FriendService] Received opcode 0x" + opcode.toString(16), "seqNum", seqNum, data);
  
  // Check if we have a callback for this seqNum
  const callback = getPendingCallback(seqNum);
  if (callback) {
    removePendingCallback(seqNum);
    clearTimeout(callback.timeoutId);
    callback.resolve(callback.handler(data));
  } else {
    console.warn("[FriendService] No pending callback for seqNum", seqNum);
  }
});

// Some friend flows (search) return generic RES_SUCCESS, so handle it explicitly
registerHandler(RES_SUCCESS, (payload, seqNum) => {
  const data = parsePayload(payload);
  const callback = getPendingCallback(seqNum);
  if (callback) {
    removePendingCallback(seqNum);
    clearTimeout(callback.timeoutId);
    callback.resolve(callback.handler(data));
  }
});

// Helper to send request and wait for response (await seqNum to keep correlation)
async function makeRequest(command, payload, responseHandler) {
  // Send packet and get seqNum
  const payloadBytes = new TextEncoder().encode(payload);
  console.log("[FriendService] Sending command 0x" + command.toString(16) + " with payload:", payload);
  const seqNum = await sendPacket(command, payloadBytes.buffer);

  if (seqNum === false || seqNum === undefined || seqNum === null) {
    console.error("[FriendService] Failed to send command 0x" + command.toString(16));
    return { success: false, error: "Socket not connected" };
  }

  return new Promise((resolve) => {
    // Prepare timeout
    const timeoutId = setTimeout(() => {
      console.error("[FriendService] Request timeout for command 0x" + command.toString(16));
      resolve({ success: false, error: "Request timeout" });
    }, 10000);

    // Register callback for this seqNum
    registerPendingCallback(seqNum, {
      handler: responseHandler,
      resolve,
      timeoutId,
    });
  });
}

export async function sendFriendRequest(friendId) {
  console.log("[FriendService] Sending friend request to account ID:", friendId);
  
  return makeRequest(
    CMD_FRIEND_ADD,
    JSON.stringify({ friend_id: friendId }),
    (data) => {
      if (data?.success) {
        return { success: true, message: "Friend request sent" };
      }
      return { success: false, error: data?.error || "Failed to send request" };
    }
  );
}

export async function acceptFriendRequest(requestId) {
  console.log("[FriendService] Accepting friend request:", requestId);
  
  return makeRequest(
    CMD_FRIEND_ACCEPT,
    JSON.stringify({ request_id: requestId }),
    (data) => {
      if (data?.success) {
        return { success: true, message: "Friend request accepted" };
      }
      return { success: false, error: data?.error || "Failed to accept request" };
    }
  );
}

export async function rejectFriendRequest(requestId) {
  console.log("[FriendService] Rejecting friend request:", requestId);
  
  return makeRequest(
    CMD_FRIEND_REJECT,
    JSON.stringify({ request_id: requestId }),
    (data) => {
      if (data?.success) {
        return { success: true, message: "Friend request rejected" };
      }
      return { success: false, error: data?.error || "Failed to reject request" };
    }
  );
}

export async function removeFriend(friendId) {
  console.log("[FriendService] Removing friend:", friendId);
  
  return makeRequest(
    CMD_FRIEND_REMOVE,
    JSON.stringify({ friend_id: friendId }),
    (data) => {
      if (data?.success) {
        return { success: true, message: "Friend removed" };
      }
      return { success: false, error: data?.error || "Failed to remove friend" };
    }
  );
}

export async function getFriendList() {
  console.log("[FriendService] Fetching friend list");
  
  return makeRequest(
    CMD_FRIEND_LIST,
    JSON.stringify({}),
    (data) => {
      if (data?.success) {
        return { success: true, friends: data.friends || [] };
      }
      return { success: false, error: data?.error || "Failed to fetch friends", friends: [] };
    }
  );
}

export async function getFriendRequests() {
  console.log("[FriendService] Fetching pending friend requests");
  
  return makeRequest(
    CMD_FRIEND_REQUESTS,
    JSON.stringify({}),
    (data) => {
      if (data?.success) {
        return { success: true, requests: data.requests || [] };
      }
      return { success: false, error: data?.error || "Failed to fetch requests", requests: [] };
    }
  );
}

export async function searchUser(query) {
  console.log("[FriendService] Searching for user:", query);
  
  return makeRequest(
    CMD_SEARCH_USER,
    JSON.stringify({ query }),
    (data) => {
      if (data?.success) {
        return { success: true, results: data.users || data.results || [] };
      }
      return { success: false, error: data?.error || "No users found", results: [] };
    }
  );
}
