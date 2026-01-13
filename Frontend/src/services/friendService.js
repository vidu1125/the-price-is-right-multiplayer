import { sendPacket } from "../network/dispatcher";
import { registerHandler, registerDefaultHandler } from "../network/receiver";

// Friend Commands
const CMD_FRIEND_ADD = 0x0501;
const CMD_FRIEND_ACCEPT = 0x0505;
const CMD_FRIEND_REJECT = 0x0506;
const CMD_FRIEND_REMOVE = 0x0507;
const CMD_FRIEND_LIST = 0x0508;
const CMD_FRIEND_REQUESTS = 0x0509;
const CMD_SEARCH_USER = 0x050C;

// Response Codes (success = 200, errors 4xx)
const RES_SUCCESS = 200;
const RES_FRIEND_ADDED = 228;
const RES_FRIEND_REQUEST_ACCEPTED = 229;
const RES_FRIEND_REQUEST_REJECTED = 230;
const RES_FRIEND_REMOVED = 231;
const RES_FRIEND_LIST = 232;
const RES_FRIEND_REQUESTS = 233;

// Map of request IDs to callbacks for tracking responses
const responseCallbacks = new Map();
let requestCounter = 0;

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
registerDefaultHandler((opcode, buffer) => {
  const data = parsePayload(buffer);
  console.log("[FriendService] Received opcode 0x" + opcode.toString(16), data);
  
  // Check if we have a callback for this response
  // Since we can't correlate by seqNum easily, we use the first waiting callback
  const callbacks = Array.from(responseCallbacks.values());
  if (callbacks.length > 0) {
    const callback = callbacks[0];
    responseCallbacks.delete(callback.id);
    clearTimeout(callback.timeoutId);
    callback.resolve(callback.handler(data));
  }
});

// Helper to send request and wait for response
function makeRequest(command, payload, responseHandler) {
  return new Promise((resolve) => {
    const requestId = ++requestCounter;
    
    // Prepare timeout
    const timeoutId = setTimeout(() => {
      responseCallbacks.delete(requestId);
      console.error("[FriendService] Request timeout for command 0x" + command.toString(16));
      resolve({ success: false, error: "Request timeout" });
    }, 10000);

    // Store callback
    responseCallbacks.set(requestId, {
      id: requestId,
      handler: responseHandler,
      resolve,
      timeoutId,
    });

    // Send packet
    const payloadBytes = new TextEncoder().encode(payload);
    console.log("[FriendService] Sending command 0x" + command.toString(16) + " with payload:", payload);
    sendPacket(command, payloadBytes.buffer);
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
