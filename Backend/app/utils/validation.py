# Backend/app/utils/validation.py
"""
Input validation utilities
"""
import re

def validate_room_name(name):
    """Validate room name (3-63 characters, alphanumeric + spaces)"""
    if not name or len(name) < 3 or len(name) > 63:
        return False
    
    # Allow alphanumeric, spaces, and basic punctuation
    if not re.match(r'^[a-zA-Z0-9\s\-_]+$', name):
        return False
    
    return True

def validate_room_settings(mode, max_players, round_time):
    """Validate room settings"""
    # Mode validation
    if mode not in ['scoring', 'elimination']:
        return False
    
    # Elimination mode requires exactly 4 players
    if mode == 'elimination' and max_players != 4:
        return False
    
    # Player count validation
    if max_players < 4 or max_players > 6:
        return False
    
    # Round time validation (15-60 seconds)
    if round_time < 15 or round_time > 60:
        return False
    
    return True

def validate_username(username):
    """Validate username (3-32 characters, alphanumeric + underscore)"""
    if not username or len(username) < 3 or len(username) > 32:
        return False
    
    if not re.match(r'^[a-zA-Z0-9_]+$', username):
        return False
    
    return True
