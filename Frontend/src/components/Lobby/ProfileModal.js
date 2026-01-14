import React, { useState } from 'react';
import './ProfileModal.css';
import { changePassword } from '../../services/profileService';

export default function ProfileModal({ profile, onClose }) {
    const [showPasswordForm, setShowPasswordForm] = useState(false);
    const [oldPassword, setOldPassword] = useState('');
    const [newPassword, setNewPassword] = useState('');
    const [error, setError] = useState('');
    const [success, setSuccess] = useState('');
    const [loading, setLoading] = useState(false);

    const handleChangePassword = async (e) => {
        e.preventDefault();
        setError('');
        setSuccess('');
        setLoading(true);

        try {
            const res = await changePassword(oldPassword, newPassword);
            if (res.success) {
                setSuccess('Password changed successfully!');
                setOldPassword('');
                setNewPassword('');
                setTimeout(() => {
                    setShowPasswordForm(false);
                    setSuccess('');
                }, 2000);
            } else {
                setError(res.error || 'Failed to change password');
            }
        } catch (err) {
            setError(err.error || 'Failed to change password');
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="profile-modal-overlay">
            <div className="profile-modal">
                <h2>PROFILE</h2>

                <div className="profile-info">
                    <div className="info-row">
                        <span className="info-label">Name:</span>
                        <span>{profile?.name}</span>
                    </div>
                    <div className="info-row">
                        <span className="info-label">Email:</span>
                        <span>{profile?.email}</span>
                    </div>
                    {/* Add more profile fields here if needed */}
                </div>

                <div className="password-section">
                    {!showPasswordForm ? (
                        <button
                            className="change-pass-btn"
                            style={{ width: '100%' }}
                            onClick={() => setShowPasswordForm(true)}
                        >
                            Change Password
                        </button>
                    ) : (
                        <form onSubmit={handleChangePassword} className="password-form">
                            <div className="form-group">
                                <input
                                    type="password"
                                    placeholder="Current Password"
                                    value={oldPassword}
                                    onChange={(e) => setOldPassword(e.target.value)}
                                    required
                                />
                            </div>
                            <div className="form-group">
                                <input
                                    type="password"
                                    placeholder="New Password (min 6 chars)"
                                    value={newPassword}
                                    onChange={(e) => setNewPassword(e.target.value)}
                                    required
                                    minLength={6}
                                />
                            </div>

                            {error && <div className="error-text">{error}</div>}
                            {success && <div className="success-text">{success}</div>}

                            <button
                                type="submit"
                                className="change-pass-btn"
                                disabled={loading}
                            >
                                {loading ? 'Updating...' : 'Update Password'}
                            </button>
                            <button
                                type="button"
                                className="close-btn"
                                style={{ background: '#4B5563', marginTop: '0.5rem' }}
                                onClick={() => setShowPasswordForm(false)}
                            >
                                Cancel
                            </button>
                        </form>
                    )}
                </div>

                <button className="close-btn" onClick={onClose}>
                    Close
                </button>
            </div>
        </div>
    );
}
