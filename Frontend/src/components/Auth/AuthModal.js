import React from 'react';
import Login from './Login';
import Register from './Register';
import './AuthModal.css';

function AuthModal({ isOpen, mode, onClose, onLoginSuccess, onRegisterSuccess, onSwitchMode }) {
  if (!isOpen) return null;

  return (
    <div className="auth-modal-overlay" onClick={onClose}>
      <div className="auth-modal-content" onClick={(e) => e.stopPropagation()}>
        <button className="auth-modal-close" onClick={onClose}>✕</button>
        {mode === 'login' ? (
          <Login 
            onLoginSuccess={(user) => {
              onLoginSuccess(user);
              onClose();
            }}
            onSwitchToRegister={() => onSwitchMode('register')}
          />
        ) : (
          <Register 
            onRegisterSuccess={(user) => {
              onRegisterSuccess(user);
              onClose();
            }}
            onSwitchToLogin={() => onSwitchMode('login')}
          />
        )}
      </div>
    </div>
  );
}

export default AuthModal;
