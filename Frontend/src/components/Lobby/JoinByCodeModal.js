import React, { useState } from 'react';
import { joinRoom } from '../../services/roomService';
import "./RoomPanel.css"; // Reuse modal styles

export default function JoinByCodeModal({ onClose }) {
    const [code, setCode] = useState('');

    const handleSubmit = async (e) => {
        e.preventDefault();

        if (code.length !== 6) {
            alert('Mã phòng phải có 6 ký tự');
            return;
        }

        try {
            await joinRoom(null, code.toUpperCase());
            onClose();
        } catch (error) {
            console.error(error);
            alert('Lỗi join phòng');
        }
    };

    return (
        <div className="modal-overlay" onClick={onClose}>
            <div className="modal-content" onClick={(e) => e.stopPropagation()}>
                <h2>Find Room</h2>
                <form onSubmit={handleSubmit}>
                    <label>
                        Enter Room Code:
                        <input
                            type="text"
                            value={code}
                            onChange={(e) => setCode(e.target.value)}
                            placeholder="6-character code"
                            maxLength={6}
                            style={{ textTransform: 'uppercase' }}
                            required
                        />
                    </label>
                    <div className="modal-actions">
                        <button type="submit" className="btn-primary">Join</button>
                        <button type="button" onClick={onClose} className="btn-secondary">Cancel</button>
                    </div>
                </form>
            </div>
        </div>
    );
}
