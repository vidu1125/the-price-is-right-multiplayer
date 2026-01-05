from app.models import db
from .base import Base
from sqlalchemy.sql import func
from sqlalchemy.orm import relationship

class Room(Base):
    __tablename__ = "rooms"

    id = db.Column(db.Integer, primary_key=True)

    name = db.Column(db.String(100), nullable=False)
    code = db.Column(db.String(10), unique=True, nullable=False)

    visibility = db.Column(db.String(20), default="public")

    host_id = db.Column(
        db.Integer,
        db.ForeignKey("accounts.id"),
        nullable=False
    )

    status = db.Column(db.String(20), default="waiting")

    # Game settings
    mode = db.Column(db.String(20), default="scoring")
    max_players = db.Column(db.Integer, default=5)

    wager_mode = db.Column(db.Boolean, default=False)

    created_at = db.Column(db.DateTime, server_default=func.now())
    updated_at = db.Column(db.DateTime, onupdate=func.now())
    
    # Relationship to get members
    members = relationship("RoomMember", backref="room", lazy="select")


class RoomMember(Base):
    __tablename__ = "room_members"

    room_id = db.Column(
        db.Integer,
        db.ForeignKey("rooms.id"),
        primary_key=True
    )

    account_id = db.Column(
        db.Integer,
        db.ForeignKey("accounts.id"),
        primary_key=True
    )


    joined_at = db.Column(db.DateTime, server_default=func.now())
