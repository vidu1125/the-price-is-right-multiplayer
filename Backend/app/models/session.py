from sqlalchemy import (
    Column, Integer, Boolean, Timestamp, ForeignKey
)
from sqlalchemy.dialects.postgresql import UUID
from .base import Base


class Session(Base):
    __tablename__ = "sessions"

    account_id = Column(Integer, ForeignKey("accounts.id"), primary_key=True)
    session_id = Column(UUID, nullable=False)
    connected = Column(Boolean, default=True)
    updated_at = Column(Timestamp)
