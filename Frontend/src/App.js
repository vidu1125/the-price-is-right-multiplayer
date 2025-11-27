import React, { useState, useEffect } from 'react';
import CreateRoomModal from './components/Room/CreateRoomModal';
import RoomList from './components/Room/RoomList';
import { roomService } from './services/roomService';
import './App.css';

function App() {
    const [showCreateModal, setShowCreateModal] = useState(false);
    const [rooms, setRooms] = useState([]);
    const [loading, setLoading] = useState(false);
    const [accountId, setAccountId] = useState(null);

    // Initialize account_id
    useEffect(() => {
        let id = localStorage.getItem('account_id');
        if (!id) {
            id = '1'; // Default test user
            localStorage.setItem('account_id', id);
        }
        setAccountId(id);
        console.log('✅ Account ID:', id);
    }, []);

    // Load rooms on mount
    useEffect(() => {
        if (accountId) {
            loadRooms();
        }
    }, [accountId]);

    const loadRooms = async () => {
        setLoading(true);
        try {
            const result = await roomService.listRooms('waiting', 'public');
            if (result.success) {
                setRooms(result.rooms);
            }
        } catch (error) {
            console.error('Failed to load rooms:', error);
        } finally {
            setLoading(false);
        }
    };

    const handleRoomCreated = (roomId) => {
        console.log('Room created:', roomId);
        setShowCreateModal(false);
        loadRooms(); // Refresh room list
    };

    const handleDeleteRoom = async (roomId) => {
        if (!window.confirm('Are you sure you want to delete this room?')) {
            return;
        }

        try {
            const result = await roomService.deleteRoom(roomId);
            if (result.success) {
                alert('✅ ' + result.message);
                loadRooms(); // Refresh list
            } else {
                alert('❌ ' + result.error);
            }
        } catch (error) {
            alert('❌ ' + (error.error || 'Failed to delete room'));
        }
    };

    const handleJoinRoom = (roomId) => {
        // TODO: Implement join room logic
        console.log('Join room:', roomId);
        alert(`Joining room ${roomId}... (Not implemented yet)`);
    };

    return (
        <div className="App">
            {/* Header */}
            <header className="app-header">
                <h1>🎮 The Price Is Right - Multiplayer</h1>
                <div className="header-info">
                    <span className="account-badge">👤 Account ID: {accountId}</span>
                    <button 
                        className="btn-refresh" 
                        onClick={loadRooms}
                        disabled={loading}
                    >
                        🔄 Refresh
                    </button>
                </div>
            </header>

            {/* Main Content */}
            <main className="app-main">
                {/* Action Bar */}
                <div className="action-bar">
                    <button 
                        className="btn-create-room" 
                        onClick={() => setShowCreateModal(true)}
                    >
                        ➕ Create New Room
                    </button>
                    <div className="room-count">
                        {rooms.length} room(s) available
                    </div>
                </div>

                {/* Room List */}
                {loading ? (
                    <div className="loading">Loading rooms...</div>
                ) : (
                    <RoomList 
                        rooms={rooms} 
                        currentAccountId={parseInt(accountId)}
                        onJoinRoom={handleJoinRoom}
                        onDeleteRoom={handleDeleteRoom}
                    />
                )}
            </main>

            {/* Create Room Modal */}
            {showCreateModal && (
                <CreateRoomModal
                    onClose={() => setShowCreateModal(false)}
                    onRoomCreated={handleRoomCreated}
                />
            )}

            {/* Footer */}
            <footer className="app-footer">
                <p>🎯 IT4062E - Network Programming Project</p>
            </footer>
        </div>
    );
}

export default App;