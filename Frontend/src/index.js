import React from 'react';
import ReactDOM from 'react-dom/client';
// ✅ Import handlers FIRST (at module load time)
import './services/authService';
import { initSocket } from './network/socketClient';
import App from './App';
import "./index.css";

// ✅ Init socket immediately (before App mounts)
initSocket();

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(<App />);

