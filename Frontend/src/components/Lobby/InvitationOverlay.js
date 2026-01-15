import React from 'react';
import './InvitationOverlay.css';

export default function InvitationOverlay({ invitation, onAccept, onDecline }) {
    if (!invitation) return null;

    return (
        <div className="invite-overlay">
            <div className="invite-card">
                <div className="invite-header">
                    <div className="invite-icon">🔔</div>
                    <h3>GAME INVITATION</h3>
                </div>

                <div className="invite-body">
                    <p className="invite-text">
                        Player <span className="highlight">{invitation.sender_name || `#${invitation.sender_id}`}</span> has invited you to join:
                    </p>
                    <div className="room-badge">
                        <span className="room-name">{invitation.room_name}</span>
                    </div>
                </div>

                <div className="invite-footer">
                    <button className="accept-btn" onClick={onAccept}>ACCEPT</button>
                    <button className="decline-btn" onClick={onDecline}>DECLINE</button>
                </div>
            </div>
        </div>
    );
}
