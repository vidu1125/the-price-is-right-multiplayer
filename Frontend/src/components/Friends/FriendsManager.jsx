import React, { useState, useEffect } from "react";
import {
  sendFriendRequest,
  acceptFriendRequest,
  rejectFriendRequest,
  removeFriend,
  getFriendList,
  getFriendRequests,
  searchUser,
} from "../../services/friendService";
import "./FriendsManager.css";

export default function FriendsManager() {
  const [activeTab, setActiveTab] = useState("friends"); // friends, requests, search
  const [friends, setFriends] = useState([]);
  const [pendingRequests, setPendingRequests] = useState([]);
  const [searchQuery, setSearchQuery] = useState("");
  const [searchResults, setSearchResults] = useState([]);
  const [hasSearched, setHasSearched] = useState(false);
  const [loading, setLoading] = useState(false);
  const [message, setMessage] = useState("");
  const [error, setError] = useState("");

  // Load friends list
  useEffect(() => {
    if (activeTab === "friends") {
      loadFriends();
    }
  }, [activeTab]);

  // Load pending requests
  useEffect(() => {
    if (activeTab === "requests") {
      loadPendingRequests();
    }
  }, [activeTab]);

  const loadFriends = async () => {
    setLoading(true);
    setError("");
    try {
      const res = await getFriendList();
      if (res?.success && res.friends) {
        setFriends(res.friends);
        setMessage("");
      } else {
        setFriends([]);
        setMessage(res?.error || "No friends yet");
      }
    } catch (err) {
      setError("Failed to load friends");
      setFriends([]);
    } finally {
      setLoading(false);
    }
  };

  const loadPendingRequests = async () => {
    setLoading(true);
    setError("");
    try {
      const res = await getFriendRequests();
      if (res?.success && res.requests) {
        setPendingRequests(res.requests);
        setMessage("");
      } else {
        setPendingRequests([]);
        setMessage(res?.error || "No pending requests");
      }
    } catch (err) {
      setError("Failed to load pending requests");
      setPendingRequests([]);
    } finally {
      setLoading(false);
    }
  };

  const handleSendFriendRequest = async (friendId) => {
    setLoading(true);
    setError("");
    setMessage("");

    try {
      const res = await sendFriendRequest(friendId);
      if (res?.success) {
        setMessage("Friend request sent successfully!");
        setSearchQuery("");
        setSearchResults(searchResults.filter(r => r.id !== friendId));
      } else {
        setError(res?.error || "Failed to send friend request");
      }
    } catch (err) {
      setError(err?.message || "Failed to send friend request");
    } finally {
      setLoading(false);
    }
  };

  const handleSearchUsers = async (e) => {
    e.preventDefault();
    if (!searchQuery.trim()) {
      setSearchResults([]);
      setHasSearched(false);
      return;
    }

    setLoading(true);
    setError("");
    setHasSearched(true);

    try {
      const res = await searchUser(searchQuery);
      if (res?.success && res.results) {
        setSearchResults(res.results);
      } else {
        setSearchResults([]);
        setError(res?.error || "No users found");
      }
    } catch (err) {
      setSearchResults([]);
      setError("Failed to search users");
    } finally {
      setLoading(false);
    }
  };

  const handleAcceptRequest = async (requestId) => {
    setLoading(true);
    try {
      const res = await acceptFriendRequest(requestId);
      if (res?.success) {
        setPendingRequests(
          pendingRequests.filter((r) => r.id !== requestId)
        );
        setMessage("Friend request accepted!");
      } else {
        setError(res?.error || "Failed to accept request");
      }
    } catch (err) {
      setError("Failed to accept request");
    } finally {
      setLoading(false);
    }
  };

  const handleRejectRequest = async (requestId) => {
    setLoading(true);
    try {
      const res = await rejectFriendRequest(requestId);
      if (res?.success) {
        setPendingRequests(
          pendingRequests.filter((r) => r.id !== requestId)
        );
        setMessage("Friend request rejected");
      } else {
        setError(res?.error || "Failed to reject request");
      }
    } catch (err) {
      setError("Failed to reject request");
    } finally {
      setLoading(false);
    }
  };

  const handleRemoveFriend = async (friendId) => {
    if (!window.confirm("Remove this friend?")) return;

    setLoading(true);
    try {
      const res = await removeFriend(friendId);
      if (res?.success) {
        setFriends(friends.filter((f) => f.id !== friendId));
        setMessage("Friend removed");
      } else {
        setError(res?.error || "Failed to remove friend");
      }
    } catch (err) {
      setError("Failed to remove friend");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="friends-manager">
      <div className="friends-card">
        <div className="friends-header">
          <div className="header-content">
            <div>
              <h2>Friends & Social</h2>
              <p>Manage your friends and connect with other players</p>
            </div>
            <button className="back-to-lobby-btn" onClick={() => window.location.href = '/'}>
              ← Back to Lobby
            </button>
          </div>
        </div>

        {/* Tabs */}
        <div className="friends-tabs">
          <button
            className={`tab-button ${activeTab === "friends" ? "active" : ""}`}
            onClick={() => setActiveTab("friends")}
          >
            👥 Friends {friends.length > 0 && `(${friends.length})`}
          </button>
          <button
            className={`tab-button ${activeTab === "requests" ? "active" : ""}`}
            onClick={() => setActiveTab("requests")}
          >
            📬 Requests {pendingRequests.length > 0 && `(${pendingRequests.length})`}
          </button>
          <button
            className={`tab-button ${activeTab === "search" ? "active" : ""}`}
            onClick={() => setActiveTab("search")}
          >
            🔍 Find Friends
          </button>
        </div>

        {/* Messages */}
        {message && <div className="message success">{message}</div>}
        {error && <div className="message error">{error}</div>}

        {/* Tab Content */}
        {activeTab === "friends" && (
          <div className="tab-content">
            {loading ? (
              <div className="loading">Loading friends...</div>
            ) : friends.length === 0 ? (
              <div className="empty-state">
                <p>No friends yet</p>
                <p className="hint">Go to "Find Friends" to add some!</p>
              </div>
            ) : (
              <div className="friends-list">
                {friends.map((friend) => (
                  <div key={friend.id} className="friend-item">
                    {friend.avatar && (
                      <img
                        src={friend.avatar}
                        alt={friend.name}
                        className="friend-avatar"
                      />
                    )}
                    <div className="friend-info">
                      <h3>{friend.name}</h3>
                      <p className="friend-status">
                        {friend.status || "offline"}
                      </p>
                    </div>
                    <button
                      className="friend-action remove-btn"
                      onClick={() => handleRemoveFriend(friend.id)}
                      disabled={loading}
                    >
                      Remove
                    </button>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}

        {activeTab === "requests" && (
          <div className="tab-content">
            {loading ? (
              <div className="loading">Loading pending requests...</div>
            ) : pendingRequests.length === 0 ? (
              <div className="empty-state">
                <p>No pending friend requests</p>
              </div>
            ) : (
              <div className="requests-list">
                {pendingRequests.map((request) => (
                  <div key={request.id} className="request-item">
                    {request.sender_avatar && (
                      <img
                        src={request.sender_avatar}
                        alt={request.sender_name}
                        className="request-avatar"
                      />
                    )}
                    <div className="request-info">
                      <h3>{request.sender_name || "Unknown"}</h3>
                      <p className="request-email">{request.sender_email}</p>
                    </div>
                    <div className="request-actions">
                      <button
                        className="action-btn accept-btn"
                        onClick={() => handleAcceptRequest(request.id)}
                        disabled={loading}
                      >
                        ✓ Accept
                      </button>
                      <button
                        className="action-btn reject-btn"
                        onClick={() => handleRejectRequest(request.id)}
                        disabled={loading}
                      >
                        ✗ Reject
                      </button>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}

        {activeTab === "search" && (
          <div className="tab-content">
            <form className="search-form" onSubmit={handleSearchUsers}>
              <input
                type="text"
                placeholder="Search by account ID"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="search-input"
              />
              <button
                type="submit"
                className="search-btn"
                disabled={loading || !searchQuery.trim()}
              >
                {loading ? "Searching..." : "Search"}
              </button>
            </form>

            {loading && <div className="loading">Searching users...</div>}

            {searchResults.length > 0 && (
              <div className="search-results">
                <h3>Search Results ({searchResults.length})</h3>
                <div className="results-list">
                  {searchResults.map((user) => (
                    <div key={user.id} className="result-item">
                      {user.avatar && (
                        <img
                          src={user.avatar}
                          alt={user.name}
                          className="result-avatar"
                        />
                      )}
                      <div className="result-info">
                        <h3>{user.name}</h3>
                        <p className="result-email">{user.email}</p>
                      </div>
                      <button
                        className="result-action"
                        onClick={() => handleSendFriendRequest(user.id)}
                        disabled={loading}
                      >
                        + Add Friend
                      </button>
                    </div>
                  ))}
                </div>
              </div>
            )}

            {!loading && hasSearched && searchResults.length === 0 && (
              <div className="empty-state">
                <p>No users found matching "{searchQuery}"</p>
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
}
