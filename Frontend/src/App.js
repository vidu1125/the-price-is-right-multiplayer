import React, { useState, useEffect } from 'react';
import CreateRoomModal from './components/Room/CreateRoomModal';
import RoomList from './components/Room/RoomList';
import AuthModal from './components/Auth/AuthModal';
import { roomService } from './services/roomService';
import { authService } from './services/authService';
import './App.css';

function App() {
    const [showCreateModal, setShowCreateModal] = useState(false);
    const [showAuthModal, setShowAuthModal] = useState(false);
    const [rooms, setRooms] = useState([]);
    const [loading, setLoading] = useState(false);
    const [accountId, setAccountId] = useState(null);
    const [isLoggedIn, setIsLoggedIn] = useState(false);
    const [currentUser, setCurrentUser] = useState(null);
    const [authMode, setAuthMode] = useState('login'); // 'login' or 'register'

    // Initialize account_id
    useEffect(() => {
        let id = localStorage.getItem('account_id');
        if (id) {
            setAccountId(id);
            setIsLoggedIn(true);
            const user = authService.getCurrentUser();
            setCurrentUser(user);
            console.log('✅ Account ID:', id);
        }
    }, []);

    // Ensure auth modal is closed on initial load (defensive)
    useEffect(() => {
        setShowAuthModal(false);
    }, []);

    // Load rooms on mount
    useEffect(() => {
        if (accountId && isLoggedIn) {
            loadRooms();
        }
    }, [accountId, isLoggedIn]);

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

    const handleLoginSuccess = (user) => {
        setAccountId(user.id);
        setIsLoggedIn(true);
        setCurrentUser(authService.getCurrentUser());
        setShowAuthModal(false);
    };

    const handleRegisterSuccess = (user) => {
        setAccountId(user.id);
        setIsLoggedIn(true);
        setCurrentUser(authService.getCurrentUser());
        setShowAuthModal(false);
    };

    const handleLogout = () => {
        authService.logout();
        setAccountId(null);
        setIsLoggedIn(false);
        setCurrentUser(null);
        setRooms([]);
        setAuthMode('login');
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

    // Load rooms when component mounts (no auth required)
    useEffect(() => {
        loadRooms();
    }, []);

    return (
        <div className="App">
            {/* Header */}
            <header className="app-header">
                <h1>🎮 The Price Is Right - Multiplayer</h1>
                <div className="header-info">
                    {isLoggedIn ? (
                        <div className="user-info">
                            <span>👤 {currentUser?.name || currentUser?.email}</span>
                            <button 
                                className="logout-button" 
                                onClick={handleLogout}
                            >
                                Đăng xuất
                            </button>
                        </div>
                    ) : (
                        <div className="user-info">
                            <button 
                                className="auth-nav-button" 
                                onClick={() => {
                                    setAuthMode('login');
                                    setShowAuthModal(true);
                                }}
                            >
                                Đăng nhập
                            </button>
                            <button 
                                className="auth-nav-button active" 
                                onClick={() => {
                                    setAuthMode('register');
                                    setShowAuthModal(true);
                                }}
                            >
                                Đăng ký
                            </button>
                        </div>
                    )}
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
                        onClick={() => {
                            if (!isLoggedIn) {
                                alert('Vui lòng đăng nhập để tạo phòng');
                                setShowAuthModal(true);
                                return;
                            }
                            setShowCreateModal(true);
                        }}
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
                        currentAccountId={accountId ? parseInt(accountId) : null}
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

            {/* Auth Modal */}
            <AuthModal 
                isOpen={showAuthModal}
                mode={authMode}
                onClose={() => setShowAuthModal(false)}
                onLoginSuccess={handleLoginSuccess}
                onRegisterSuccess={handleRegisterSuccess}
                onSwitchMode={setAuthMode}
            />

            {/* Footer */}
            <footer className="app-footer">
                <p>🎯 IT4062E - Network Programming Project</p>
            </footer>
        </div>
    );
}

export default App;