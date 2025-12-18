// GameRulesPanel.js
import { useState, useEffect } from "react";
import "./WaitingRoom.css";
import socketService from "../../../services/socketService";

export default function GameRulesPanel({ isHost, roomId, initialRules }) {
  const [editMode, setEditMode] = useState(false);
  const [loading, setLoading] = useState(false);
  const [rules, setRules] = useState({
    maxPlayers: initialRules?.maxPlayers || 5,
    mode: initialRules?.mode || 'elimination',
    wagerEnabled: initialRules?.wagerEnabled || false,
    roundTime: initialRules?.roundTime || 30
  });

  useEffect(() => {
    if (initialRules) {
      setRules(initialRules);
    }
  }, [initialRules]);

  const handleSaveRules = async () => {
    setLoading(true);
    try {
      const result = await socketService.setRules(roomId, rules);
      
      if (result.success) {
        alert("Rules updated successfully!");
        setEditMode(false);
      } else {
        alert(`Failed to update rules: ${result.error}`);
      }
    } catch (error) {
      alert("Error updating rules");
    } finally {
      setLoading(false);
    }
  };

  const getRoundTimeLabel = (seconds) => {
    if (seconds <= 20) return 'Slow';
    if (seconds <= 40) return 'Normal';
    return 'Fast';
  };

  return (
    <div className="wr-panel game-rules-panel">
      {/* Header chứa tiêu đề và nút Settings ẩn bên góc phải */}
      <div className="panel-header-with-action">
        <h3>GAME RULES</h3>
        {isHost && !editMode && (
          <button className="settings-icon-btn" onClick={() => setEditMode(true)}>
            edit
          </button>
        )}
      </div>
      
      {/* 1. MAX PLAYER */}
      <div className="rule-group rule-group-row">
        <label>Max Players:</label>
        {editMode ? (
          <select 
            value={rules.maxPlayers}
            onChange={(e) => setRules({...rules, maxPlayers: parseInt(e.target.value)})}
            className="rule-select"
          >
            <option value={4}>4</option>
            <option value={5}>5</option>
            <option value={6}>6</option>
          </select>
        ) : (
          <span className="rule-value-static">{rules.maxPlayers}</span>
        )}
      </div>

      {/* 2. MODE */}
      <div className="rule-group">
        <label>Mode:</label>
        {editMode ? (
          <div className="mode-options">
            <button 
              className={rules.mode === 'elimination' ? 'active' : ''}
              onClick={() => setRules({...rules, mode: 'elimination'})}
            >
              Eliminate
            </button>
            <button 
              className={rules.mode === 'scoring' ? 'active' : ''}
              onClick={() => setRules({...rules, mode: 'scoring'})}
            >
              Scoring
            </button>
          </div>
        ) : (
          <div className="mode-options">
            <button className={rules.mode === 'elimination' ? 'active' : ''}>
              Eliminate
            </button>
            <button className={rules.mode === 'scoring' ? 'active' : ''}>
              Scoring
            </button>
          </div>
        )}
      </div>
      
      {/* 3. WAGER MODE */}
      <div className="rule-group rule-group-row">
        <label>Wager Mode:</label>
        {editMode ? (
          <div className="status-badges-container">
            <span 
              className={`status-badge on ${rules.wagerEnabled ? 'active' : ''}`}
              onClick={() => setRules({...rules, wagerEnabled: true})}
              style={{ cursor: 'pointer' }}
            >
              ON
            </span>
            <span 
              className={`status-badge off ${!rules.wagerEnabled ? 'active' : ''}`}
              onClick={() => setRules({...rules, wagerEnabled: false})}
              style={{ cursor: 'pointer' }}
            >
              OFF
            </span>
          </div>
        ) : (
          <div className="status-badges-container">
            <span className={`status-badge on ${rules.wagerEnabled ? 'active' : ''}`}>ON</span>
            <span className={`status-badge off ${!rules.wagerEnabled ? 'active' : ''}`}>OFF</span>
          </div>
        )}
      </div>

      {/* 4. ROUND TIME */}
      <div className="rule-group">
        <label>Round Time:</label>
        {editMode ? (
          <div className="time-options">
            <button 
              className={rules.roundTime <= 20 ? 'active' : ''}
              onClick={() => setRules({...rules, roundTime: 15})}
            >
              Slow (15s)
            </button>
            <button 
              className={rules.roundTime > 20 && rules.roundTime <= 40 ? 'active' : ''}
              onClick={() => setRules({...rules, roundTime: 30})}
            >
              Normal (30s)
            </button>
            <button 
              className={rules.roundTime > 40 ? 'active' : ''}
              onClick={() => setRules({...rules, roundTime: 60})}
            >
              Fast (60s)
            </button>
          </div>
        ) : (
          <div className="time-options">
            <button className={getRoundTimeLabel(rules.roundTime) === 'Slow' ? 'active' : ''}>
              Slow
            </button>
            <button className={getRoundTimeLabel(rules.roundTime) === 'Normal' ? 'active' : ''}>
              Normal
            </button>
            <button className={getRoundTimeLabel(rules.roundTime) === 'Fast' ? 'active' : ''}>
              Fast
            </button>
          </div>
        )}
      </div>

      {/* Save/Cancel buttons when editing */}
      {editMode && (
        <div className="rule-actions">
          <button onClick={handleSaveRules} disabled={loading}>
            {loading ? 'Saving...' : 'Save'}
          </button>
          <button onClick={() => setEditMode(false)}>Cancel</button>
        </div>
      )}
    </div>
  );
}