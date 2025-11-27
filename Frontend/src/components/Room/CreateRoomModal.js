import React, { useState } from 'react';
import { roomService } from '../../services/roomService';
import './CreateRoomModal.css';

const CreateRoomModal = ({ onClose, onRoomCreated }) => {
    const [formData, setFormData] = useState({
        name: '',  // ← SỬA: room_name → name
        visibility: 'public',
        mode: 'scoring',  // ← SỬA: game_mode → mode
        max_players: 4,
        round_time_sec: 15,  // ← SỬA: timer_duration → round_time_sec
        wager_enabled: false
    });
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState('');

    const handleChange = (e) => {
        const { name, value, type, checked } = e.target;
        setFormData(prev => ({
            ...prev,
            [name]: type === 'checkbox' ? checked : (type === 'number' ? parseInt(value) : value)
        }));
    };

    const handleSubmit = async (e) => {
        e.preventDefault();
        setLoading(true);
        setError('');

        try {
            const result = await roomService.createRoom(formData);
            alert(`✅ ${result.message}`);
            onRoomCreated(result.room_id);
            onClose();
        } catch (err) {
            setError(err.error || 'Failed to create room');
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="modal-overlay" onClick={onClose}>
            <div className="modal-content" onClick={(e) => e.stopPropagation()}>
                <h2>🎮 Create New Room</h2>
                
                {error && <div className="error-message">{error}</div>}
                
                <form onSubmit={handleSubmit}>
                    <div className="form-group">
                        <label>Room Name *</label>
                        <input
                            type="text"
                            name="name"
                            value={formData.name}
                            onChange={handleChange}
                            placeholder="Enter room name"
                            required
                        />
                    </div>

                    <div className="form-group">
                        <label>Visibility</label>
                        <select name="visibility" value={formData.visibility} onChange={handleChange}>
                            <option value="public">🌐 Public</option>
                            <option value="private">🔒 Private</option>
                        </select>
                    </div>

                    <div className="form-group">
                        <label>Game Mode</label>
                        <select name="mode" value={formData.mode} onChange={handleChange}>
                            <option value="scoring">📊 Scoring Mode</option>
                            <option value="elimination">⚔️ Elimination Mode</option>
                        </select>
                    </div>

                    <div className="form-group">
                        <label>Max Players (4-8)</label>
                        <input
                            type="number"
                            name="max_players"
                            value={formData.max_players}
                            onChange={handleChange}
                            min="4"
                            max="8"
                            required
                        />
                    </div>

                    <div className="form-group">
                        <label>Round Time (seconds)</label>
                        <input
                            type="number"
                            name="round_time_sec"
                            value={formData.round_time_sec}
                            onChange={handleChange}
                            min="10"
                            max="60"
                            required
                        />
                    </div>

                    <div className="form-group checkbox">
                        <label>
                            <input
                                type="checkbox"
                                name="wager_enabled"
                                checked={formData.wager_enabled}
                                onChange={handleChange}
                            />
                            Enable Wager Mode 🎲
                        </label>
                    </div>

                    <div className="button-group">
                        <button type="submit" disabled={loading} className="btn-primary">
                            {loading ? 'Creating...' : 'Create Room'}
                        </button>
                        <button type="button" onClick={onClose} className="btn-secondary">
                            Cancel
                        </button>
                    </div>
                </form>
            </div>
        </div>
    );
};

export default CreateRoomModal;