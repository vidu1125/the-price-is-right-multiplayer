import React, { useState } from 'react';
import { roomService } from '../../services/roomService';
import './CreateRoomModal.css';

const CreateRoomModal = ({ onClose, onRoomCreated }) => {
    const [formData, setFormData] = useState({
        name: '',
        visibility: 'public'
    });
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState('');

    const handleChange = (e) => {
        const { name, value } = e.target;
        setFormData(prev => ({ ...prev, [name]: value }));
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