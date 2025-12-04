import React from 'react';
import './RoomList.css';

const RoomList = ({ rooms, currentAccountId, onJoinRoom, onDeleteRoom }) => {
    if (rooms.length === 0) {
        return (
            <div className="empty-state">
                <div className="empty-icon">🎮</div>
                <h2>No rooms available</h2>
                <p>Be the first to create a room!</p>
            </div>
        );
    }

    return (
        <div className="room-list">
            {rooms.map(room => (
                <div key={room.id} className="room-card">
                    <div className="room-header">
                        <h3 className="room-name">{room.name}</h3>
                        <span className={`room-status status-${room.status}`}>
                            {room.status}
                        </span>
                    </div>

                    <div className="room-info">
                        <div className="info-item">
                            <span className="info-label">👤 Host:</span>
                            <span className="info-value">{room.host_name}</span>
                        </div>
                        <div className="info-item">
                            <span className="info-label">👥 Players:</span>
                            <span className="info-value">{room.current_players}</span>
                        </div>
                        <div className="info-item">
                            <span className="info-label">🔒 Visibility:</span>
                            <span className="info-value">
                                {room.visibility === 'public' ? '🌐 Public' : '🔒 Private'}
                            </span>
                        </div>
                    </div>

                    <div className="room-actions">
                        {room.host_id === currentAccountId ? (
                            <>
                                <button 
                                    className="btn-action btn-delete"
                                    onClick={() => onDeleteRoom(room.id)}
                                >
                                    🗑️ Delete
                                </button>
                                <button 
                                    className="btn-action btn-manage"
                                    onClick={() => alert('Manage room (Not implemented yet)')}
                                >
                                    ⚙️ Manage
                                </button>
                            </>
                        ) : (
                            <button 
                                className="btn-action btn-join"
                                onClick={() => onJoinRoom(room.id)}
                            >
                                ➡️ Join Room
                            </button>
                        )}
                    </div>
                </div>
            ))}
        </div>
    );
};

export default RoomList;