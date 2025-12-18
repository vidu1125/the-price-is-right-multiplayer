# Backend/app/utils/time_sync.py
"""
Server time synchronization utilities
"""
import time

def get_server_time_ms():
    """
    Get current server time in milliseconds since epoch
    
    Returns:
        int: Timestamp in milliseconds
    """
    return int(time.time() * 1000)

def get_server_time_us():
    """
    Get current server time in microseconds since epoch
    
    Returns:
        int: Timestamp in microseconds
    """
    return int(time.time() * 1000000)
