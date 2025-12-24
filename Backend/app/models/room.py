from sqlalchemy import (
    Column, Integer, String, Timestamp, Boolean, ForeignKey
)
from .base import Base

class Room(Base):
    __tablename__ = "rooms"

    id = Column(Integer, primary_key=True)
    name = Column(String(100))
    code = Column(String(10), unique=True)
    visibility = Column(String(20))
    host_id = Column(Integer, ForeignKey("accounts.id"))
    status = Column(String(20))
    created_at = Column(Timestamp)
    updated_at = Column(Timestamp)


class RoomMember(Base):
    __tablename__ = "room_members"

    room_id = Column(Integer, ForeignKey("rooms.id"), primary_key=True)
    account_id = Column(Integer, ForeignKey("accounts.id"), primary_key=True)
    joined_at = Column(Timestamp)
