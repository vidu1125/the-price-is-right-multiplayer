"""
Routes package initialization
"""
from flask import Blueprint

# Import all route blueprints here
from app.routes.room_routes import room_bp

# List of all blueprints to register
__all__ = ['room_bp']