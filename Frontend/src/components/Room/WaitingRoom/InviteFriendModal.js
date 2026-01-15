import React, { useState, useEffect } from 'react';
import './InviteFriendModal.css';
import { getFriendList } from '../../../services/friendService';
import { invitePlayer } from '../../../services/inviteService';

export default function InviteFriendModal({ roomId, onClose }) {
    const [friends, setFriends] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);
    const [inviting, setInviting] = useState(new Set()); // Track inviting status per friend

    useEffect(() => {
        loadFriends();
    }, []);

    const loadFriends = async () => {
        setLoading(true);
        try {
            const response = await getFriendList();
            if (response && response.success) {
                // Filter friends who might be eligible (e.g., not offline if needed, but let's show all for now)
                // Typically only ONLINE or ONLINE_IDLE friends can be invited, but server handles check too.
                setFriends(response.friends || []);
            } else {
                setError("Failed to load friends list.");
            }
        } catch (e) {
            console.error("Error loading friends:", e);
            setError("An error occurred while loading friends.");
        } finally {
            setLoading(false);
        }
    };

    const handleInvite = async (friendId) => {
        if (inviting.has(friendId)) return;

        // Optimistically mark as inviting
        setInviting(prev => new Set(prev).add(friendId));

        try {
            console.log(`Inviting friend ${friendId} to room ${roomId}`);
            const result = await invitePlayer(friendId, roomId);

            if (result.success) {
                alert("Invitation sent!");
            } else {
                alert(`Failed to invite: ${result.error || "Unknown error"}`);
                // Remove from inviting set if failed so user can retry
                setInviting(prev => {
                    const newSet = new Set(prev);
                    newSet.delete(friendId);
                    return newSet;
                });
            }
        } catch (e) {
            console.error("Invite error:", e);
            alert("Error sending invitation.");
            setInviting(prev => {
                const newSet = new Set(prev);
                newSet.delete(friendId);
                return newSet;
            });
        }
    };

    const [manualId, setManualId] = useState("");

    const handleManualInvite = async (e) => {
        e.preventDefault();
        if (!manualId.trim()) return;

        const targetId = parseInt(manualId.trim());
        if (isNaN(targetId)) {
            alert("Please enter a valid numeric ID.");
            return;
        }

        if (inviting.has(targetId)) {
            alert(`Invite to ID ${targetId} already sent or in progress.`);
            return;
        }

        console.log(`[ManualInvite] Attempting to invite ID: ${targetId}`);
        await handleInvite(targetId);
        setManualId("");
    };

    return (
        <div className="invite-modal-overlay" onClick={onClose}>
            <div className="invite-modal-content" onClick={e => e.stopPropagation()}>
                <div className="invite-modal-header">
                    <h3>Invite Friends</h3>
                    <button className="close-modal-btn" onClick={onClose}>×</button>
                </div>

                {error && <div className="invite-error">{error}</div>}

                {/* Manual Invite Section */}
                <div className="manual-invite-section">
                    <form onSubmit={handleManualInvite}>
                        <input
                            type="text"
                            placeholder="Enter Player ID"
                            value={manualId}
                            onChange={(e) => setManualId(e.target.value)}
                            className="manual-invite-input"
                        />
                        <button type="submit" className="manual-invite-btn" disabled={!manualId.trim()}>
                            Invite ID
                        </button>
                    </form>
                </div>

                <div className="invite-friends-list">
                    {loading ? (
                        <div className="invite-loading">Loading friends...</div>
                    ) : friends.length === 0 ? (
                        <div className="invite-empty">No friends found. Add some friends from the Lobby first!</div>
                    ) : (
                        friends.map(friend => (
                            <div key={friend.id} className="invite-friend-item">
                                <div className="invite-friend-info">
                                    <img
                                        src={friend.avatar || "/bg/default-avatar.png"}
                                        alt={friend.name}
                                        className="invite-friend-avatar"
                                    />
                                    <div className="invite-friend-details">
                                        <h4>{friend.name}</h4>
                                        <p className={`invite-friend-status ${friend.status ? friend.status.toLowerCase() : 'offline'}`}>
                                            {friend.status || "Offline"}
                                        </p>
                                    </div>
                                </div>
                                <button
                                    className="invite-action-btn"
                                    onClick={() => handleInvite(friend.account_id || friend.id)}
                                    disabled={inviting.has(friend.account_id || friend.id)}
                                >
                                    {inviting.has(friend.account_id || friend.id) ? "Sent" : "Invite"}
                                </button>
                            </div>
                        ))
                    )}
                </div>
            </div>
        </div>
    );
}
