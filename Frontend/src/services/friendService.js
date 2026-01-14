import { OPCODE } from "../network/opcode";
import { 
  sendFriendAdd, 
  sendFriendAccept, 
  sendFriendReject, 
  sendFriendRemove, 
  sendFriendList, 
  sendFriendRequests,
  sendSearchUser 
} from "../network/dispatcher";
import { registerHandler } from "../network/receiver";

// Use generated opcodes to avoid drift from backend definitions
const RES_FRIEND_ADDED = OPCODE.RES_FRIEND_ADDED;
const RES_FRIEND_REQUEST_ACCEPTED = OPCODE.RES_FRIEND_REQUEST_ACCEPTED;
const RES_FRIEND_REQUEST_REJECTED = OPCODE.RES_FRIEND_REQUEST_REJECTED;
const RES_FRIEND_REMOVED = OPCODE.RES_FRIEND_REMOVED;
const RES_FRIEND_LIST = OPCODE.RES_FRIEND_LIST;
const RES_FRIEND_REQUESTS = OPCODE.RES_FRIEND_REQUESTS;
const RES_SUCCESS = OPCODE.RES_SUCCESS;
const ERR_BAD_REQUEST = OPCODE.ERR_BAD_REQUEST;
const ERR_CONFLICT = OPCODE.ERR_CONFLICT;
const ERR_FRIEND_NOT_FOUND = OPCODE.ERR_FRIEND_NOT_FOUND;
const ERR_NOT_FOUND = OPCODE.ERR_NOT_FOUND;
const ERR_FORBIDDEN = OPCODE.ERR_FORBIDDEN;
const ERR_SERVER_ERROR = OPCODE.ERR_SERVER_ERROR;

// Callback storage for async responses
const pendingCallbacks = new Map();

// Sequence number tracking to match requests with responses
let lastSeqNum = 0;

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

// Wrapper to track which seqNum is used for each request
function trackAndSend(sendFn) {
  const currentSeqNum = ++lastSeqNum;
  console.log("[FriendService] Tracking request #" + currentSeqNum);
  sendFn();
  return currentSeqNum;
}

// Register handlers for all friend responses
function setupHandlers() {
  const opcodeMap = {
    [RES_FRIEND_ADDED]: "RES_FRIEND_ADDED",
    [RES_FRIEND_REQUEST_ACCEPTED]: "RES_FRIEND_REQUEST_ACCEPTED",
    [RES_FRIEND_REQUEST_REJECTED]: "RES_FRIEND_REQUEST_REJECTED",
    [RES_FRIEND_REMOVED]: "RES_FRIEND_REMOVED",
    [RES_FRIEND_LIST]: "RES_FRIEND_LIST",
    [RES_FRIEND_REQUESTS]: "RES_FRIEND_REQUESTS",
    [RES_SUCCESS]: "RES_SUCCESS",
    [ERR_BAD_REQUEST]: "ERR_BAD_REQUEST",
    [ERR_CONFLICT]: "ERR_CONFLICT",
    [ERR_FRIEND_NOT_FOUND]: "ERR_FRIEND_NOT_FOUND",
    [ERR_NOT_FOUND]: "ERR_NOT_FOUND",
    [ERR_FORBIDDEN]: "ERR_FORBIDDEN",
    [ERR_SERVER_ERROR]: "ERR_SERVER_ERROR",
  };

  Object.keys(opcodeMap).forEach((opcode) => {
    registerHandler(Number(opcode), (payload, seqNum) => {
      const data = parsePayload(payload);
      console.log(`[FriendService] Received ${opcodeMap[opcode]} (0x${Number(opcode).toString(16)}) seqNum:${seqNum}`, data);
      
      // Try to match by network seqNum first
      let callback = pendingCallbacks.get(seqNum);
      
      // If not found, match by order (FIFO - first waiting callback)
      if (!callback) {
        for (let [key, cb] of pendingCallbacks) {
          callback = cb;
          pendingCallbacks.delete(key);
          break;
        }
      } else {
        pendingCallbacks.delete(seqNum);
      }
      
      if (callback) {
        clearTimeout(callback.timeoutId);
        callback.resolve(data);
      } else {
        console.warn("[FriendService] No pending callback for seqNum", seqNum);
      }
    });
  });
}

// Initialize handlers once
setupHandlers();

// Helper to create a promise that waits for response
function waitForResponse(requestId, timeoutMs = 10000) {
  let timeoutId;
  
  const promise = new Promise((resolve) => {
    timeoutId = setTimeout(() => {
      pendingCallbacks.delete(requestId);
      console.error("[FriendService] Request timeout");
      resolve({ success: false, error: "Request timeout" });
    }, timeoutMs);

    pendingCallbacks.set(requestId, { resolve, timeoutId });
  });

  return promise;
}

export async function sendFriendRequest(friendId) {
  console.log("[FriendService] Sending friend request to account ID:", friendId);
  
  const requestId = trackAndSend(() => sendFriendAdd(friendId));
  const promise = waitForResponse(requestId);
  
  return promise;
}

export async function acceptFriendRequest(requestId) {
  console.log("[FriendService] Accepting friend request:", requestId);
  
  const trackId = trackAndSend(() => sendFriendAccept(requestId));
  const promise = waitForResponse(trackId);
  
  return promise;
}

export async function rejectFriendRequest(requestId) {
  console.log("[FriendService] Rejecting friend request:", requestId);
  
  const trackId = trackAndSend(() => sendFriendReject(requestId));
  const promise = waitForResponse(trackId);
  
  return promise;
}

export async function removeFriend(friendId) {
  console.log("[FriendService] Removing friend:", friendId);
  
  const trackId = trackAndSend(() => sendFriendRemove(friendId));
  const promise = waitForResponse(trackId);
  
  return promise;
}

export async function getFriendList() {
  console.log("[FriendService] Fetching friend list");
  
  const trackId = trackAndSend(() => sendFriendList());
  const promise = waitForResponse(trackId);
  
  return promise;
}

export async function getFriendRequests() {
  console.log("[FriendService] Fetching pending friend requests");
  
  const trackId = trackAndSend(() => sendFriendRequests());
  const promise = waitForResponse(trackId);
  
  return promise;
}

export async function searchUser(query) {
  console.log("[FriendService] Searching for user:", query);
  
  const trackId = trackAndSend(() => sendSearchUser(query));
  const response = await waitForResponse(trackId);
  
  // Map backend 'users' field to frontend 'results' field for consistency
  if (response?.success && response.users) {
    return {
      success: true,
      results: response.users,
      count: response.count
    };
  }
  
  return response;
}
