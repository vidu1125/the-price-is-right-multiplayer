import React, { useState, useEffect } from 'react';
import './InvitationOverlay.css';

export default function InvitationOverlay({ invitation, onAccept, onDecline }) {
    const [timeLeft, setTimeLeft] = useState(60);

    useEffect(() => {
        if (!invitation) return;

        // Reset timer when a new invitation appears
        setTimeLeft(60);

        const timer = setInterval(() => {
            setTimeLeft(prev => {
                if (prev <= 1) {
                    clearInterval(timer);
                    onDecline(); // Auto decline
                    return 0;
                }
                return prev - 1;
            });
        }, 1000);

        return () => clearInterval(timer);
    }, [invitation, onDecline]);

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
                    <p className="invite-timer">Expires in {timeLeft}s</p>
                </div>

                <div className="invite-footer">
                    <button className="accept-btn" onClick={onAccept}>ACCEPT</button>
                    <button className="decline-btn" onClick={onDecline}>DECLINE</button>
                </div>
            </div>
        </div>
    );
}
